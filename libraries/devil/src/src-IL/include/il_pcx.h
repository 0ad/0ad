//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_pcx.h
//
// Description: Reads and writes from/to a .pcx file.
//
//-----------------------------------------------------------------------------


#ifndef PCX_H
#define PCX_H

#include "il_internal.h"


#ifdef _WIN32
#pragma pack(push, packed_struct, 1)
#endif
typedef struct PCXHEAD
{
	ILubyte		Manufacturer;
	ILubyte		Version;
	ILubyte		Encoding;
	ILubyte		Bpp;
	ILushort	Xmin, Ymin, Xmax, Ymax;
	ILushort	HDpi;
	ILushort	VDpi;
	ILubyte		ColMap[48];
	ILubyte		Reserved;
	ILubyte		NumPlanes;
	ILushort	Bps;
	ILushort	PaletteInfo;
	ILushort	HScreenSize;
	ILushort	VScreenSize;
	ILubyte		Filler[54];
} IL_PACKSTRUCT PCXHEAD;
#ifdef _WIN32
#pragma pack(pop, packed_struct)
#endif

// For checking and reading
ILboolean iIsValidPcx(ILvoid);
ILboolean iCheckPcx(PCXHEAD *Header);
ILboolean iLoadPcxInternal(ILvoid);
ILboolean iSavePcxInternal(ILvoid);
ILboolean iUncompressPcx(PCXHEAD *Header);
ILboolean iUncompressSmall(PCXHEAD *Header);

// For writing
ILuint encput(ILubyte byt, ILubyte cnt);
ILuint encLine(ILubyte *inBuff, ILint inLen, ILubyte Stride);


#endif//PCX_H
