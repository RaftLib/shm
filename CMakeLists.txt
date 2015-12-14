cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(shm)
set( version 1.1 )
set( CMAKE_INCLUDE_CURRENT_DIR ON )


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
