//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/include/sgi.h
//
// Description: Reads from and writes to SGI graphics files.
//
//-----------------------------------------------------------------------------


#ifndef SGI_H
#define SGI_H

#include "il_internal.h"

typedef struct iSgiHeader
{
	ILshort		MagicNum;	// IRIS image file magic number
	ILbyte		Storage;	// Storage format
	ILbyte		Bpc;		// Number of bytes per pixel channel
	ILushort	Dim;		// Number of dimensions
	ILushort	XSize;		// X size in pixels
	ILushort	YSize;		// Y size in pixels
	ILushort	ZSize;		// Number of channels
	ILint		PixMin;		// Minimum pixel value
	ILint		PixMax;		// Maximum pixel value
	ILint		Dummy1;		// Ignored
	ILbyte		Name[80];	// Image name
	ILint		ColMap;		// Colormap ID
	ILbyte		Dummy[404];	// Ignored
} IL_PACKSTRUCT iSgiHeader;

// Sgi format #define's
#define SGI_VERBATIM		0
#define SGI_RLE				1
#define SGI_MAGICNUM		474

// Sgi colormap types
#define SGI_COLMAP_NORMAL	0
#define SGI_COLMAP_DITHERED	1
#define SGI_COLMAP_SCREEN	2
#define SGI_COLMAP_COLMAP	3


// Internal functions
ILboolean	iIsValidSgi(ILvoid);
ILboolean	iCheckSgi(iSgiHeader *Header);
ILboolean	iLoadSgiInternal(ILvoid);
ILboolean	iSaveSgiInternal(ILvoid);
ILvoid		iExpandScanLine(ILubyte *Dest, ILubyte *Src, ILuint Bpc);
ILint		iGetScanLine(ILubyte *ScanLine, iSgiHeader *Head, ILuint Length);
ILint		iGetScanLineFast(ILubyte *ScanLine, iSgiHeader *Head, ILuint Length, ILubyte*);
ILvoid		sgiSwitchData(ILubyte *Data, ILuint SizeOfData);
ILboolean	iNewSgi(iSgiHeader *Head);
ILboolean	iReadNonRleSgi(iSgiHeader *Head);
ILboolean	iReadRleSgi(iSgiHeader *Head);
ILboolean	iSaveRleSgi(ILubyte *Data);

#endif//SGI_H
