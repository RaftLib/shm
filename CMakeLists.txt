cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(shm)
set( version 1.1 )
set( CMAKE_INCLUDE_CURRENT_DIR ON )



##
# at this point we just need a warning
##
if( CMAKE_HOST_UNIX )
find_library( NUMA_LIBRARY
              NAMES numa
              PATHS
              ${CMAKE_LIBRARY_PATH}
              $ENV{NUMA_PATH}/lib
              /usr/lib
              /usr/local/lib
              /opt/local/lib )
if( NUMA_LIBRARY )
    set( CMAKE_NUMA_LIBS ${NUMA_LIBRARY} )
else()
##
# get machine type
##
    execute_process( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )
    message( STATUS "found architecture: ${ARCHITECTURE}" )
endif()

endif( CMAKE_HOST_UNIX )


include_directories ( ${PROJECT_SOURCE_DIR} )
include_directories ( ${PROJECT_SOURCE_DIR}/include )

add_subdirectory( lib )
add_subdirectory( testsuite )

##
# enable minimal testsuite
##
set( TESTAPPS   alloc
                close
                cppstylealloc
                cppstyleopen
                outofrange
                pagemigrate
                wrongkey
                zerobytes
                zerokeylength )

enable_testing()
foreach( TEST ${TESTAPPS} )
 add_test( NAME "${TEST}_test" COMMAND ${TEST} )
endforeach( TEST ${TESTAPPS} )
