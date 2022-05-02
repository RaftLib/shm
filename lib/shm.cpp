/*
 * shm.cpp - 
 * Copyright 2021 Jonathan Beard
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
 *
 * @author: Jonathan Beard
 * @version: September 7 2021
 */
#include <shm>
#include <fcntl.h>
/**
 * this is needed for mprotect on both the POSIX
 * and SystemV implementations 
 */
#include <sys/mman.h>

#if( _USE_SYSTEMV_SHM_ == 1)
#include <sys/shm.h>
#include <sys/ipc.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <sched.h>
#include <stdlib.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <cstdint>
#include <cinttypes>
#include <iostream>
#include <sstream>
#include <cmath>
#include <climits>
#include <limits>
#include <random>
#include <functional>

#if __APPLE__
#include <malloc/malloc.h>
#endif

#if __linux
/** might need to install numactl-dev **/
#include <sys/sysinfo.h>
#include <malloc.h>

#if PLATFORM_HAS_NUMA == 1
#include <numaif.h>
#include <numa.h>
#endif 

#endif


#ifndef UNUSED 
#ifdef __clang__
#define UNUSED( x ) (void)(x)
#else
#define UNUSED( x )[&x]{}()
#endif
//FIXME need to double check to see IF THIS WORKS ON MSVC
#endif

#if USE_CPP_EXCEPTIONS==1
SHMException::SHMException( const std::string &message ) : std::exception(),
                                                           message( message )
{
}

SHMException::SHMException( const std::string &&message ) : SHMException( message )
{
}

const char* 
SHMException::what() const noexcept
{
    return( message.c_str() );
};
#endif


void 
shm::gen_key( shm_key_t &key, const int proj_id )
{
#if _USE_POSIX_SHM_ == 1
    //string key
    UNUSED( proj_id );
    static std::random_device rd;
    static std::mt19937 gen( rd() );
    static std::uniform_int_distribution<> distrib( 0, std::numeric_limits< int >::max() );
    const auto val = distrib( gen );
    shm_key_t val_key;
    std::memset(    val_key, 
                    '\0', 
                    shm_key_length );

    std::snprintf( val_key, 
                   shm_key_length,
                   "%d",
                   val );

    shm::key_copy( key, val_key );
    return;
#elif _USE_SYSTEMV_SHM_ == 1
    //integer key
    char *path = getcwd( nullptr, 0 );
    if( path == nullptr )
    {
        std::perror( "failed to get cwd, switching to guns, a.k.a. root dir (/)" );
        key = ftok( "/", proj_id);
    }
    else
    {
        key = ftok( path, proj_id);
    }
    return;
#endif
}


bool      
shm::key_copy( shm_key_t               &dst_key,
               const   shm_key_t        src_key )
{
#if _USE_POSIX_SHM_ == 1
    //string key
    std::memset( dst_key, '\0', shm_key_length );
    std::strncpy(   (char*) dst_key,
                    src_key,
                    shm_key_length );
    return( true );                    
#elif _USE_SYSTEMV_SHM_ == 1
    dst_key = src_key;
    return( true );
#else
    //not implemented
    return( false );
#endif
}

