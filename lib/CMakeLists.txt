set( CMAKE_INCLUDE_CURRENT_DIR ON )


add_library( shm shm.cpp )
install( TARGETS shm
         ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
