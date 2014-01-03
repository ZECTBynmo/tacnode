# This is a fork of node that builds as a lib. 

As a post-build step (which you can remove) it concatenates all output libraries into one for convenience. All together, this is a very large library. Over 150 Mb on disk, and 2.5 - 3 Mb in memory once loaded.

NOTE: I haven't tested the concatenation step on anything but windows. If it doesn't work, there may be something wrong with how I setup the postbuild step in the gyp file.

For windows, I ended up calling the library concatenation script (tools/concatlibs.py) directly from vcbuild.bat

### Project Status

This works right now, but I wouldn't recommend trying to use it without being ready to roll up your sleeves.

Currently, there are issues with stderr and stdout. Where should they go? It would be great to build a 'log' page that's automatically hosted and receives all output, but right now nothing happens (unless it crashes haha).

### To build:

Prerequisites (Unix only):

    * Python 2.6 or 2.7
    * GNU Make 3.81 or newer
    * libexecinfo (FreeBSD and OpenBSD only)

Unix/Macintosh:

    ./configure
    make
    make install

Windows:

    vcbuild.bat

### To Use:
	///////////////////
    // You will need to create a fake argc and argv
	// Here's the common case, the equivelant of "node myScript.js" from the command line
    ///////////////////

    // First we grab the native function to find the current working directory
	#ifdef WINDOWS
        #include <direct.h>
        #define GetCurrentDir _getcwd
    #else
        #include <unistd.h>
        #define GetCurrentDir getcwd
    #endif

    char cCurrentPath[FILENAME_MAX];    // FILENAME_MAX is defined in stdio.h

    // Find the current working directory
    GetCurrentDir(cCurrentPath, sizeof(cCurrentPath));

    // Synthesize argv and argc
    char *argv[] = {cCurrentPath, "C:/myScript.js" };
    int argc = sizeof(argv) / sizeof(char*) - 1;

    // Now we start up node. The third argument sets whether we're using the
    // default UV message loop (which is blocking). If false, we use a custom
    // non-blocking loop thats setup using asynchronous boost timers
    node::Start( argc, argv, false );

### To read more stuff
Look at the node repository
https://github.com/joyent/node
