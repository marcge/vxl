#ifndef vcl_utility_txx_
#define vcl_utility_txx_
/*
  fsm@robots.ox.ac.uk
*/
#include "vcl_utility.h"

#undef VCL_PAIR_INSTANTIATE

#if !VCL_USE_NATIVE_STL
# include "emulation/vcl_utility.txx"
#elif defined(VCL_EGCS)
# include "egcs/vcl_utility.txx"
#elif defined(VCL_GCC_295) && !defined(GNU_LIBSTDCXX_V3)
# include "gcc-295/vcl_utility.txx"
#elif defined(GNU_LIBSTDCXX_V3)
# include "gcc-libstdcxx-v3/vcl_utility.txx"
#elif defined(VCL_SUNPRO_CC)
# include "sunpro/vcl_utility.txx"
#elif defined(VCL_SGI_CC)
# include "sgi/vcl_utility.txx"
#elif defined(VCL_WIN32)
# include "win32/vcl_utility.txx"
#else
# include "iso/vcl_utility.txx"
#endif

#endif
