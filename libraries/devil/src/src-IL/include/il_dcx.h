//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 12/03/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_dcx.h
//
// Description: Reads from a .dcx file.
//
//-----------------------------------------------------------------------------


#ifndef DCX_H
#define DCX_H

#include "il_internal.h"


#ifdef _WIN32
#pragma pack(push, packed_struct, 1)
#endif
typedef struct DCXHEAD
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
} IL_PACKSTRUCT DCXHEAD;
#ifdef _WIN32
#pragma pack(pop, packed_struct)
#endif

// For checking and reading
ILboolean iIsValidDcx(ILvoid);
ILboolean iCheckDcx(DCXHEAD *Header);
ILboolean iLoadDcxInternal(ILvoid);
ILimage*  iUncompressDcx(DCXHEAD *Header);
ILimage*  iUncompressDcxSmall(DCXHEAD *Header);

#endif//PCX_H
