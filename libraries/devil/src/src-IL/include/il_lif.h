//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2001 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_lif.c
//
// Description: Reads a Homeworld image.
//
//-----------------------------------------------------------------------------


#ifndef LIF_H
#define LIF_H

#include "il_internal.h"

typedef struct LIF_HEAD
{
    char	Id[8];			//"Willy 7"
    ILuint	Version;		// Version Number (260)
    ILuint	Flags;			// Usually 50
    ILuint	Width;
	ILuint	Height;
    ILuint	PaletteCRC;		// CRC of palettes for fast comparison.
    ILuint	ImageCRC;		// CRC of the image.
	ILuint	PalOffset;		// Offset to the palette (not used).
	ILuint	TeamEffect0;	// Team effect offset 0
	ILuint	TeamEffect1;	// Team effect offset 1
} LIF_HEAD;

ILboolean iIsValidLif(ILvoid);
ILboolean iCheckLif(LIF_HEAD *Header);
ILboolean iLoadLifInternal(ILvoid);

#endif//LIF_H
