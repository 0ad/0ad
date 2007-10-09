
===============================================================
        SR - Simulation and Representation Toolkit
       (also known as the small scene graph toolkit)

                Marcelo Kallmann 1995-2004

        See file sr.h for compilation options and
       important notes about programming conventions
===============================================================

==== LIBRARIES ====
 
 - sr: stand alone SR library with all main classes
 - srgl: OpenGL rendering of the SR scene nodes (www.opengl.org)
 - srfl: FLTK-OpenGL viewers and other UI tools (www.fltk.org)

==== EXECUTABLES ====

 - srmodel: demonstration and processing of SrModel
 - srtest: several small programs to test/demonstrate
           the toolkit. All tests are compiled in the
           same executable, but only one test is called
           from srtest_main.cpp.
 - srxapp: Example application to start a project containing
           a FLTK user interface and the 3D viewer SrViewer,
           which uses FLTK and OpenGL.
   
==== VISUALC ====

 - Configurations Debug and Release are the usual ones, and
   configuration Compact is meant to build executables
   without any dependencies on DLLs.
 - Generated libs are: sr, srfl, srgl (release)
                       src, srflc, srglc (compact), and
                       srd, srfld, srgld (debug)
 - Folder visualc6 contains projects for Visual C++ 6
 - Folder visualc7 contains projects for Visual C++ .NET
 - FLTK include and libs are set to: ..\..\fltk
  
==== LINUX ====

 - the linux folder contains makefiles for compilation using
   gnu g++ and gmake tools.
 - Some makefiles will look for FLTK as specified in the
   main makefile linux/makefile
 - gmake must be called from inside the linux/ directory
   
==== TESTED PLATFORMS ====

 - Windows 98 with Visual C++ 6.0
 - Windows XP with Visual C++ .NET
 - Linux with gmake and g++
  
==== KNOWN BUGS ====

 - check if SrSnEditor::~SrSnEditor() bug is really fixed
   
 - OpenGL: Transparency is currently disabled in SrViewer,
   because it had side-effects with "solid" polygons.

 - SrViewer: Need to review/fix camera control and view_all()
   
==== WISH LIST ====

 - Compare performance of using double or floats in sr_bv_math.h
 - Finish texture support in SrModel
 - SrPolygon: make a better grow() method
 - Make a good trackball manipulator scene node

