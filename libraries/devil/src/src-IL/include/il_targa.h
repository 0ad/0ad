//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_targa.h
//
// Description: Targa (.tga) functions
//
//-----------------------------------------------------------------------------


#ifndef TARGA_H
#define TARGA_H

#include "il_internal.h"

#ifdef _MSC_VER
#pragma pack(push, tga_struct, 1)
#endif
// Mac OS X fix from Tron Thomas
#ifdef MACOSX 
#pragma pack(1)
#endif

typedef struct TARGAHEAD
{
	ILubyte		IDLen;
	ILubyte		ColMapPresent;
	ILubyte		ImageType;
	ILshort		FirstEntry;
	ILshort		ColMapLen;
	ILubyte		ColMapEntSize;

	ILshort		OriginX;
	ILshort		OriginY;
	ILushort	Width;
	ILushort	Height;
	ILubyte		Bpp;
	ILubyte		ImageDesc;
} IL_PACKSTRUCT TARGAHEAD;

typedef struct TARGAFOOTER
{
	ILuint ExtOff;			// Extension Area Offset
	ILuint DevDirOff;		// Developer Directory Offset
	ILbyte Signature[16];	// TRUEVISION-XFILE
	ILbyte Reserved;		// ASCII period '.'
	ILbyte NullChar;		// NULL
} IL_PACKSTRUCT TARGAFOOTER;
#ifdef MACOSX
#pragma pack()
#endif
#ifdef _MSC_VER
#pragma pack(pop, tga_struct)
#endif

#define TGA_EXT_LEN		495
typedef struct TARGAEXT
{
	// Dev Directory
	//	We don't mess with this

	// Extension Area
	ILshort	Size;				// should be TGA_EXT_LEN
	ILbyte	AuthName[41];		// the image author's name
	ILbyte	AuthComments[324];	// author's comments
	ILshort	Month, Day, Year, Hour, Minute, Second;	// internal date of file
	ILbyte	JobID[41];			// the job description (if any)
	ILshort	JobHour, JobMin, JobSecs;	// the job's time
	ILbyte	SoftwareID[41];		// the software that created this
	ILshort	SoftwareVer;		// the software version number * 100
	ILbyte	SoftwareVerByte;	// the software version letter
	ILint	KeyColor;			// the transparent colour
} TARGAEXT;


// Different Targa formats
#define TGA_NO_DATA				0
#define TGA_COLMAP_UNCOMP		1
#define TGA_UNMAP_UNCOMP		2
#define TGA_BW_UNCOMP			3
#define TGA_COLMAP_COMP			9
#define TGA_UNMAP_COMP			10
#define TGA_BW_COMP				11


// Targa origins
#define IMAGEDESC_ORIGIN_MASK	0x30
#define IMAGEDESC_TOPLEFT		0x20
#define IMAGEDESC_BOTLEFT		0x00
#define IMAGEDESC_BOTRIGHT		0x10
#define IMAGEDESC_TOPRIGHT		0x30


// Internal functions
ILboolean	iIsValidTarga();
ILboolean	iGetTgaHead(TARGAHEAD *Header);
ILboolean	iCheckTarga(TARGAHEAD *Header);
ILboolean	iLoadTargaInternal(ILvoid);
ILboolean	iSaveTargaInternal(ILvoid);
//ILvoid		iMakeString(char *Str);
ILboolean	iReadBwTga(TARGAHEAD *Header);
ILboolean	iReadColMapTga(TARGAHEAD *Header);
ILboolean	iReadUnmapTga(TARGAHEAD *Header);
ILboolean	iUncompressTgaData(ILimage *Image);
ILboolean	i16BitTarga(ILimage *Image);
ILvoid		iGetDateTime(ILuint *Month, ILuint *Day, ILuint *Yr, ILuint *Hr, ILuint *Min, ILuint *Sec);


#endif//TARGA_H
