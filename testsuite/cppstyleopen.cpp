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

int
main( int argc, char **argv )
{
   shm_key_t key;
   shm::gen_key( key, 42 );
   std::cerr << key << "\n";
   std::int32_t *ptr( nullptr );
   try
   {
      ptr = shm::einit< std::int32_t >( key, 100 );
   }
   catch( bad_shm_alloc ex )
   {
      std::cerr << ex.what() << "\n";
      exit( EXIT_FAILURE );
   }
   for( int i( 0 ); i < 100; i++ )
   {  
      ptr[ i ] = i;
      std::cout << ptr[ i ] << "\n";
   }
   std::int32_t *ptr2( nullptr );
   try
   {
      ptr2 = shm::eopen< std::int32_t >( key );
   }
   catch( bad_shm_alloc ex )
   {
      std::cerr << ex.what() << "\n";
      exit( EXIT_FAILURE );
   }
   
   if(  std::memcmp( ptr,ptr2, 100 * sizeof( std::int32_t ) ) != 0 )
   {
      shm::close( key, 
                  reinterpret_cast<void**>( &ptr), 
                  0x1000,
                  true,
                  true );
      /** memory regions aren't equal, fail **/
      assert( false );
   }
   


   shm::close( key, 
               reinterpret_cast<void**>( &ptr), 
               0x1000,
               true,
               true );
   /** should get here and be done **/
   return( EXIT_SUCCESS );
}
