'src' contains a modified version of DevIL (http://openil.sf.net), based on 1.6.7. (Modifications by Philip Taylor / philip@wildfiregames.com etc)

Fairly significant changes have been made to the DDS code (il_dds.h, il_dds.c, il_dds-save.c), to get far better quality at the expense of speed.

There are some small changes to the configuration in il.h, and to the project files. Unused parts have been deleted. Everything else should be clean.

The IL project file is set to use the Intel Compiler, since that makes the DDS code run faster. It ought to be easy to compile it with normal MSVC, hopefully.


Pre-built .libs aren't provided, because they should be straightforward to rebuild as part of the textureconv solution, and most people never need them anyway (since the game doesn't use this library), and they're large, and they'll change whenever I update the DDS code.


The DDS changes should probably be cleaned up a little and then offered to the DevIL project.