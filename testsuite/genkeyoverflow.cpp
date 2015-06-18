/**
 * zerobytes.cpp - 
 * @author: Jonathan Beard
 * @version: Thu Jun 18 08:09:25 2015
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

int
main( int argc, char **argv )
{
   char key_buff[ 256 ];
   /** blow buffer, should fail **/
   shm::genkey( key_buff, 0x10000 );
   return( EXIT_SUCCESS );
}