void*
shm::init( const shm_key_t     &key,
           const std::size_t   nbytes,
           const bool zero   /* zero mem */,
           void   *ptr )
{
    auto handle_open_failure = [&]( const shm_key_t &key ) -> void*
    {
#if USE_CPP_EXCEPTIONS==1      
        std::stringstream ss;
#endif
        if( errno == EEXIST )
        {
#if USE_CPP_EXCEPTIONS==1      
            ss << "SHM Handle already exists \"" << key << "\" already exists, please use open\n";
            throw shm_already_exists( ss.str() );
#else            
            return( (void*)-1 );
#endif            
        }
        else 
        {
#if USE_CPP_EXCEPTIONS==1      
            ss << "Failed to open shm with file descriptor \"" << 
               key << "\", error code returned: ";
            ss << std::strerror( errno );
            throw bad_shm_alloc( ss.str() ); 
#else
            return( nullptr );
#endif
        }
    };

    if( nbytes == 0 )
    {
#if USE_CPP_EXCEPTIONS==1      
       throw bad_shm_alloc( "nbytes cannot be zero when allocating memory!" );
#else
        return( nullptr );
#endif
    }
    
    /** 
     * NOTE: 
     * - actual allocation size should be alloc_bytes,
     * user has no idea so we'll re-calc this at the  end
     * when we unmap the data.
     * - This is the same out that is returned at the end
     * for ease of keeping track of code between the two
     * different versions. 
     */
    void *out( nullptr );
    

    /**
     * NOTE: this is largely for truncation purposes,
     * but this is also needed as a sanity check and 
     * some off the values are needed elsewhere so let's
     * do this before we go into POSIX/SystemV specific 
     * code. 
     */
    const auto num_phys_pages( sysconf( _SC_PHYS_PAGES ) );
    const auto page_size( sysconf( _SC_PAGE_SIZE ) );
    const auto total_possible_bytes( num_phys_pages * page_size );
    if( nbytes > total_possible_bytes )
    {
    
#if USE_CPP_EXCEPTIONS==1      
         std::stringstream errstr;
         errstr << "You've tried to allocate too many bytes (" << nbytes << "),"
             << " the total possible is (" << total_possible_bytes << ")\n";
         throw bad_shm_alloc( errstr.str() ); 
#else
         //errno should be set (hopefully)
         return( nullptr );
#endif
    }
    /** get allocations size including extra dummy page **/
    const auto alloc_bytes( 
       static_cast< std::size_t >( 
          std::ceil(  
             static_cast< float >( nbytes) / 
            static_cast< float >( page_size ) ) + 1 ) * page_size 
    );
    
#if _USE_POSIX_SHM_ == 1
    int fd( shm::failure  );
    //stupid hack to get around platforms that are
    //using LD_PRELOAD, e.g., dynamic binary tools
    struct stat st;
    std::stringstream path;
    path << "/dev/shm/" << key;
    if( stat( path.str().c_str(), &st ) == 0 )
    {
#if USE_CPP_EXCEPTIONS==1      
        std::stringstream ss;
#endif
#if USE_CPP_EXCEPTIONS==1      
            ss << "SHM Handle already exists \"" << key << "\" already exists, please use open\n";
            throw shm_already_exists( ss.str() );
#else            
            return( (void*)-1 );
#endif            
    }
    
    /* set read/write set create if not exists */
    const std::int32_t flags( O_RDWR | O_CREAT | O_EXCL );
    /* set read/write by user */
    const mode_t mode( S_IWUSR | S_IRUSR );
    fd  = shm_open( key, 
                    flags, 
                    mode );
    if( fd == failure )
    {
        //if using exceptions you won't return
        return( handle_open_failure( key ) );
    }
    
    if( ftruncate( fd, alloc_bytes ) != shm::success )
    {
#if USE_CPP_EXCEPTIONS==1      
       std::stringstream ss;
       ss << "Failed to truncate shm for file descriptor (" << fd << ") ";
       ss << "with number of bytes (" << nbytes << ").  Error code returned: ";
       ss << std::strerror( errno );
       shm_unlink( key );
       throw bad_shm_alloc( ss.str() );
#else
       shm_unlink( key );
       return( nullptr );
#endif
    }

    /** 
     * NOTE: might be useful to change page size to something larger than default
     * for some applications. We'll add that as a future option, however, for now
     * I'll leave the note here.
     * flags = MAP_HUGETLB | MAP_ANONYMOUS | MAP_HUGE_2MB
     * flags = MAP_HUGETLB | MAP_ANONYMOUS | MAP_HUGE_1GB
     * we'll need to make sure huge pages are installed/enabled first
     * for ubuntu + apt:
     * apt-get install hugepages
     * you'll need to set it up, some good info if you don't know what you're 
     * doing is here: https://kerneltalks.com/services/what-is-huge-pages-in-linux/
     * you'll likely need to reboot to clear out any funky kernel states, once you're done,
     * write a test program and make sure that you're allocating, the command:
     * hugeadm --explain 
     * should tell you what's set up and in use.
     */
    out = mmap( ptr, 
                alloc_bytes, 
                ( PROT_READ | PROT_WRITE ), 
                MAP_SHARED, 
                fd, 
                0 );
    if( out == MAP_FAILED )
    {
#if USE_CPP_EXCEPTIONS==1      
       std::stringstream ss;
       ss << "Failed to mmap shm region with the following error: " << 
         std::strerror( errno ) << ",\n" << "unlinking.";
       shm_unlink( key );
       throw bad_shm_alloc( ss.str() );
#else
       shm_unlink( key );
       return( nullptr );
#endif
    }
/**
 * ###### END POSIX SECTION ######
 */
#elif _USE_SYSTEMV_SHM_ == 1
/**
 * ###### START SYSTEM-V SECTION  ######
 */
    /** first time you try to open it **/
    const auto shmid = 
        shmget( key, alloc_bytes, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR );
    if( shmid == shm::failure ) 
    {
        //if using exceptions, you won't return from this
        return( handle_open_failure( key ) );
    }

    out  = shmat( shmid, nullptr, 0 );
    if( out == (void*)-1 )
    {
#if USE_CPP_EXCEPTIONS==1      
       std::stringstream ss;
       ss << "Failed to mmap shm region with the following error: " << 
         std::strerror( errno ) << ",\n" << "unlinking.";
       throw bad_shm_alloc( ss.str() );
#else
       return( nullptr );
#endif
    }


#endif /** end switch between posix/sysV **/    
    if( zero )
    {
       /* everything theoretically went well, lets initialize to zero */
       std::memset( out, 0x0, nbytes );
    }
    char *temp( reinterpret_cast< char* >( out ) );
    if( mprotect( (void*) &temp[ alloc_bytes - page_size ],
                   page_size, 
                   PROT_NONE ) != 0 )
    {
#if DEBUG   
      perror( "Error, failed to set page protection, not fatal just dangerous." );
#endif      
   }
   return( out );
}



