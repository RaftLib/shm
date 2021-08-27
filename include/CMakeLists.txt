##
# run header file through pre-processor so we can 
# set platform specific definitions before building
# and installing.
##
configure_file( "shm_module.hpp.in" "shm_module.hpp" @ONLY )
install( FILES ${PROJECT_SOURCE_DIR}/include/shm  
         DESTINATION ${CMAKE_INSTALL_PREFIX}/include )
install( FILES ${PROJECT_BINARY_DIR}/include/shm_module.hpp  
         DESTINATION ${CMAKE_INSTALL_PREFIX}/include )

