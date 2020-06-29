##
# start check for RT libs
# var CMAKE_RT_LIBS will default to "" on non-unix platforms
##
if( NOT CMAKE_HOST_UNIX OR WIN32 )
    set( CMAKE_RT_LIBS "" )
else()
find_library( RT_LIBRARY
              NAMES rt
              PATHS
              ${CMAKE_LIBRARY_PATH}
              $ENV{RT_PATH}/lib
              /usr/lib
              /usr/local/lib
              /opt/local/lib )
if( RT_LIBRARY )
    get_filename_component( RT_LIBRARY ${RT_LIBRARY} DIRECTORY )
    set( CMAKE_RT_LDFLAGS "-L${RT_LIBRARY}" )
    set( CMAKE_RT_LIB "-lrt" )
else()
    set( CMAKE_RT_LIB "" )
endif()
mark_as_advanced( RT_LIBRARY CMAKE_RT_LIBS )
endif()
## end check for RT libs
