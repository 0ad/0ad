//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/26/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_pix.c
//
// Description: Reads from an Alias | Wavefront .pix file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PIX
#include "il_manip.h"
#include "il_endian.h"


#ifdef _MSC_VER
#pragma pack(push, pix_struct, 1)
#endif
typedef struct PIXHEAD
{
	ILushort	Width;
	ILushort	Height;
	ILushort	OffX;
	ILushort	OffY;
	ILushort	Bpp;
} IL_PACKSTRUCT PIXHEAD;
#ifdef _MSC_VER
#pragma pack(pop, pix_struct)
#endif

ILboolean iCheckPix(PIXHEAD *Header);
ILboolean iLoadPixInternal(ILvoid);


// Internal function used to get the Pix header from the current file.
ILboolean iGetPixHead(PIXHEAD *Header)
{
	if (iread(Header, sizeof(PIXHEAD), 1) != 1)
		return IL_FALSE;

	BigUShort(&Header->Width);
	BigUShort(&Header->Height);
	BigUShort(&Header->OffX);
	BigUShort(&Header->OffY);
	BigUShort(&Header->Bpp);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidPix()
{
	PIXHEAD	Head;

	if (!iGetPixHead(&Head))
		return IL_FALSE;
	iseek(-(ILint)sizeof(PIXHEAD), IL_SEEK_CUR);

	return iCheckPix(&Head);
}


// Internal function used to check if the HEADER is a valid Pix header.
ILboolean iCheckPix(PIXHEAD *Header)
{
	if (Header->Width == 0 || Header->Height == 0)
		return IL_FALSE;
	if (Header->Bpp != 24)
		return IL_FALSE;
	//if (Header->OffY != Header->Height)
	//	return IL_FALSE;

	return IL_TRUE;
}


//! Reads a Pix file
ILboolean ilLoadPix(const ILstring FileName)
{
	ILHANDLE	PixFile;
	ILboolean	bPix = IL_FALSE;

	PixFile = iopenr(FileName);
	if (PixFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPix;
	}

	bPix = ilLoadPixF(PixFile);
	icloser(PixFile);

	return bPix;
}


//! Reads an already-opened Pix file
ILboolean ilLoadPixF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadPixInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a Pix
ILboolean ilLoadPixL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadPixInternal();
}


// Internal function used to load the Pix.
ILboolean iLoadPixInternal()
{
	PIXHEAD	Header;
	ILuint	i, j;
	ILubyte	ByteHead, Colour[3];

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetPixHead(&Header))
		return IL_FALSE;
	if (!iCheckPix(&Header)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ilTexImage(Header.Width, Header.Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	for (i = 0; i < iCurImage->SizeOfData; ) {
		ByteHead = igetc();
		if (iread(Colour, 1, 3) != 3)
			return IL_FALSE;
		for (j = 0; j < ByteHead; j++) {
			iCurImage->Data[i++] = Colour[0];
			iCurImage->Data[i++] = Colour[1];
			iCurImage->Data[i++] = Colour[2];
		}
	}

	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	ilFixImage();

	return IL_TRUE;
}

#endif//IL_NO_PIX
