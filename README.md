This is a fork of node that builds as a lib. As a post-build step (which you can remove) it concatinates all output libraries into one for convenience
===
NOTE: I haven't tested the concatination step on anything but windows. If it doesn't work, there may be something wrong with how I setup the postbuild step in the gyp file. 

For windows, I ended up calling the library concatination script (tools/concatlibs.py) directly from vcbuild.bat

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