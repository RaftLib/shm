/**
 * wrongkey.cpp - 
 * @author: Jonathan Beard
 * @version: Thu Jun 18 08:08:15 2015
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
#include <sstream>
#include <unistd.h>
#include <string>

int
main( int argc, char **argv )
{
   shm_key_t key;
   shm::gen_key( key, 42 );
   
   std::int32_t *ptr( nullptr );
   const auto nbytes( 0x1000 );
   try
   {
      ptr = reinterpret_cast< std::int32_t* >( shm::init( key, nbytes ) );
   }
   catch( bad_shm_alloc ex )
   {
      std::cerr << ex.what() << "\n";
      exit( EXIT_FAILURE );
   }
   /** if we get to this point then we assume that the mem is writable **/
   try
   {
      shm_key_t wrong_key;
      shm::gen_key( wrong_key, 42 );
      shm::close( wrong_key, 
                  reinterpret_cast<void**>( &ptr ), 
                  nbytes,
                  true,
                  true );
   }
   catch( invalid_key_exception ex )
   {
      /** real key so we don't manually cleanup **/
      shm::close( key, 
                  reinterpret_cast<void**>( &ptr), 
                  nbytes,
                  true,
                  true );
   }
   /**
    * if the exception catch above was executed then
    * the file handle shouldn't exist
    */
   //FIXME - set this up correctly for SYSTEMV as well,
   //this only works currently for POSIX. 
#if _USE_POSIX_SEM_ == 1
   std::stringstream ss;
   ss << "/dev/shm/" << key;
   if( access( ss.str().c_str(), F_OK ) != -1 )
   {
      std::cerr << "File exists!!\n";
      return( EXIT_FAILURE );
   }
#elif _USE_SYSTEMV_SEM_ == 1

#endif
   
   /** should get here and be done **/
   return( EXIT_SUCCESS );
}
