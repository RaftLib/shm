#Description
Easy SHM library for C++, still a work in progress but it's 
a easier way to code up an anonymous SHM mapping, protect
the last page and generate a key to access it....I've used
similar code quite a bit and was tired of re-coding it.  If
I find time I'll make it a bit more advanced, like adding
a monitor process to cleanup if for some reason the code
crashes (you know, things happen right?).  I've tested the
code on OS X and Linux, but as usual your mileage may vary
greatly.

#Important Notes
So I've checked the functions found in the testsuite.  I've
used the code quite a bit in various forms so it should work
rather well...however if you find bugs please submit a pull
request and I'll get it fixed ASAP.

#Compilation Notes
To build this library on OS X you'll need to run: 
glibtoolize
aclocal
autoconf
automake --add-missing
make
make check
sudo make intall

The only changes for linux will be libtoolize for 
call to libtool instead of glibtoolize...on OS X
the binary might be named differently depending 
on how you've installed it so your mileage may 
vary.  If you're advanced enough to install libtool
on OS X you should be able to figure this out.


#Usage
To use this library, simply:
\#include <shm>

When compiling your code that uses it, link with -lshm.  On 
Linux you'll have to compile with the -lrt, -lpthread and 
-lnuma in order to fully utilize all the features.  In 
the future I might add a flag to compile without the NUMA
features so that you can use the library without having 
to install libnuma.

#Parting Thoughts
Hopefully everything will work well....there are fairly good comments
if you're interested in the SHM header file under /include.


