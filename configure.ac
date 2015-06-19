#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libshm], [1], [jonathan.c.beard@gmail.com])
AC_CONFIG_SRCDIR([lib/shm.cpp])
AC_CONFIG_MACRO_DIR([m4])
#AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
LT_INIT
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h \
                  mman.h stat.h types.h sched.h sysinfo.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([ftruncate memset munmap strerror shm_unlink shm_open])

case $host_os in 
   darwin*)
      os_bsd=no
      os_darwin=yes
      os_linux=no
   ;;
   linux*)
      os_bsd=no
      os_darwin=no
      os_linux=yes
   ;;
   bsd*)
      os_bsd=yes
      os_darwin=no
      os_linux=no
   ;;
esac

AM_CONDITIONAL([DARWIN], [test x$os_darwin = xyes]) 
AM_CONDITIONAL([LINUX], [test x$os_linux = xyes]) 

AC_CONFIG_FILES([ 
                  Makefile
                  lib/Makefile
                  testsuite/Makefile
                ])
AC_OUTPUT
