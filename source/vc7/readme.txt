VC7.1 (2003) workspace, for Intel compiler 7.1

TODO:

- get latest glext.h:
  download from http://oss.sgi.com/projects/ogl-sample/ABI/glext.h ;
  put it in GL subdir of compiler's include dir

- install ZLib:
  download from http://www.stud.uni-karlsruhe.de/~urkt/zlib.zip ; 
  put the DLL in binaries\, put header and lib into compiler dirs
  Note: this is version 1.1.4.8751

- omit GUI folder (not necessary ATM)
OR
- install Xerces:
  download Xerces binary from http://xml.apache.org/xerces-c/download.cgi ;
  put the DLL in binaries\, add the include dir to the compiler's include path


NOTE: important workspace settings (already set)

- code generation = multithreaded [debug]
- entry point = entry
- add lib\ to include path