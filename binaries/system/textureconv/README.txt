 TEXTURE CONVERTER
-=================-


Drag-and-drop BMP or TGA files onto TexConv_Normal.exe. The converter will create DDS files, in the same location as the original images. This might take a while - roughly ten seconds for a 256x256 image - so be patient. A message box will appear when it has finished.


The DDS file will be either DXT1 or DXT3, depending on whether it has any alpha data (for transparency, player-colours, etc) - DXT1 when there's no alpha, DXT3 when there is some.


If the image's height is double its width (e.g. 128x256 or 256x512), it will be treated specially: the alpha data is represented by the bottom half of the image, rather than the standard alpha channel. The bottom half should be greyscale; white for solid, black for transparent, and greys in between.

If you don't want this to happen (for example, if you have a texture for a shield that's taller than it is wide), use TexConv_WithoutAlpha.exe instead of TexConv_Normal.exe. (If you are converting a TGA file, its real alpha channel will be used instead.)

Similarly, if you do want this to happen but your texture isn't the right size, just use TexConv_WithAlpha.exe (which will force the bottom half to be interpreted as the alpha channel).


If you use a DDS file as input to the Texture Converter, it will output a BMP, with the alpha channel shown in the bottom half. Or use TexConv_ToTGA.exe to convert to TGA (using the real alpha channel).


Accepted input formats are currently: DDS, BMP, TGA, PSD, PSP (though the latter two are unlikely to work in all cases).


Various settings can be given from the command line or from .bat files. They are:

  -dxt1    Force output to be DXT1
  -dxt3    Force output to be DXT3
  -dxt5    Force output to be DXT5
  -bmp     Force output to be BMP
  -tga     Force output to be TGA

  -mipmaps       Store mipmaps in the DDS (default)
  -nomipmaps     Don't store mipmaps in the DDS 

  -alphablock    Use the bottom half of the image as the alpha channel
  -noalphablock  Use the image's real alpha channel

They can also be combined with nested brackets, and each setting only applies until a matching closing bracket is reached. For example
  "TexConv.exe 1.bmp ( -nomipmaps -dxt1 2.bmp -dxt3 3.bmp ( -dxt5 4.bmp ) 5.bmp ) 6.bmp"
will use the following settings:
  1.bmp - default
  2.bmp - DXT1, no mipmaps
  3.bmp - DXT3, no mipmaps
  4.bmp - DXT5, no mipmaps
  5.bmp - DXT3, no mipmaps
  6.bmp - default

The TexConv_*.exe programs just run textureconv.exe with some of those settings; it seems that they can't be .bat files, since those are unable to find textureconv.exe, which is very annoying, and so the code in source/tools/textureconv/launch is used to create all the exes.