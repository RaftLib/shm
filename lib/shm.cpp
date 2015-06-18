/**
 * shm.cpp - 
 * @author: Jonathan Beard
 * @version: Thu Aug  1 14:26:34 2013
 */
#include <shm>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <sched.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <errno.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <cstdint>
#include <cinttypes>
#include <iostream>
#include <sstream>
#include <cmath>

#if __APPLE__
#include <malloc/malloc.h>
#endif

#if __linux
/** might need to install numactl-dev **/
#include <malloc.h>
#include <numaif.h>
#include <numa.h>
#endif

bad_shm_alloc::bad_shm_alloc( const std::string message ) : std::exception(),
                                                            message( message )
{
   /** nothing to do **/
}


const char*
bad_shm_alloc::what() const noexcept 
{
   return( message.c_str() );
}

page_alignment_exception::page_alignment_exception( const std::string message ) : std::exception(),
                                                                                  message( message )
{
   /** nothing to do **/
}

const char*
page_alignment_exception::what() const noexcept
{
   return( message.c_str() );
}


invalid_key_exception::invalid_key_exception( const std::string message ) : std::exception(),
                                                                            message( message )
{
   /** nothing to do **/
}

const char*
invalid_key_exception::what() const noexcept
{
   return( message.c_str() );
}

void
shm::genkey( char * const buffer, 
             const std::size_t length )
{
   assert( buffer != nullptr );
   if( length == 0 )
   {
      throw invalid_key_exception( "Key length must be longer than zero!" );   
   }
   using key_t = std::uint32_t;
#define PRIKEY PRIu32
   key_t key( 1 ); 
#ifndef DEBUG
   FILE *fp = std::fopen("/dev/urandom","r");
   if(fp == NULL)
   {
      const char *err = "Error, couldn't open /dev/urandom!!\n";
      std::perror( err );
      exit( EXIT_FAILURE );
   }
   if( std::fread( &key, sizeof( key_t ), 1, fp) != 1 )
   {
      const char *err = "Error, incorrect number of integers retured!!\n";
      std::fclose(fp);
      std::perror(err);
      exit( EXIT_FAILURE );
   }
   else
   {
      std::fclose( fp );
   }
#endif
   const auto buffer_size( 1000 );
   char gen_buffer[ buffer_size ];
   std::memset( gen_buffer, 
                '\0', 
                buffer_size );

   std::snprintf( gen_buffer, 
                  buffer_size, 
                  "%" PRIKEY "", 
                  key );
   const auto cp_length( 
      std::min( 
         std::strlen( gen_buffer ), length ) );
   std::memset(  buffer, 
                '\0', 
                length );
   std::strncpy( buffer, 
                 gen_buffer,
                 cp_length - 1 /* null term */ );
   return;
}

