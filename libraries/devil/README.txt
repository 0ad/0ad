'src' contains a modified version of DevIL (http://openil.sf.net), based on 1.6.7. (Modifications by Philip Taylor / philip@wildfiregames.com etc)

Fairly significant changes have been made to the DDS code (il_dds.h, il_dds.c, il_dds-save.c), to get far better quality at the expense of speed. I think some bugs have been fixed in some filtering code as well, except I can't remember very well now.

The changes should probably be cleaned up a little and then offered to the DevIL project.

'include' includes the headers, and 'lib' contains precompiled libraries, intended for use by tools that want DevIL. Combinations of Release/Debug and ANSI/Unicode are provided - the standard filenames are for Unicode, and you need to add '_A' for the standard non-Unicode version. The .sln in src lets you recompile all the libraries using Batch Build if you ever think it'd be a fun thing to do.