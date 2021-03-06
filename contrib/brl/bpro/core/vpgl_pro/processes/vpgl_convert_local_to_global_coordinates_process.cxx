// This is brl/bpro/core/vpgl_pro/processes/vpgl_convert_local_to_global_coordinates_process.cxx
#include <bprb/bprb_func_process.h>
//:
// \file

#include <bprb/bprb_parameters.h>
#include <vcl_iostream.h>
#include <vpgl/vpgl_lvcs.h>
#include <vpgl/vpgl_lvcs_sptr.h>

//: initialization
bool vpgl_convert_local_to_global_coordinates_process_cons(bprb_func_process& pro)
{
  //this process takes four inputs: 
  // 0: (string) lvcs filename
  // 1: (double)  latitude
  // 2: (double)  longitude
  // 3: (double)  elevation
  bool ok=false;
  vcl_vector<vcl_string> input_types;
  input_types.push_back("vpgl_lvcs_sptr"); 
  input_types.push_back("double");
  input_types.push_back("double");
  input_types.push_back("double");
  ok = pro.set_input_types(input_types);
  if (!ok) return ok;

  // outputs x,y,z in local coordinates
  // 0: (double) x
  // 1: (double) y
  // 2: (double) z
  vcl_vector<vcl_string> output_types;
  output_types.push_back("double");  
  output_types.push_back("double");  
  output_types.push_back("double");  
  ok = pro.set_output_types(output_types);
  if (!ok) return ok;
  
  return true;

}

//: Execute the process
bool vpgl_convert_local_to_global_coordinates_process(bprb_func_process& pro)
{
  if (pro.n_inputs() != pro.input_types().size()) {
    vcl_cout << "vpgl_convert_local_to_global_coordinates_process: The number of inputs should be " << pro.input_types().size() << vcl_endl;
    return false;
  }

  // get the inputs
  vpgl_lvcs_sptr lvcs = pro.get_input<vpgl_lvcs_sptr>(0);
  double x = pro.get_input<double>(1);
  double y = pro.get_input<double>(2);
  double z = pro.get_input<double>(3);

 
  double lat,lon,el;
  lvcs->local_to_global(x,y,z,vpgl_lvcs::wgs84,lon,lat,el,vpgl_lvcs::DEG, vpgl_lvcs::METERS);

  pro.set_output_val<double>(0, (double)lat);
  pro.set_output_val<double>(1, (double)lon);
  pro.set_output_val<double>(2, (double)el);

  return true;
}