void*
shm::init( const char * const key,
           const std::size_t nbytes,
           const bool zero   /* zero mem */,
           void   *ptr )
{
   assert( key != nullptr );
   if( nbytes == 0 )
   {
      throw bad_shm_alloc( "nbytes cannot be zero when allocating memory!" );
   }
   const std::int32_t success( 0 );
   const std::int32_t failure( -1 );
   int fd( failure  );
   errno = success;
   /* essentially we want failure if the file exists already */
   if( access( key, F_OK ) == success )
   {
      std::stringstream ss;
      ss << "File exists with name \"" << key << "\", error code returned: ";
      ss << strerror( errno );
      throw bad_shm_alloc( ss.str() );
   }
   /* set read/write set create if not exists */
   const std::int32_t flags( O_RDWR | O_CREAT | O_EXCL );
   /* set read/write by user */
   const mode_t mode( S_IWUSR | S_IRUSR );
   errno = success;
   fd  = shm_open( key, 
                   flags, 
                   mode );
   if( fd == failure )
   {
      std::stringstream ss;
      ss << "Failed to open shm with file descriptor \"" << key << "\", error code returned: ";
      ss << strerror( errno );
      throw bad_shm_alloc( ss.str() ); 
   }
   /* else begin truncate */
   errno = success;
   if( ftruncate( fd, nbytes ) != success )
   {
      std::stringstream ss;
      ss << "Failed to truncate shm for file descriptor (" << fd << ") ";
      ss << "with number of bytes (" << nbytes << ").  Error code returned: ";
      ss << strerror( errno );
      shm_unlink( key );
      throw bad_shm_alloc( ss.str() );
   }
   /* else begin mmap */
   /** get allocations size including extra dummy page **/
   const auto page_size( sysconf( _SC_PAGESIZE ) );
   const auto alloc_bytes( 
      static_cast< std::size_t >( 
         std::ceil(  
            static_cast< float >( nbytes) / 
           static_cast< float >( page_size ) ) + 1 ) * page_size 
   );
   /** 
    * NOTE: actual allocation size should be alloc_bytes,
    * user has no idea so we'll re-calc this at the  end
    * when we unmap the data.
    */
   errno = success;
   void *out( nullptr );
   out = mmap( ptr, 
               alloc_bytes, 
               ( PROT_READ | PROT_WRITE ), 
               MAP_SHARED, 
               fd, 
               0 );
   if( out == MAP_FAILED )
   {
      std::stringstream ss;
      ss << "Failed to mmap shm region with the following error: " << strerror( errno ) << ",\n";
      ss << "unlinking.";
      shm_unlink( key );
      throw bad_shm_alloc( ss.str() );
   }
   /** mmap should theoretically return start of page **/
   assert( reinterpret_cast< std::uintptr_t >( out ) % page_size == 0 );
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

/** 
 * Open - opens the shared memory segment with the file
 * descriptor stored at key.
 * @param   key - const char *
 * @return  void* - start of allocated memory, or NULL if
 *                  error
 */
void*
shm::open( const char *key )
{
   assert( key != nullptr );
   /* accept no zero length keys */
   assert( strlen( key ) > 0 );
   const std::int32_t success( 0 );
   const std::int32_t failure( -1 );
   int fd( failure );
   struct stat st;
   std::memset( &st, 
                0x0, 
                sizeof( struct stat ) );
   const int flags( O_RDWR | O_CREAT );
   mode_t mode( 0 );
   errno = success;
   fd = shm_open( key, 
                  flags, 
                  mode ); 
   if( fd == failure )
   {
      std::stringstream ss;
      ss << "Failed to open shm with key \"" << key << "\", with the following error code: ";
      ss << strerror( errno ); 
      throw bad_shm_alloc( ss.str() );
   }
   /* stat the file to get the size */
   errno = success;
   if( fstat( fd, &st ) != success )
   {
      std::stringstream ss;
      ss << "Failed to stat shm region with the following error: " << strerror( errno ) << ",\n";
      ss << "unlinking.";
      shm_unlink( key );
      throw bad_shm_alloc( ss.str() );
   }
   void *out( NULL );
   errno = success;
   out = mmap( NULL, 
               st.st_size, 
               (PROT_READ | PROT_WRITE), 
               MAP_SHARED, 
               fd, 
               0 );
   if( out == MAP_FAILED )
   {
      std::stringstream ss;
      ss << "Failed to mmap shm region with the following error: " << strerror( errno ) << ",\n";
      ss << "unlinking.";
      shm_unlink( key );
      throw bad_shm_alloc( ss.str() );
   }
   /* close fd */
   ::close( fd );
   /* done, return mem */
   return( out );
}

bool
shm::close( const char *key,
            void *ptr,
            const std::size_t nbytes,
            const bool zero,
            const bool unlink )
{
   assert( key != nullptr );
   std::stringstream ss_a;
#if __linux
   ss_a << "/dev/shm/" << key;
#elif defined __APPLE__
   ss_a << key;
#else
#warning "Undefined architecture, results may vary!"
   ss_a << key;
#endif
   if( access( ss_a.str().c_str(), F_OK ) != 0 )
   {
      std::stringstream ss;
      ss << "Can't find key: \"" << ss_a.str() << "\", check again and manually clean up.";
      throw invalid_key_exception( ss.str() ); 
   }
   const std::int32_t success( 0 );
   if( zero )
   {
      std::memset( ptr, 0x0, nbytes );
   }
   /** get allocations size including extra dummy page **/
   const auto page_size( sysconf( _SC_PAGESIZE ) );
   const auto alloc_bytes( 
      static_cast< std::size_t >( 
         std::ceil(  
            static_cast< float >( nbytes ) / 
           static_cast< float >( page_size ) ) + 1 ) * page_size 
   );
   errno = success;
   if( munmap( ptr, alloc_bytes ) != success )
   {
#if DEBUG   
      perror( "Failed to unmap shared memory, attempting to close!!" );
#endif
   }
   if( unlink )
   {
      errno = success;
      return( shm_unlink( key ) == success );
   }
   else
   {
      return( true );
   }
}

#ifdef __linux
bool
move_to_tid_numa( const pid_t thread_id,
                  void *ptr,
                  const std::size_t nbytes )
{
   /** check alignment of pages first **/
   const auto page_size( sysconf( _SC_PAGESIZE ) );
   
   const auto ptr_addr( 
      reinterpret_cast< std::uintptr_t >( ptr ) );
   if( ptr_addr % page_size == 0 )
   {
      std::stringstream ss;
      ss << "Variable 'ptr' must be page aligned, currently it is(" << 
       ptr_addr << "), please fix...exiting!!\n";
      throw page_alignment_exception( ss.str() );
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
    * CPUs and multiple possible NUMA nodes to choose from.  The simple
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
   int_t *mem_node     = new int_t[ num_pages ];
   int_t *mem_status   = new int_t[ num_pages ];
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
}
#endif
