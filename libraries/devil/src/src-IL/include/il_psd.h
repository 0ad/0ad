//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2001 by Denton Woods
// Last modified: 01/23/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_il_psd.c
//
// Description: Reads from a PhotoShop (.psd) file.
//
//-----------------------------------------------------------------------------


#ifndef PSD_H
#define PSD_H

#include "il_internal.h"

#ifdef _MSC_VER
#pragma pack(push, packed_struct, 1)
#endif
typedef struct PSDHEAD
{
	ILubyte		Signature[4];
	ILushort	Version;
	ILubyte		Reserved[6];
	ILushort	Channels;
	ILuint		Height;
	ILuint		Width;
	ILushort	Depth;
	ILushort	Mode;
} IL_PACKSTRUCT PSDHEAD;

#ifdef _MSC_VER
#pragma pack(pop,  packed_struct)
#endif

ILushort	ChannelNum;

ILboolean	iIsValidPsd(ILvoid);
ILboolean	iCheckPsd(PSDHEAD *Header);
ILboolean	iLoadPsdInternal(ILvoid);
ILboolean	ReadPsd(PSDHEAD *Head);
ILboolean	ReadGrey(PSDHEAD *Head);
ILboolean	ReadIndexed(PSDHEAD *Head);
ILboolean	ReadRGB(PSDHEAD *Head);
ILboolean	ReadCMYK(PSDHEAD *Head);
ILuint		*GetCompChanLen(PSDHEAD *Head);
ILboolean	PsdGetData(PSDHEAD *Head, ILvoid *Buffer, ILboolean Compressed);
ILboolean	ParseResources(ILuint ResourceSize, ILubyte *Resources);
ILboolean	GetSingleChannel(PSDHEAD *Head, ILubyte *Buffer, ILboolean Compressed);
ILboolean	iSavePsdInternal(ILvoid);



#endif//PSD_H
