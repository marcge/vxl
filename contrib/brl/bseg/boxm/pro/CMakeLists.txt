# boxm's Process Library

# bprb depends on expat and bvxm_prodepends on bprb primarily
INCLUDE( ${BRL_MODULE_PATH}/FindEXPAT.cmake )
IF(EXPAT_FOUND)
  INCLUDE_DIRECTORIES( ${BRL_INCLUDE_DIR}/bpro )
  INCLUDE_DIRECTORIES( ${BRL_INCLUDE_DIR}/bpro/bprb )
  INCLUDE_DIRECTORIES( ${BRL_INCLUDE_DIR}/bseg )
  INCLUDE_DIRECTORIES( ${GEL_INCLUDE_DIR}/mrc/vpgl )
  INCLUDE_DIRECTORIES( ${GEL_INCLUDE_DIR}/mrc )

  SET(boxm_pro_sources
      boxm_processes.h
      boxm_register.h   boxm_register.cxx
     )

  AUX_SOURCE_DIRECTORY(Templates boxm_pro_sources)
  AUX_SOURCE_DIRECTORY(processes boxm_pro_sources)

  ADD_LIBRARY(boxm_pro ${boxm_pro_sources})

  TARGET_LINK_LIBRARIES(boxm_pro boxm boxm_util boxm_sample_algo boxm_sample vpgl_pro imesh bprb brdb brip vsol bsta bsta_pro vnl_io vnl vil vgl vbl_io vul vbl)

  IF( BUILD_TESTING )
    SUBDIRS(tests)
  ENDIF( BUILD_TESTING )

ENDIF(EXPAT_FOUND)
