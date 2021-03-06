#include <testlib/testlib_test.h>

#if HAS_OPENCL
#include <boxm2/volm/boxm2_volm_wr3db_index.h>
#include <boxm2/volm/boxm2_volm_wr3db_index_sptr.h>
#include <bbas/volm/volm_query.h>
#include <bbas/volm/volm_query_sptr.h>
#include <volm/volm_camera_space.h>
#include <volm/volm_camera_space_sptr.h>
#include <volm/volm_spherical_container.h>
#include <volm/volm_spherical_container_sptr.h>
#include <volm/volm_spherical_shell_container.h>
#include <volm/volm_spherical_shell_container_sptr.h>
#include <boxm2/volm/boxm2_volm_matcher_p1.h>
#include <volm/volm_loc_hyp.h>
#include <bbas/volm/volm_io.h>
#include <bbas/volm/volm_tile.h>
#include <boxm2/volm/boxm2_volm_locations.h>
#include <boxm2/volm/boxm2_volm_locations_sptr.h>
#include <bbas/bocl/bocl_manager.h>
#include <bbas/bocl/bocl_device.h>
#include <vcl_set.h>
#include <vcl_iostream.h>
#include <volm/volm_geo_index.h>
#include <volm/volm_geo_index_sptr.h>
#include <vsol/vsol_polygon_2d.h>
#include <vsol/vsol_point_2d.h>
#include <vnl/vnl_random.h>
#include <volm/volm_category_io.h>

