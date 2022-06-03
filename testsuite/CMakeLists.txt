set( CMAKE_INCLUDE_CURRENT_DIR ON )

if( CPP_EXCEPTIONS )
set( TESTAPPS   alloc
                doublealloc
                close
                cppstylealloc
                cppstyleopen
                outofrange
                wrongkey
                zerobytes
                two_process 
                ${NUMA_TESTS}
                 )
else()
##
# need to add idefs to all shm test cases
##
set( TESTAPPS   
                alloc
                two_process 
                ${NUMA_TESTS}
                 )
endif()
include_directories( ${PROJECT_SOURCE_DIR}/include )

foreach( APP ${TESTAPPS} )
    add_executable( ${APP} "${APP}.cpp" )
    target_link_libraries( ${APP} shm ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_RT_LIB} ${CMAKE_NUMA_LIB} )
    add_test( NAME "${APP}_test" COMMAND ${APP} )
endforeach( APP ${TESTAPPS} )