void*
shm::open( const shm_key_t &key )
{
   /**
    * just like with init, use same output
    * pointer stack location for both. 
    */
   void *out( nullptr );
    
    auto handle_open_failure = [&]( const shm_key_t &key ) -> void*
    {
#if USE_CPP_EXCEPTIONS==1      
        std::stringstream ss;
        ss << 
            "Failed to open shm with key \"" << key << "\", with the following error code (";
        ss << std::strerror( errno ) << ")"; 
        throw bad_shm_alloc( ss.str() );
#else
        return( nullptr );
#endif

    };
#if _USE_POSIX_SHM_ == 1
   int fd( shm::failure );
   const int flags( O_RDWR | O_CREAT );
   mode_t mode( 0 );
   fd = shm_open( key, 
                  flags, 
                  mode ); 
   if( fd == failure )
   {
        return( handle_open_failure( key ) );
   }
   struct stat st;
   std::memset( &st, 
                0x0, 
                sizeof( struct stat ) );
   /* stat the file to get the size */
   if( fstat( fd, &st ) != shm::success )
   {
#if USE_CPP_EXCEPTIONS==1      
      std::stringstream ss;
      ss << "Failed to stat shm region with the following error: " << std::strerror( errno ) << ",\n";
      ss << "unlinking.";
      shm_unlink( key );
      throw bad_shm_alloc( ss.str() );
#else
      shm_unlink( key );
      return( nullptr );
#endif
   }
   out = mmap( nullptr, 
               st.st_size, 
               (PROT_READ | PROT_WRITE), 
               MAP_SHARED, 
               fd, 
               0 );
   if( out == MAP_FAILED )
   {
#if USE_CPP_EXCEPTIONS==1      
      std::stringstream ss;
      ss << "Failed to mmap shm region with the following error: " << std::strerror( errno ) << ",\n";
      ss << "unlinking.";
      shm_unlink( key );
      throw bad_shm_alloc( ss.str() );
#else
      shm_unlink( key );
      return( nullptr );
#endif
   }
   /* close fd */
   ::close( fd );
   /* done, return mem */
   return( out );

/** END POSIX MEMORY **/
#elif _USE_SYSTEMV_SHM_ == 1
/** START SYSTEMV MEMORY **/

//STEP1 shmget
    const auto shmid = 
        shmget( key, sizeof(int), S_IRUSR | S_IWUSR );
    if( shmid == shm::failure ) 
    {
        //if using exceptions, you won't return from this
        return( handle_open_failure( key ) );
    }
//STEP2 shmat
    out = shmat(shmid, nullptr, 0);
    if( out == (void*)-1 ) 
    {
#if USE_CPP_EXCEPTIONS==1      
        std::stringstream ss;
        ss << "Failed to SHM attach the shm region with the following error (" 
            <<  std::strerror( errno ) << ").";
        throw bad_shm_alloc( ss.str() );
#else
        return( nullptr );
#endif
    }
/** END SYSTEMV MEMORY **/
#endif
    //if we're here, everything theoretically worked
    return( out );
}

