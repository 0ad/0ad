//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/21/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_pic.h
//
// Description: Softimage Pic (.pic) functions
//
//-----------------------------------------------------------------------------


#ifndef PIC_H
#define PIC_H

#include "il_internal.h"

#ifdef _MSC_VER
#pragma pack(push, packed_struct, 1)
#endif
typedef struct PIC_HEAD
{
   ILint	Magic;			// PIC_MAGIC_NUMBER
   ILfloat	Version;		// Version of format
   ILbyte	Comment[80];	// Prototype description
   ILbyte	Id[4];			// 'PICT'
   ILshort	Width;			// Image width, in pixels
   ILshort	Height;			// Image height, in pixels
   ILfloat	Ratio;			// Pixel aspect ratio
   ILshort	Fields;			// Picture field type
   ILshort	Padding;		// Unused
} IL_PACKSTRUCT PIC_HEAD;

typedef struct CHANNEL
{
	ILubyte	Size;
	ILubyte	Type;
	ILubyte	Chan;
	ILvoid	*Next;
} CHANNEL;
#ifdef _MSC_VER
#pragma pack(pop,  packed_struct)
#endif


// Data type
#define PIC_UNSIGNED_INTEGER	0x00
#define PIC_SIGNED_INTEGER		0x10	// XXX: Not implemented
#define PIC_SIGNED_FLOAT		0x20	// XXX: Not implemented


// Compression type
#define PIC_UNCOMPRESSED		0x00
#define PIC_PURE_RUN_LENGTH		0x01
#define PIC_MIXED_RUN_LENGTH	0x02

// CHANNEL types (OR'd)
#define PIC_RED_CHANNEL			0x80
#define PIC_GREEN_CHANNEL		0x40
#define PIC_BLUE_CHANNEL		0x20
#define PIC_ALPHA_CHANNEL		0x10
#define PIC_SHADOW_CHANNEL		0x08	// XXX: Not implemented
#define PIC_DEPTH_CHANNEL		0x04	// XXX: Not implemented
#define PIC_AUXILIARY_1_CHANNEL	0x02	// XXX: Not implemented
#define PIC_AUXILIARY_2_CHANNEL	0x01	// XXX: Not implemented

ILboolean iIsValidPic(ILvoid);
ILboolean iCheckPic(PIC_HEAD *Header);
ILboolean iLoadPicInternal(ILvoid);
ILboolean readScanlines(ILuint *image, ILint width, ILint height, CHANNEL *channel, ILuint alpha);
ILuint    readScanline(ILubyte *scan, ILint width, CHANNEL *channel,  ILint bytes);
ILboolean channelReadRaw(ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes);
ILboolean channelReadPure(ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes);
ILboolean channelReadMixed(ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes);



#endif//PIC_H
