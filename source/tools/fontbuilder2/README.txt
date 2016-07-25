0 A.D. Font Builder 
====================

The Font Builder generates pre-rendered font glyphs for use in the game engine. Its output for each font consists
of an 8-bit greyscale PNG image and a descriptor .fnt file that describes and locates each individual glyph in the image
(see fileformat.txt for details).

See the wiki page for more information:

    http://trac.wildfiregames.com/wiki/Font_Builder2

Prerequisites
-------------

The main prerequisite for the fontbuilder is the Cairo imaging library and its Python bindings, PyCairo. On most
Linux distributions, this should be merely a matter of installing a package (e.g. 'python-cairo' for Debian/Ubuntu),
but on Windows it's more involved.

We'll demonstrate the process for Windows 32-bit first. Grab a Win32 binary for PyCairo from
     
     http://ftp.gnome.org/pub/GNOME/binaries/win32/pycairo/1.8/

and install it using the installer. There are installers available for Python versions 2.6 and 2.7. The installer
extracts the necessary files into Lib\site-packages\cairo within your Python installation directory.

Next is Cairo itself, and some dependencies which are required for Cairo to work. Head to
   
    http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/

and get the following binaries. Listed next to each are their version numbers at the time of writing; these may vary
over time, so be adaptive!
    
    - Cairo (cairo_1.8.10-3_win32.zip)
    - Fontconfig (fontconfig_2.8.0-2_win32.zip)
    - Freetype (freetype_2.4.4-1_win32.zip)
    - Expat (expat_2.0.1-1_win32.zip)
    - libpng (libpng_1.4.3-1_win32.zip)
    - zlib (zlib_1.2.5-2_win32.zip).

Each ZIP file will contain a bin subfolder with a DLL file in it. Put the following DLLs in Lib\site-packages\cairo
within your Python installation:
    
    libcairo-2.dll (from cairo_1.8.10-3_win32.zip)
    libfontconfig-1.dll (from fontconfig_2.8.0-2_win32.zip)
    freetype6.dll (from freetype_2.4.4-1_win32.zip)
    libexpat-1.dll (from expat_2.0.1-1_win32.zip)
    libpng14-14.dll (from libpng_1.4.3-1_win32.zip)
    zlib1.dll (from zlib_1.2.5-2_win32.zip).

You should be all set now. To test whether PyCairo installed successfully, try running the following command on a 
command line:

python -c "import cairo"

If it doesn't complain, then it's installed successfully.

On Windows 64-bit, the process is similar, but no pre-built PyCairo executable appears to be available from Gnome at
the time of writing. Instead, you can install PyGTK+ for 64-bit Windows, which includes Cairo, PyCairo, and the
same set of dependencies. See this page for details:

    http://www.pygtk.org/downloads.html

Running
-------

Running the font-builder is fairly straight-forward; there are no configuration options. One caveat is that you must
run it from its own directory as the current directory.

    python fontbuilder.py

This will generate the output .png and .fnt files straight into the binaries/data/mods/public/fonts directory, ready
for in-game use.
