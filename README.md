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
	```C++
	// You will need to create a fake argc and argv
	// Here's the common case, the equivelant of "node myScript.js" from the command line
	int argc = 1;
	char* argv = "myScript.js";

	node:Start(argc, &argv);
	```

### To read more stuff
Look at the node repository
https://github.com/joyent/node