####
To build this libray on OS X you'll need to run: 
glibtoolize
aclocal
autoconf
automake --add-missing
make
make check

The only changes for linux will be libtoolize for 
call to libtool instead of glibtoolize...on OS X
the binary might be named differently depending 
on how you've installed it so your mileage may 
vary.  If you're advanced enough to install libtool
on OS X you should be able to figure this out.

Hopefully everything will work well....there are fairly good comments
if you're interested in the shm header file under /include.


