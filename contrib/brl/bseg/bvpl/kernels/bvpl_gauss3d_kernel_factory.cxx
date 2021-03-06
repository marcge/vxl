#include "bvpl_gauss3d_kernel_factory.h"
//:
// \file

#include <vcl_algorithm.h>
#include <vcl_cmath.h> // for std::sqrt() and std::exp() and std::floor()
#include <vnl/vnl_math.h>
#include <vnl/vnl_float_3.h>
#include <bsta/bsta_gauss_if3.h>

// Default Constructor
bvpl_gauss3d_kernel_factory::bvpl_gauss3d_kernel_factory()
{
  sigma1_ = 0.0f;
  sigma2_ = 0.0f;
  sigma3_ = 0.0f;
  angular_resolution_ = 0;
  rotation_axis_ = canonical_rotation_axis_;
  angle_ = 0.0f;
}

//: Constructs a kernel form gaussian spheroid with sigma parameters s1 and s2. i.e. Cov is diagonal with entries s1, s2, s2
bvpl_gauss3d_kernel_factory::bvpl_gauss3d_kernel_factory(float s1, float s2)
{
  //set variances of kernel
  sigma1_ = s1;
  sigma2_ = s2;
  sigma3_ = s2;

  //this skernel is symmetric around main axis
  angular_resolution_=0;

  //initialize variables
  angle_ = 0.0f;
  rotation_axis_ = canonical_rotation_axis_;
  parallel_axis_ = canonical_parallel_axis_;

  //create the default kernel
  create_canonical();
}

//: Constructs a kernel form gaussian ellipsoid with sigma parameters s1, s2 and s3. i.e. Cov is diagonal with entries s1, s2, s3
bvpl_gauss3d_kernel_factory::bvpl_gauss3d_kernel_factory(float s1, float s2, float s3,float supp1 , float supp2 , float supp3 )
{
  //set variances of kernel
  sigma1_ = s1;
  sigma2_ = s2;
  sigma3_ = s3;
  supp1_ = supp1;
  supp2_ = supp2;
  supp3_ = supp3;

  //this value is a meant as a limit there is not theoretical meaning to it
  angular_resolution_= float(vnl_math::pi/16.0);

  //initialize variables
  angle_ = 0.0f;
  rotation_axis_ = canonical_rotation_axis_;
  parallel_axis_ = canonical_parallel_axis_;


  //create the default kernel
  create_canonical();
}

void bvpl_gauss3d_kernel_factory::create_canonical()
{
  bsta_gauss_if3 gauss_kernel(vnl_float_3(0,0,0), vnl_float_3(sigma1_*sigma1_, sigma2_*sigma2_, sigma3_*sigma3_));

  typedef vgl_point_3d<float> point_3d;
  typedef bvpl_kernel_dispatch dispatch;

  //This is the support of the kernel
  float min_x = -vcl_floor(supp1_*sigma1_+0.01f);
  float max_x =  vcl_floor(supp1_*sigma1_+0.01f);
  float min_y = -vcl_floor(supp2_*sigma2_+0.01f);
  float max_y =  vcl_floor(supp2_*sigma2_+0.01f);
  float min_z = -vcl_floor(supp3_*sigma3_+0.01f);
  float max_z =  vcl_floor(supp3_*sigma3_+0.01f);
  float l1_norm = 0.0f;

  for (float x=min_x; x<= max_x; x+=1.f)
  {
    for (float y= min_y; y<= max_y; y+=1.f)
    {
      for (float z= min_z; z<= max_z; z+=1.f)
      {
        vnl_float_3 pt(x,y,z);
        float val = gauss_kernel.prob_density(pt);
        canonical_kernel_.push_back(vcl_pair<point_3d,dispatch>(point_3d(x,y,z), dispatch(val)));
        l1_norm +=val;
      }
    }
  }

  //normalize to L1 norm
  vcl_vector<vcl_pair<vgl_point_3d<float>, bvpl_kernel_dispatch> >::iterator k_it = canonical_kernel_.begin();
  float norm = 0.0;
  for(; k_it != canonical_kernel_.end(); k_it++)
  {
    k_it->second.c_ = k_it->second.c_ / l1_norm;
    norm+=k_it->second.c_;
  }
  vcl_cout << "Canonical kernel should have l1_norm, norm= " << norm <<vcl_endl;

   //set the dimension of the 3-d grid
  max_point_.set(int(max_x),int(max_y),int(max_z));
  min_point_.set(int(min_x),int(min_y),int(min_z));

  //set the current kernel
  kernel_ = canonical_kernel_;
  factory_name_ = name();
}
