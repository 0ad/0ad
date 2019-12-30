NVIDIA Texture Tools
====================

The NVIDIA Texture Tools is a collection of image processing and texture 
manipulation tools, designed to be integrated in game tools and asset 
processing pipelines.

The primary features of the library are mipmap and normal map generation, format 
conversion and DXT compression.


### How to build (Windows)

Open `project/vc12/thekla.sln` using Visual Studio.

Solutions for previous versions are also available, but they may not be up to date.


### How to build (Linux/OSX)

Use [cmake](http://www.cmake.org/) and the provided configure script:

```bash
$ ./configure
$ make
$ sudo make install
```


### Using NVIDIA Texture Tools

To use the NVIDIA Texture Tools in your own applications you just have to
include the following header file:

src/nvimage/nvtt/nvtt.h

And include the nvtt library in your projects. 

The following file contains a simple example that shows how to use the library:

src/nvimage/nvtt/compress.cpp

Detailed documentation of the API can be found at:

http://code.google.com/p/nvidia-texture-tools/wiki/ApiDocumentation

