Workspace settings for VC6:
________________________________________________________________________________

Compiler settings:
- set code generation to multithreaded (+debug if desired)
- set entry point to entry
________________________________________________________________________________

Includes:
- download http://oss.sgi.com/projects/ogl-sample/ABI/glext.h ;
  put it in GL subdir of compiler's include dir
- add dir containing tex.h etc. to the compiler's include path
________________________________________________________________________________

Xerces (required for the GUI, which can be omitted ATM)

download Xerces binary from http://xml.apache.org/xerces-c/download.cgi ;
put the DLL in bin in your system dir, or in the game's dir;
add its include dir to the compiler's include path
________________________________________________________________________________

ZLib 
  
download from http://www.stud.uni-karlsruhe.de/~urkt/zlib.zip ; 
put the DLL in the game's dir, and the headers/lib in the appropriate compiler 
dirs. 
Note: another version of ZLib will probably do, but best to use this one, to 
avoid conflicts (v1.1.4.8751).
________________________________________________________________________________

Files to leave out, as they aren't necessary ATM, and require compiler support:
- debug* (requires dbghelp, included with VC7)
- memcpy.cpp (requires VC6 processor pack)
________________________________________________________________________________

NB: earlier steps that are now unnecessary:
- adding .lib files - now taken care of by the code, via #pragma comment(lib
- installing DX7 SDK - detect code now uses stock Win32 and DX3 calls