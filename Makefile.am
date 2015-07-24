AUTOMAKE_OPTIONS=foreign
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = lib testsuite
AM_CXXFLAGS =-O0 -g -std=c++11
AM_CFLAGS   =-O0 -g -std=c99
AM_CPPFLAGS = -I./$(top_srcdir)/include
include_HEADERS = $(top_srcdir)/include/shm
AM_LDFLAGS=
if STATIC
AM_LDFLAGS += -static -static-libgcc -static-libstdc++
endif
