/**
 * alloc.cpp - 
 * @author: Jonathan Beard
 * @version: Thu Jun 18 08:08:03 2015
 * 
 * Copyright 2015 Jonathan Beard
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cstdint>
#include <iostream>
#include <shm>
#include <cstring>
#include <cassert>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


int
main( int argc, char **argv )
{
   shm_key_t key    = { shm_initial_key }; 
   shm::gen_key( key, 42);
   
   using type_t = std::uint32_t;


   type_t *ptr( nullptr );
  
#if (USE_CPP_EXCEPTIONS==1)   
   try
   {
      ptr = reinterpret_cast< type_t* >( shm::init( key, 0x1000, false, nullptr ) );
   }
   catch( bad_shm_alloc ex )
   {
      std::cerr << ex.what() << "\n";
      exit( EXIT_FAILURE );
   }
#else
   ptr = reinterpret_cast< type_t* >( shm::init( key, 0x1000, false, nullptr ) );
   if( ptr == (void*)-1 && ptr != nullptr ) 
   {
      std::fprintf( stderr, "Failed to allocate pointer\n" );
      exit( EXIT_FAILURE );
   }
#endif
   
    auto child = fork();
    switch( child )
    {
        case( 0 /** child **/ ):
        {   
#if (USE_CPP_EXCEPTIONS==1)   
            try
            {
                ptr = shm::eopen< type_t >( key );
            }
            catch( bad_shm_alloc ex )
            {
                std::cerr << ex.what() << "\n";
                exit( EXIT_FAILURE );
            }
#else
            ptr = shm::eopen< type_t >( key );
            if( ptr == (void*)-1 && ptr != nullptr )
            {
                std::fprintf( stderr, "Failed to open shm ptr\n" );
                exit( EXIT_FAILURE );
            }

#endif
            __atomic_store_n( ptr, 0x1137, __ATOMIC_RELEASE );

            shm::close( key, 
                        reinterpret_cast<void**>( &ptr), 
                        0x1000,
                        false /** don't zero   **/,
                        false /** don't unlink **/ );
        }
        break;
        case( -1 /** error, back to parent **/ ):
        {
            exit( EXIT_FAILURE );
        }
        break;
        default:
        {
            //spin on value being written from child
            type_t val = 0;
            while( val != 0x1137 )
            {
                __atomic_load( ptr, &val, __ATOMIC_RELAXED );
            } 
            std::fprintf( stdout, "leet\n" );
            int status = 0;
            waitpid( -1, &status, 0 );
            shm::close( key, 
                        reinterpret_cast<void**>( &ptr), 
                        0x1000,
                        true    /** zero   **/,
                        true    /** unlink **/ );
        }
    }
    
   return( EXIT_SUCCESS );
}
