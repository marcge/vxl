#ifndef boxm_render_ocl_scene_manager_h_
#define boxm_render_ocl_scene_manager_h_
//:
// \file
#include <vcl_string.h>
#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vnl/vnl_vector_fixed.h>
#include <vbl/vbl_array_2d.h>
#include <bocl/bocl_manager.h>
#include <bocl/bocl_utils.h>
#include <boxm/ocl/boxm_ocl_scene.h>

#include <vil/vil_image_view.h>
#include <vpgl/vpgl_perspective_camera.h>
#include <vul/vul_file_iterator.h>

class boxm_render_ocl_scene_manager : public bocl_manager<boxm_render_ocl_scene_manager >
{
 public:


  typedef float obs_type;

  boxm_render_ocl_scene_manager() :
    block_ptrs_(0),
    scene_dims_(0),
    block_dims_(0),
    scene_x_(0),scene_y_(0),scene_z_(0),
    cells_(0),
    cell_data_(0),
    cell_alpha_(0),
    root_level_(0),
    img_dims_(0),
    offset_y_(0),
    offset_x_(0),
    bni_(1),bnj_(1),
    wni_(1),wnj_(1),
    cells_size_(0),
    cell_data_size_(0),
    output_img_(),
    program_(0) {}
  ~boxm_render_ocl_scene_manager() {
    if (program_)
      clReleaseProgram(program_);
  }

  // read the scene, cam and image
  bool init_ray_trace(boxm_ocl_scene *scene,
                      vpgl_camera_double_sptr cam,
                      vil_image_view<obs_type> &obs);
  //: 2d workgroup
  void set_bundle_ni(unsigned bundle_x) {bni_=bundle_x;}
  void set_bundle_nj(unsigned bundle_y) {bnj_=bundle_y;}

  //: run update
  bool run_scene();
  bool set_args();
  bool set_kernel();
  bool release_kernel();
  bool set_commandqueue();
  bool release_commandqueue();
  bool set_workspace();
  //: set  root level
  bool set_scene_data();
  bool set_scene_data_buffers();
  bool release_scene_data_buffers();
  bool clean_scene_data();

  //: set input image and image dimensions and camera
  bool set_input_view();
  bool set_input_view_buffers();
  bool release_input_view_buffers();
  bool clean_input_view();

  //: set the tree, data , aux_data and bbox
  bool set_tree_buffers();
  bool release_tree_buffers();
  bool clean_tree();

  //: load all blocks in an array and store the tree pointers in block_ptrs;
  bool set_all_blocks();

  unsigned wni() {return wni_;}
  unsigned wnj() {return wnj_;}

  bool read_output_image();
  bool read_trees();
  void print_tree();
  void print_image();

  //: cleanup
  bool clean_update();

  cl_float * output_image() {return image_;}
  // image
  cl_float * image_;
  cl_uint * image_gl_;
  cl_mem   image_buf_;
  cl_mem   image_gl_buf_;


  //: helper functions
  bool run();

  // Set up Scene
  bool set_scene_dims();
  bool set_scene_dims_buffers();
  bool set_scene_origin();
  bool set_scene_origin_buffers();
  bool set_block_dims();
  bool set_block_dims_buffers();
  bool set_block_ptrs();
  bool set_block_ptrs_buffers();

  bool clean_scene_dims();
  bool release_scene_dims_buffers();
  bool clean_scene_origin();
  bool release_scene_origin_buffers();
  bool clean_block_dims();
  bool release_block_dims_buffers();
  bool clean_block_ptrs();
  bool release_block_ptrs_buffers();


  // Set up Camera
  bool set_persp_camera();
  bool set_persp_camera(vpgl_perspective_camera<double> * pcam);

  bool set_persp_camera_buffers();
  bool write_persp_camera_buffers();
  bool release_persp_camera_buffers();
  bool clean_persp_camera();

  // Set up input image
  bool set_input_image();
  bool set_input_image_buffers();
  bool set_image_dims_buffers();
  bool release_input_image_buffers();
  bool clean_input_image();
  // bool set_root_level
  bool set_root_level();
  bool set_root_level_buffers();
  bool release_root_level_buffers();
  bool clean_root_level();

  bool set_offset_buffers(int off_x, int off_y);
  bool release_offset_buffers();
  //open cl side helper functions
  int build_kernel_program(cl_program & program);
  cl_kernel kernel() {return kernel_;}

  //necessary CL items
  // for pass0 to compute seg len
  cl_program program_;

  cl_command_queue command_queue_;
  cl_kernel kernel_;

 protected:

  // scene information
  cl_int * block_ptrs_;
  // (x,y,z,0)
  cl_int * scene_dims_;
  cl_float * scene_origin_;
  // (x,y,z,0)
  cl_float * block_dims_;
  //array of tree cells,
  cl_int* cells_;
  cl_uint  cells_size_;

  //array of data pointed to by tree
  cl_float* cell_data_;
  cl_float* cell_alpha_;
  //cl_half* cell_data_;
  cl_uint  cell_data_size_;


  //root level
  cl_uint root_level_;

  // image dimensions
  cl_uint* img_dims_;

  //offset for non-overlapping sections
  cl_uint  offset_x_;
  cl_uint  offset_y_;
  // bounding box for each tree
  cl_float * tree_bbox_;


  // camera

  cl_float * persp_cam_;

  cl_uint bni_;
  cl_uint bnj_;

  cl_int scene_x_;
  cl_int scene_y_;
  cl_int scene_z_;

  // workspace dimensions which will be
  // greater than or equal to image dimensions

  cl_uint wni_;
  cl_uint wnj_;
  // pointer to cl memory on GPU
  cl_mem   cells_buf_;
  cl_mem   cell_data_buf_;
  cl_mem   cell_alpha_buf_;
  cl_mem   tree_bbox_buf_;

  cl_mem   persp_cam_buf_;
  cl_mem   img_dims_buf_;

  cl_mem   scene_orig_buf_;

  cl_mem   root_level_buf_;

  cl_mem   scene_dims_buf_;
  cl_mem   scene_origin_buf_;
  cl_mem   block_ptrs_buf_;
  cl_mem   block_dims_buf_;

  boxm_ocl_scene * scene_;
  vpgl_camera_double_sptr cam_;
  vil_image_view<float>  output_img_;

  vcl_size_t globalThreads[2];
  vcl_size_t localThreads[2] ;
};

#endif // boxm_render_ocl_scene_manager_h_