bool
shm::close( const shm_key_t &key,
            void **ptr,
            const std::size_t nbytes,
            const bool zero,
            const bool unlink )
{
   if( zero && (ptr != nullptr) && ( *ptr != nullptr ) )
   {
      std::memset( *ptr, 0x0, nbytes );
   }
#if _USE_POSIX_SHM_ == 1
   if( ptr != nullptr )
   {
      /** get allocations size including extra dummy page **/
      const auto page_size( sysconf( _SC_PAGESIZE ) );
      const auto alloc_bytes( 
         static_cast< std::size_t >( 
            std::ceil(  
               static_cast< float >( nbytes ) / 
              static_cast< float >( page_size ) ) + 1 ) * page_size 
      );
      if( ( *ptr != nullptr ) && ( munmap( *ptr, alloc_bytes ) != shm::success ) )
      {
#if DEBUG   
         perror( "Failed to unmap shared memory, attempting to close!!" );
#endif
      }
      *ptr = nullptr;
   }
   if( unlink )
   {
      if( shm_unlink( key ) != 0 )
      {
#if USE_CPP_EXCEPTIONS==1      
         switch( errno )
         {
            case( ENOENT ):
            {
               throw invalid_key_exception( "File descriptor to unlink does not exist!" );
            }
            default:
            {
                throw invalid_key_exception( "Undefined error, check error codes" );
            }
         }
#endif         
      }
   }
   //jump to return true if here. 
/** END POSIX MEMORY IMPL **/
#elif _USE_SYSTEMV_SHM_ == 1
/** START SYSTEMV IMPL **/
    /**
     * we could have gotten here b/c something failed and the 
     * user code is now calling close on an invalid shm seg. 
     * so let's stat first to be sure. 
     */
     const auto shmid = 
         shmget( key, sizeof(int), S_IRUSR | S_IWUSR );
     if( shmid == shm::failure ) 
     {
        /**
         * could check if( errno == ENOENT )
         * but not too much point here given
         * what we want to close doesn't exist. 
         */
         return( true );
     }
    /**
     * NOTE: This may not work quite perfectly b/c 
     * if there are M communicating pairs and you 
     * call this and only one happens to be detached,
     * then this may cause the segment to be deleted.
     */
    if(shmctl(shmid, IPC_RMID, nullptr) == -1) {
#if USE_CPP_EXCEPTIONS==1      
       std::stringstream ss;
       ss << "Failed to set the SystemV memory region to exit on detach, non-fatal error (" 
         << std::strerror( errno ) << ")\n";
       throw bad_shm_alloc( ss.str() );
#else
       return( out );
#endif
    }
     //else, it exists
     if( shmdt( *ptr ) == shm::failure ) 
     {
#if USE_CPP_EXCEPTIONS==1
        std::stringstream ss;
        ss << "Failed to detach SHM with error code (" 
            <<  std::strerror( errno ) << ").";
        throw invalid_key_exception( ss.str() );
#else
        return( nullptr );
#endif
     }

/** END SYSTEMV IMPL **/
#endif
    return( true );
}

