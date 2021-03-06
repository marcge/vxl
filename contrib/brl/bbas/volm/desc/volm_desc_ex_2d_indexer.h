// This is brl/bbas/volm/desc/volm_desc_ex_2d_indexer.h
#ifndef volm_desc_ex_2d_indexer_h_
#define volm_desc_ex_2d_indexer_h_
//:
// \file
// \brief  A class to create a volm_desc_ex_land_only (existence) descriptor index using 2D classification maps of reference worlds at each location  
//         In the 2D case, the distances are given as a set of radii, e.g. 100, 500 and 1000. 
//         during indexing, anything further than 1000 m is completely ignored.
//                    so during query descriptor generator to match to this index, in the query image, anything that is labeled further than 1000 m should be ignored as well
//         similarly, in the 2D case, "sky" is not a valid land class to be considered for 'existence', so "sky" bin in the descriptor histogram will never be upcounted
//                    so during query descriptor generator, the "sky" bin should always be left empty even if sky is visible in the image
//
// \author  Ozge C. Ozcanli
// \date May 29, 2013
// \verbatim
//  Modifications
//   <none yet>
// \endverbatim
//


#include "volm_desc_indexer.h"
#include "volm_desc_ex_land_only.h"
#include <volm/volm_buffered_index.h>

#include <volm/volm_io_tools.h>
#include <volm/volm_category_io.h>

class volm_desc_ex_2d_indexer : public volm_desc_indexer
{
public:
  static vcl_string& get_name();

  volm_desc_ex_2d_indexer(vcl_string const& input_folder,
                          vcl_string const& out_index_folder, 
                          vcl_vector<double> const& radius,
                          unsigned const& nlands = volm_osm_category_io::volm_land_table.size());

  // handles any necessary loading during indexing when it switches processing from one leaf to the next leaf
  //  for the case of this class, find the set of volm_img_info that intersects the current leaf
  virtual bool get_next();

  virtual bool extract(double lat, double lon, double elev, vcl_vector<unsigned char>& values);

  //: each driving indexer should overwrite with the size of the descriptor
  virtual unsigned layer_size() { return layer_size_; }

  virtual vcl_string get_index_type_str() { return volm_desc_ex_2d_indexer::get_name(); }

  //: generate parameter files
  virtual bool write_params_file();

public:
  vcl_vector<volm_img_info> classification_maps_;  // unsigned char images with id of the land types, the id in the value of entries in volm_label_table
  
  unsigned nlands_;
  unsigned ndists_;
  vcl_vector<double> radius_;
  unsigned layer_size_;
  vcl_vector<unsigned> current_leaf_maps_;  // cash the maps of the current leaf
  double largest_rad_;
  double largest_rad_seconds_;

  vcl_map<unsigned, vil_image_view_base_sptr > lon_imgs;  // compute these for the classification maps faster access
  vcl_map<unsigned, vil_image_view_base_sptr > lat_imgs;
};

#endif  //volm_desc_ex_2d_indexer_h_
