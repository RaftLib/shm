set( CMAKE_INCLUDE_CURRENT_DIR ON )

set( TESTAPPS   alloc
                doublealloc
                close
                cppstylealloc
                cppstyleopen
                outofrange
                wrongkey
                zerobytes
                zerokeylength 
                tworandomkeys 
                ${NUMA_TESTS}
                 )

include_directories( ${PROJECT_SOURCE_DIR}/include )

foreach( APP ${TESTAPPS} )
    add_executable( ${APP} "${APP}.cpp" )
    target_link_libraries( ${APP} shm ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_RT_LIB} ${CMAKE_NUMA_LIB} )
    add_test( NAME "${APP}_test" COMMAND ${APP} )
endforeach( APP ${TESTAPPS} )
