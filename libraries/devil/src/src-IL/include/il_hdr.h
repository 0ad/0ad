//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2004 by Denton Woods (this file by thakis)
// Last modified: 06/09/2004
//
// Filename: src-IL/include/il_hdr.h
//
// Description: Reads a RADIANCE High Dynamic Range Image
//
//-----------------------------------------------------------------------------


#ifndef HDR_H
#define HDR_H

#include "il_internal.h"

#ifdef _WIN32
	#pragma pack(push, gif_struct, 1)
#endif

typedef struct HDRHEADER
{
	char Signature[11]; //must be "#?RADIANCE"
	ILuint Width, Height;
} IL_PACKSTRUCT HDRHEADER;

#ifdef _WIN32
	#pragma pack(pop, gif_struct)
#endif

// Internal functions
ILboolean ilIsValidHdrF(ILHANDLE file);
ILboolean iIsValidHdr();
ILboolean iCheckHdr(HDRHEADER *Header);
ILboolean ilLoadHdrF(ILHANDLE file);
ILboolean iLoadHdrInternal();

ILvoid ReadScanline(ILubyte *scanline, ILuint w);

#endif//HDR_H
