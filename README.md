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

#TODO
Still haven't sufficiently tested the page migration, will
do eventually but will take time to write sufficient test
cases really say "it works."

#Compilation Notes
To build this library on OS X you'll need to run: 
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