static void test_volm_matcher_p1()
{
  // create a volm_query
  // depth map scene for query
  depth_map_scene_sptr depth_scene = new depth_map_scene;
  unsigned ni = 1280;
  unsigned nj = 720;
  depth_scene->set_image_size(ni, nj);
  // add a sky object
  vsol_point_2d_sptr ps0 = new vsol_point_2d(0.0, 200.0);   vsol_point_2d_sptr ps1 = new vsol_point_2d(1280.0, 200.0);
  vsol_point_2d_sptr ps2 = new vsol_point_2d(1280.0, 0.0);  vsol_point_2d_sptr ps3 = new vsol_point_2d(0.0, 0.0);
  vcl_vector<vsol_point_2d_sptr> verts_sky;
  verts_sky.push_back(ps0);  verts_sky.push_back(ps1);  verts_sky.push_back(ps2);  verts_sky.push_back(ps3);  vsol_polygon_2d_sptr sp = new vsol_polygon_2d(verts_sky);
  depth_scene->add_sky(sp, 255, "sky");
  // add an object
  vsol_point_2d_sptr pb0 = new vsol_point_2d(0.000, 720.0);  vsol_point_2d_sptr pb1 = new vsol_point_2d(425.0, 500.0);
  vsol_point_2d_sptr pb2 = new vsol_point_2d(420.0, 200.0);  vsol_point_2d_sptr pb3 = new vsol_point_2d(0.000, 200.0);
  vcl_vector<vsol_point_2d_sptr> verts_bd;
  verts_bd.push_back(pb0);  verts_bd.push_back(pb1);  verts_bd.push_back(pb2);  verts_bd.push_back(pb3);
  vsol_polygon_2d_sptr bp = new vsol_polygon_2d(verts_bd);
  vgl_vector_3d<double> np(1.0, 1.0, 0.0);
  depth_scene->add_region(bp, np, 100.0, 1000.0, "hotel", depth_map_region::FRONT_PARALLEL, 1, 15);
  vcl_string dms_bin_file = "./depth_map_scene.bin";
  vsl_b_ofstream ofs_dms(dms_bin_file);
  depth_scene->b_write(ofs_dms);
  ofs_dms.close();

  // camera space for query
  double head_mid=180.0, head_radius=10.0, head_inc=5.0;
  double tilt_mid=90.0, tilt_radius=0.0,   tilt_inc=2.0;
  double roll_mid=0.0,  roll_radius=0.0,    roll_inc=0.0;
  //double top_fov_vals[] = {3.0,  4.0, 5.0, 12.0, 17.0, 18.0,19.0, 20.0, 24.0};
  double top_fov_vals[] = {20.0, 24.0};
  vcl_vector<double> fovs(top_fov_vals, top_fov_vals + 5);
  double altitude = 1.6;
  volm_camera_space_sptr csp = new volm_camera_space(fovs, altitude, ni, nj,
                                                     head_mid, head_radius, head_inc,
                                                     tilt_mid, tilt_radius, tilt_inc,
                                                     roll_mid, roll_radius, roll_inc);
  if (depth_scene->ground_plane().size() > 0)  // enforce ground plane constraint if user specified a ground plane
  {
    camera_space_iterator cit = csp->begin();
    for ( ; cit != csp->end(); ++cit) {
      unsigned current = csp->cam_index();
      vpgl_perspective_camera<double> cam = csp->camera(); // camera at current state of iterator
      bool success = true;
      for (unsigned i = 0; success && i < depth_scene->ground_plane().size(); i++)
        success = depth_scene->ground_plane()[i]->region_ground_2d_to_3d(cam);
      if (success) // add this camera
        csp->add_camera_index(current);
    }
  }
  else
    csp->generate_full_camera_index_space();

  // create depth interval
  float vmin = 2.0f;
  float dmax = 3000.0f;
  float solid_angle = 2.0f;
  volm_spherical_container_sptr sph = new volm_spherical_container(solid_angle, vmin, dmax);
  vcl_map<double, unsigned char>& depth_interval_map = sph->get_depth_interval_map();
  vcl_vector<float> depth_interval;
  vcl_map<double, unsigned char>::iterator iter = depth_interval_map.begin();
  for (; iter != depth_interval_map.end(); ++iter)
    depth_interval.push_back((float)iter->first);

  // create spherical shell container
  float cap_angle = 180;
  float point_angle = 5;
  double radius = 1;
  float top_angle = 70;
  float bottom_angle = 70;
  volm_spherical_shell_container_sptr sph_shell = new volm_spherical_shell_container(radius, cap_angle, point_angle, top_angle, bottom_angle);

  // create volm_query
  volm_query_sptr query = new volm_query(csp, dms_bin_file, sph_shell, sph);

  // create weight
  vcl_vector<volm_weight> weights;
  volm_weight::equal_weight(weights,depth_scene);


  // create location hypothesis and associate index
  volm_tile tile(0.0, 0.0, 'N', 'W', 1.0, 1.0, 3600, 3600);
  float min_size = 1.0f;
  volm_geo_index_node_sptr hyp_root = volm_geo_index::construct_tree(tile, (float)min_size);
  // write the geo index and the hyps
  vcl_string file_name = "./geo_index_tile_0.txt";
  unsigned depth = volm_geo_index::depth(hyp_root);
  vcl_stringstream file_kml;  file_kml << "./geo_index_tile_0_depth_" << depth << ".kml";
  vnl_random rng;
  for (unsigned i = 0; i < 100; i++)
    volm_geo_index::add_hypothesis(hyp_root, rng.drand32(), rng.drand32(), rng.drand32(100.0));
  // write out the hypos
  vcl_string file_name4 = "./geo_index_tile_0_";
  volm_geo_index::write_hyps(hyp_root, file_name4);
  vcl_vector<volm_geo_index_node_sptr> leaves;
  volm_geo_index::get_leaves_with_hyps(hyp_root, leaves);
  for (unsigned jj = 0; jj < leaves.size(); jj++) {
    vcl_string out_file = vul_file::strip_extension(leaves[jj]->get_hyp_name(file_name4)) + ".kml";
    leaves[jj]->hyps_->write_to_kml(out_file, 0.0, true);
  }
  // create index
  unsigned layer_size = (unsigned)sph_shell->get_container_size();
  float buffer_capacity = 1.0f;
  boxm2_volm_wr3db_index_params params;
  params.start = 0;    params.skip = 1;
  params.vmin = vmin;  params.dmax = dmax;  params.solid_angle = solid_angle;
  params.layer_size = layer_size;
  vcl_string out_params_file = "./geo_index_tile_0_index";
  TEST("write parameter file:", params.write_params_file(out_params_file), true);

  vcl_stringstream out_file_name_pre;
  out_file_name_pre << "./geo_index_tile_0";
  for (unsigned li = 0; li < leaves.size(); li++) {
    // create a binary index file for each hypo set in a leaf
    boxm2_volm_wr3db_index_sptr ind_dst = new boxm2_volm_wr3db_index(layer_size, buffer_capacity);
    boxm2_volm_wr3db_index_sptr ind_ori = new boxm2_volm_wr3db_index(layer_size, buffer_capacity);
    boxm2_volm_wr3db_index_sptr ind_lnd = new boxm2_volm_wr3db_index(layer_size, buffer_capacity);
    vcl_string ind_dst_file = leaves[li]->get_index_name(out_file_name_pre.str());
    vcl_string ind_ori_file = leaves[li]->get_label_index_name(out_file_name_pre.str(), "orientation");
    vcl_string ind_lnd_file = leaves[li]->get_label_index_name(out_file_name_pre.str(), "land");
    ind_dst->initialize_write(ind_dst_file);
    ind_ori->initialize_write(ind_ori_file);
    ind_lnd->initialize_write(ind_lnd_file);
    unsigned indexed_cnt = 0;
    for (unsigned h_id = 0; h_id < leaves[li]->hyps_->locs_.size(); h_id++) {
      vcl_vector<unsigned char> values(layer_size, 0);
      for (unsigned i = 0; i < values.size(); i++)
        values[i] = (unsigned char)(rng.drand32(1.0, (double)(sph->get_depth_offset_map().size()-1)));
      ind_dst->add_to_index(values);
      for (unsigned i = 0; i < values.size(); i++)
        values[i] = (unsigned char)(rng.drand32(1.0, (double)(10.0)));
      ind_ori->add_to_index(values);
      for (unsigned i = 0; i < values.size(); i++)
        values[i] = (unsigned char)(rng.drand32(1.0, (double)(volm_osm_category_io::volm_land_table.size())));
      ind_lnd->add_to_index(values);
      ++indexed_cnt;
    }
    ind_dst->finalize();  ind_ori->finalize();  ind_lnd->finalize();
    boxm2_volm_wr3db_index_params::write_size_file(ind_dst_file, indexed_cnt);
  }

  for (vcl_vector<volm_weight>::iterator vit = weights.begin(); vit != weights.end(); ++vit)
    vcl_cout << ' ' << vit->w_typ_
             << ' ' << vit->w_ori_
             << ' ' << vit->w_lnd_
             << ' ' << vit->w_dst_
             << ' ' << vit->w_ord_
             << ' ' << vit->w_obj_ << vcl_endl;

  // find the device that will be used
  bocl_manager_child &mgr = bocl_manager_child::instance();
  unsigned num_device = (unsigned)mgr.numGPUs();
  TEST("GPU device is available", num_device!=0, true);
  if (num_device == 0)
    return;

  // run matcher
  vcl_string geo_folder = "./";
  vcl_string out_folder = "./";
  double threshold = 0.2;
  unsigned max_cam_per_loc = 200;
  bool is_candidate = false;
  bool is_last_pass = false;
  vgl_polygon<double> cand_poly;
  boxm2_volm_matcher_p1 obj_ps1_matcher(csp, query, leaves, buffer_capacity, geo_folder, 0, depth_interval,
                                        cand_poly, mgr.gpus_[0], is_candidate, is_last_pass, out_folder, threshold,
                                        max_cam_per_loc, weights);

  TEST("volm pass 1 matcher execution", obj_ps1_matcher.volm_matcher_p1(), true);
  
  // write out
  vcl_string out_score_bin = "./ps_1_scores_tile_0.bin";
  TEST("volm pass 1 matcher result output", obj_ps1_matcher.write_matcher_result(out_score_bin), true);

}

#else // HAS_OPENCL
static void test_volm_matcher_p1() {}
#endif // HAS_OPENCL
TESTMAIN(test_volm_matcher_p1);
