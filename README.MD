Note: This program has only been verified to work with Ubuntu 20.10 and g++.  

## Compilation
To be able to compile the project you will need a copy of live555 in the same
directory as the dense_project folder. Live555 can be found
[here](http://www.live555.com/liveMedia/public/). The Live555 folder will also
have to be named 'live'.

Live555 it self may require you to configure a Makefile for your system.
This can be done like ...  

After this you should in theory be able to compile the programs using the make
command.  
    
    make

You can also use add options like -B to force all files to be recompiled and -jn
where n is the amount of threads you want to use for compilation.This may
improve compilation time. Example:

    make -B -j16
