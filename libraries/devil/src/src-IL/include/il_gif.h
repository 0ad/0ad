//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/18/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_gif.h
//
// Description: Reads from a Graphics Interchange Format (.gif) file.
//
//-----------------------------------------------------------------------------


#ifndef GIF_H
#define GIF_H

#include "il_internal.h"

#define GIF87A 87
#define GIF89A 89

#ifdef _WIN32
	#pragma pack(push, gif_struct, 1)
#endif
typedef struct GIFHEAD
{
	char		Sig[6];
	ILushort	Width;
	ILushort	Height;
	ILubyte		ColourInfo;
	ILubyte		Background;
	ILubyte		Aspect;
} IL_PACKSTRUCT GIFHEAD;

typedef struct IMAGEDESC
{
	ILubyte		Separator;
	ILushort	OffX;
	ILushort	OffY;
	ILushort	Width;
	ILushort	Height;
	ILubyte		ImageInfo;
} IL_PACKSTRUCT IMAGEDESC;

typedef struct GFXCONTROL
{
	ILubyte		Size;
	ILubyte		Packed;
	ILushort	Delay;
	ILubyte		Transparent;
	ILubyte		Terminator;
	ILboolean	Used;
} IL_PACKSTRUCT GFXCONTROL;
#ifdef _WIN32
	#pragma pack(pop, gif_struct)
#endif

// Internal functions
ILboolean iLoadGifInternal(ILvoid);
ILboolean ilLoadGifF(ILHANDLE File);
ILboolean iIsValidGif(ILvoid);
ILboolean iGetPalette(ILubyte Info, ILpal *Pal);
ILboolean GetImages(ILpal *GlobalPal);
ILboolean SkipExtensions(GFXCONTROL *Gfx);
ILboolean GifGetData(ILubyte *Data, ILuint ImageSize);
ILboolean RemoveInterlace(ILvoid);
ILboolean iCopyPalette(ILpal *Dest, ILpal *Src);
ILboolean ConvertTransparent(ILimage *Image, ILubyte TransColour);

#endif//GIF_H