bool
shm::move_to_tid_numa( const pid_t thread_id,
                       void *ptr,
                       const std::size_t nbytes )
{
#if __linux && ( PLATFORM_HAS_NUMA == 1 )
   /** check alignment of pages first **/
   const auto page_size( sysconf( _SC_PAGESIZE ) );
   
   const auto ptr_addr( 
      reinterpret_cast< std::uintptr_t >( ptr ) );
   if( (ptr_addr % page_size) != 0 )
   {
#if USE_CPP_EXCEPTIONS==1      
      std::stringstream ss;
      ss << "Variable 'ptr' must be page aligned, currently it is(" << 
       ptr_addr % page_size << "), off please fix...exiting!!\n";
      throw page_alignment_exception( ss.str() );
#else
      return( false );
#endif
   }

   /** check to see if NUMA avail **/
   if( numa_available() == -1 )
   {
      return( true );
   }
   /** first check to see if there is more than one numa node **/
   const auto num_numa( numa_num_configured_nodes() );
   if( num_numa == 1 )
   {
      /** no point in continuing **/
      return( true );
   }

   /** RETURN VAL HERE */
   bool moved( false );
   /** get the numa node of the calling thread **/
   const auto local_node( numa_node_of_cpu( sched_getcpu() ) );      
   /** get numa node that 'pages' is on **/
   /**
    * Note: only way I could figure out how to do this was get a cpu
    * set, we'll check to see how many CPU's are in the CPU set of 
    * the thread, then we'll see if all those CPU's are on the 
    * same NUMA node, if they are then we'll check to see if the
    * alloc is on the same node, otherwise we'll move the pages to that
    * node.  The interesting case comes when there are multiple
    * CPUs per set and multiple possible NUMA nodes to choose from.  The simple
    * answer, the one I'm taking a the moment is that we can't 
    * really decide in this function.  I think in the future it'd
    * be good to have a profiler decide the shortest latency and
    * highest bandwidth for long running programs at startup and 
    * do some sort of graph matching.
    */
   cpu_set_t *cpuset( nullptr );
   /** we want set to include all cpus **/
   const auto num_cpus_alloc( get_nprocs_conf() );
   std::size_t cpu_allocate_size( -1 );
#if (__GLIBC_MINOR__ > 9 ) && (__GLIBC__ == 2)
   cpuset = CPU_ALLOC( num_cpus_alloc );
   assert( cpuset != nullptr );
   cpu_allocate_size = CPU_ALLOC_SIZE( num_cpus_alloc );
   CPU_ZERO_S( cpu_allocate_size, cpuset );
#else
   cpu_allocate_size = sizeof( cpu_set_t );
   cpuset = reinterpret_cast< cpu_set_t* >( malloc( cpu_allocate_size ) );
   assert( cpuset != nullptr );
   CPU_ZERO( cpuset );
#endif
   /** now get affinity **/
   if( sched_getaffinity( thread_id, cpu_allocate_size, cpuset ) != 0 )
   {
      perror( "Failed to get affinity for calling thread!" );
      return( false );
   }
   /**
    * NOTE: for the moment we're using an extremely simplistic
    * distance metric, we should probably be using the numa_distance
    * combined with the actual latancy/bandwidth between the 
    * processor cores.
    */
   auto numa_node_count( 0 );
   auto denom( 0 );
   for( auto p_index( 0 ); p_index < num_cpus_alloc; p_index++ )
   {
      if( CPU_ISSET( p_index /* cpu */, cpuset ) )
      {
         const auto ret_val( numa_node_of_cpu( p_index ) );
         if( ret_val != -1 )
         {
            numa_node_count += ret_val;
            denom++;
         }
      }
   }
   const auto target_node( numa_node_count / denom );

   /** we need to know how many pages we have **/
   const auto num_pages(
      static_cast< std::size_t >(
      std::ceil( 
      static_cast< float >( nbytes ) / static_cast< float >( page_size ) ) ) );
   using int_t = std::int32_t;
   int_t *mem_node  ( new int_t[ num_pages ] );
   int_t *mem_status( new int_t[ num_pages ] );
   void **page_ptr = (void**)malloc( sizeof( void* ) * num_pages );
   char *temp_ptr( reinterpret_cast< char* >( ptr ) );
   for( auto node_index( 0 ), 
             page_index( 0 )                       /** init **/; 
             node_index < num_pages                /** cond **/; 
             node_index++, page_index += page_size /** incr **/)
   {
      mem_node  [ node_index ] = target_node;
      mem_status[ node_index ] = -1;
      page_ptr[ node_index ] = (void*)&temp_ptr[ page_index ];
   }
   if( move_pages(   thread_id /** thread we want to move for **/,
                     num_pages /** total number of pages **/,
                     page_ptr  /** pointers to the start of each page **/,
                     mem_node  /** node we want to move to **/,
                     mem_status/** status flags, check for debug **/,
                     MPOL_MF_MOVE ) != 0 )
   {
#if DEBUG
      perror( "failed to move pages, non-fatal error but results may vary.");
      /** TODO, re-code all status flag checks..accidentally over wrote **/
#endif
      moved = false;
   }
   delete[]( mem_node );
   delete[]( mem_status );
   free( page_ptr );
   return( moved );
#else /** no NUMA avail **/
   return( false );
#endif
}
