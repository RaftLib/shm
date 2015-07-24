AUTOMAKE_OPTIONS = foreign
AM_CXXFLAGS=-O0 -g -std=c++11
AM_CFLAGS=-O0 -g -std=c99
AM_CPPFLAGS=-I$(top_srcdir)/include

LINUXADD=
AM_LDFLAGS=

if LINUX
LINUXADD    += -lrt -lpthread
AM_LDFLAGS  += -static -Wl,--no-whole-archive
endif

if NUMA
LINUXADD += -lnuma
AM_CPPFLAGS +=-DNUMA=1
else
AM_CPPFLAGS +=-DNUMA=0
endif

if STATIC
AM_LDFLAGS += -static -static-libgcc -static-libstdc++
endif

LDADD= -L$(top_srcdir)/lib -lshm $(LINUXADD)
TESTS = alloc close nokey outofrange zerobytes wrongkey zerokeylength genkeyoverflow\
        cppstylealloc cppstyleopen
if NUMA
TESTS += pagemigrate
endif

XFAIL_TESTS = nokey outofrange zerobytes zerokeylength genkeyoverflow


check_PROGRAMS = 
check_PROGRAMS += alloc
alloc_SOURCES = alloc.cpp

check_PROGRAMS += cppstylealloc
cppstylealloc_SOURCES = cppstylealloc.cpp

check_PROGRAMS += cppstyleopen
cppstyleopen_SOURCES = cppstyleopen.cpp

check_PROGRAMS += close
close_SOURCES = close.cpp

check_PROGRAMS += nokey
nokey_SOURCES = nokey.cpp

check_PROGRAMS += outofrange
outofrange_SOURCES = outofrange.cpp

check_PROGRAMS += zerobytes
zerobytes_SOURCES = zerobytes.cpp

check_PROGRAMS += wrongkey
wrongkey_SOURCES = wrongkey.cpp

check_PROGRAMS += zerokeylength
zerokeylength_SOURCES = zerokeylength.cpp

check_PROGRAMS += genkeyoverflow
genkeyoverflow_SOURCES = genkeyoverflow.cpp

if NUMA
check_PROGRAMS += pagemigrate
pagemigrate_SOURCES = pagemigrate.cpp
endif

CLEANFILES = *.log *.sum lldb
