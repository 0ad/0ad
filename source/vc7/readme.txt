VC7.0 (2002) workspace (auto-converted from VC7.1)

TODO:

- get latest glext.h:
  download from http://oss.sgi.com/projects/ogl-sample/ABI/glext.h ;
  put it in GL subdir of compiler's include dir

- install ZLib:
  download from http://www.stud.uni-karlsruhe.de/~urkt/zlib.zip ; 
  put the DLL in binaries\, put header and lib into compiler dirs
  Note: this is version 1.1.4.8751

- omit GUI (not necessary ATM) - remove its folder, and define NO_GUI
OR
- install Xerces:
  download Xerces binary from http://xml.apache.org/xerces-c/download.cgi ;
  put the DLL in binaries\, put include\ contents into compiler include dir


NOTE: important workspace settings (already set)

- code generation = multithreaded [debug]
- entry point = entry
- add lib\ to include path