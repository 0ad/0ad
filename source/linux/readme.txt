Linux build instructions

The Linux build is, so far, a bit hackish. Things might not work on another
system than mine, but it should work. The source/linux/ folder contains some
header files that exist to override and bug-fix some system headers. If you
have trouble with them, try renaming the offending header, and see if it does
you any good ;-)

What do I need?

- Xerces-C++ installed and in standard include path
- glext.h - you can put it in source/linux/GL/ if you don't have access to the
	system include path
- The Makefile is made for GCC 3.x.x - GCC 2.x might work, but it's not likely

Where are built things put?

The linux makefile follows the general directory layout of the VS workspaces:

source/linux/: makefile, system header overrides
binaries/: The built binaries - called "prometheus"
source/linux/deps/: Automatically generated dependency information for all
	source files
source/linux/o/: Object files
