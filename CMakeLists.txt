cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(shm)
set( version 1.1 )
set( CMAKE_INCLUDE_CURRENT_DIR ON )
##
# for submodules, this needs to be reset to avoid
# preferencing parent module paths.
##
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )
find_package( NUMA )
find_package( Threads )
find_package( LIBRT )
include( CheckSTD )




add_subdirectory( include )

include_directories ( ${PROJECT_BINARY_DIR}/include )
include_directories ( ${PROJECT_SOURCE_DIR}/include )

add_subdirectory( lib )



if( NOT WIN32 )
##
# make and setup pkg-config
##
mark_as_advanced( PKG_CONFIG_PATHWAY )
set( PKG_CONFIG_PATH "" CACHE STRING "Set the pkg-config path, othwerwise will figure out" )
if( NOT PKG_CONFIG_PATH )
execute_process( COMMAND  pkg-config --variable pc_path pkg-config 
                 COMMAND  tr ':' '\n' 
                 COMMAND  head -n 1
                 OUTPUT_VARIABLE LOCAL_PKG_CONFIG_PATHWAY )
string(REGEX REPLACE "\n$" "" LOCAL_PKG_CONFIG_PATHWAY "${LOCAL_PKG_CONFIG_PATHWAY}")
set( PKG_CONFIG_PATH ${LOCAL_PKG_CONFIG_PATHWAY} )
endif()

string(REPLACE ":" ";" PREFIX_LIST "${CMAKE_SYSTEM_PREFIX_PATH}")
list(FIND PREFIX_LIST ${CMAKE_INSTALL_PREFIX}  _index)

if(${_index} GREATER 0)
    file( MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/pkgconfig ) 
    set( PKG_CONFIG_PATH ${CMAKE_INSTALL_PREFIX}/pkgconfig )
    message( WARNING "You should set PKG_CONFIG_PATH=${CMAKE_INSTALL_PREFIX}/pkgconfig/:$PKG_CONFIG_PATH when installing to non-standard prefix for pkg-config to work correctly!" )
else()
    message( STATUS "Setting PKG_CONFIG_PATH to: ${PKG_CONFIG_PATH}" )
endif()



configure_file( "shm.pc.in" "shm.pc" @ONLY )
install( FILES ${PROJECT_BINARY_DIR}/shm.pc DESTINATION  ${PKG_CONFIG_PATH} )

##
# BUILD Tests
##
mark_as_advanced( BUILD_TESTS )
set( BUILD_TESTS trrue CACHE BOOL "Tests build target available if true" )
if( BUILD_TESTS )
    enable_testing()
    add_subdirectory( testsuite )
endif()
endif()
