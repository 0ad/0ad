--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
NVIDIA Texture Tools
README.txt
Version 2.0
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
TABLE OF CONTENTS
--------------------------------------------------------------------------------
I.   Instructions
II.  Contents
III. Compilation Instructions
IV.  Using NVIDIA Texture Tools in your own applications
V.   Known Issues
VI.  Frequently Asked Questions
--------------------------------------------------------------------------------

I.   Introduction
--------------------------------------------------------------------------------

This is our first alpha release of our new Texture Tools. The main highlights of 
this release are support for all DX10 texture formats, higher speed and improved
compression quality.

In addition to that it also comes with a hardware accelerated compressor that 
uses CUDA to compress blocks in parallel on the GPU and runs around 10 times 
faster than the CPU counterpart.

You can obtain CUDA from our developer site at:

http://developer.nvidia.com/object/cuda.html

The source code of the Texture Tools is being released under the terms of 
the MIT license.


II.  Contents
--------------------------------------------------------------------------------

This release contains only the source code of the texture compression library
and an example commandline application that shows its use.


III. Compilation Instructions
--------------------------------------------------------------------------------

The compression library and the example can be compiled with Visual Studio 8 on 
Windows using the following solution file:

project\vc8\nvtt.sln

On most other platforms you can also use cmake. For more information about 
cmake, visit:

http://www.cmake.org/

On unix systems you can use the standard build procedure (assuming cmake is 
installed on your system):

$ ./configure
$ make
$ sudo make install


IV.  Using NVIDIA Texture Tools
--------------------------------------------------------------------------------

To use the NVIDIA Texture Tools in your own applications you just have to
include the following header file:

src/nvimage/nvtt/nvtt.h

And include the nvtt library in your projects. 

The following file contains a simple example that shows how to use the library:

src/nvimage/nvtt/compress.cpp

The usage of the commandline tool is the following:

$ nvcompress [options] infile [outfile]

where 'infile' is and TGA, PNG, PSD, DDS or JPG file, 'outfile' is a DDS file
and 'options' is one or more of the following:

Input options:
  -color   	The input image is a color map (default).
  -normal  	The input image is a normal map.
  -tonormal	Convert input to normal map.
  -clamp   	Clamp wrapping mode (default).
  -repeat  	Repeat wrapping mode.
  -nomips  	Disable mipmap generation.

Compression options:
  -fast    	Fast compression.
  -nocuda  	Do not use cuda compressor.
  -rgb     	RGBA format
  -bc1     	BC1 format (DXT1)
  -bc2     	BC2 format (DXT3)
  -bc3     	BC3 format (DXT5)
  -bc3n    	BC3 normal map format (DXT5n/RXGB)
  -bc4     	BC4 format (ATI1)
  -bc5     	BC5 format (3Dc/ATI2)

In order to run the compiled example on a PC that doesn't have Microsoft Visual 
Studio 2003 installed, you will have to install the Microsoft Visual Studio 2003
redistributable package that you can download at:

http://go.microsoft.com/fwlink/?linkid=65127&clcid=0x409


V.   Known Issues
--------------------------------------------------------------------------------

None so far. Please send suggestions and bug reports to:

TextureTools@nvidia.com

or report them at:

http://code.google.com/p/nvidia-texture-tools/issues/list


VI.  Frequently Asked Questions
--------------------------------------------------------------------------------

- Do the NVIDIA Texture Tools work on OSX?
It currently compiles and runs properly, but it has not been tested extensively.
In particular there may be endiannes errors in the code.


- Do the NVIDIA Texture Tools work on Linux?
Yes.


- Do the NVIDIA Texture Tools work on Vista?
Yes, but note that CUDA is not supported on Vista yet, so the tool is not hardware 
accelerated.


- Is CUDA required?
No. The Visual Studio solution file contains a configuration that allows you
to compile the texture tools without CUDA support. The cmake scripts automatically
detect the CUDA installation and use it only when available.


- Where can I get CUDA?
http://developer.nvidia.com/object/cuda.html


- Why is feature XYZ not supported?
In order to keep the code small and reduce maintenance costs we have limited the 
features available in our new texture tools. We also have open sourced the code, so
that people can modify it and add their own favourite features.


- Can I use the NVIDIA Texture Tools in my commercial application?
Yes, the NVIDIA Texture Tools are licensed under the MIT license.


- Can I use the NVIDIA Texture Tools in my GPL application?
Yes, the MIT license is compatible with the GPL and LGPL licenses.



