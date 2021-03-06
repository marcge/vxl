#ifndef boxm2_vecf_ocl_landmark_warp_h_included_
#define boxm2_vecf_ocl_landmark_warp_h_included_

#include <bocl/bocl_cl.h>
#include <vcl_vector.h>
#include <vgl/vgl_point_3d.h>
#include <boxm2/boxm2_scene.h>
#include <boxm2/ocl/boxm2_opencl_cache.h>
#include <boxm2/vecf/boxm2_vecf_landmark_warp.h>

#include "boxm2_vecf_ocl_vector_field_adaptor.h"

class boxm2_vecf_ocl_landmark_warp : public boxm2_vecf_ocl_vector_field_adaptor
{
  public:
    boxm2_vecf_ocl_landmark_warp(vcl_vector<vgl_point_3d<double> > const& control_pts_source,
                                 vcl_vector<vgl_point_3d<double> > const& control_pts_target);


};

#endif
