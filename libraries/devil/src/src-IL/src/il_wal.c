//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_wal.c
//
// Description: Loads a Quake .wal texture.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_WAL
#include "il_manip.h"
#include "il_q2pal.h"


typedef struct WALHEAD
{
	ILbyte	FileName[32];	// Image name
	ILuint	Width;			// Width of first image
	ILuint	Height;			// Height of first image
	ILuint	Offsets[4];		// Offsets to image data
	ILbyte	AnimName[32];	// Name of next frame
	ILuint	Flags;			// ??
	ILuint	Contents;		// ??
	ILuint	Value;			// ??
} WALHEAD;

ILboolean iLoadWalInternal(ILvoid);


//! Reads a .wal file
ILboolean ilLoadWal(const ILstring FileName)
{
	ILHANDLE	WalFile;
	ILboolean	bWal = IL_FALSE;

	WalFile = iopenr(FileName);
	if (WalFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bWal;
	}

	bWal = ilLoadWalF(WalFile);
	icloser(WalFile);

	return bWal;
}


//! Reads an already-opened .wal file
ILboolean ilLoadWalF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadWalInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .wal file
ILboolean ilLoadWalL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadWalInternal();
}


ILboolean iLoadWalInternal()
{
	WALHEAD	Header;
	ILimage	*Mipmaps[3], *CurImage;
	ILuint	i, NewW, NewH;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	CurImage = iCurImage;

	if (iread(&Header, sizeof(WALHEAD), 1) != 1)
		return IL_FALSE;
	if (!ilTexImage(Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	for (i = 0; i < 3; i++) {
		Mipmaps[i] = (ILimage*)icalloc(sizeof(ILimage), 1);
		if (Mipmaps[i] == NULL)
			goto cleanup_error;
		Mipmaps[i]->Pal.Palette = (ILubyte*)ialloc(768);
		if (Mipmaps[i]->Pal.Palette == NULL)
			goto cleanup_error;
		memcpy(Mipmaps[i]->Pal.Palette, ilDefaultQ2Pal, 768);
		Mipmaps[i]->Pal.PalType = IL_PAL_RGB24;
	}

	NewW = Header.Width;
	NewH = Header.Height;
	for (i = 0; i < 3; i++) {
		NewW /= 2;
		NewH /= 2;
		iCurImage = Mipmaps[i];
		if (!ilTexImage(NewW, NewH, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
			goto cleanup_error;
		// Don't set until now so ilTexImage won't get rid of the palette.
		Mipmaps[i]->Pal.PalSize = 768;
		Mipmaps[i]->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	iCurImage = CurImage;
	ilCloseImage(iCurImage->Mipmaps);
	iCurImage->Mipmaps = Mipmaps[0];
	Mipmaps[0]->Next = Mipmaps[1];
	Mipmaps[1]->Next = Mipmaps[2];

	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	if (iCurImage->Pal.Palette && iCurImage->Pal.PalSize && iCurImage->Pal.PalType != IL_PAL_NONE)
		ifree(iCurImage->Pal.Palette);
	iCurImage->Pal.Palette = (ILubyte*)ialloc(768);
	if (iCurImage->Pal.Palette == NULL)
		goto cleanup_error;

	iCurImage->Pal.PalSize = 768;
	iCurImage->Pal.PalType = IL_PAL_RGB24;
	memcpy(iCurImage->Pal.Palette, ilDefaultQ2Pal, 768);

	iseek(Header.Offsets[0], IL_SEEK_SET);
	if (iread(iCurImage->Data, Header.Width * Header.Height, 1) != 1)
		goto cleanup_error;

	for (i = 0; i < 3; i++) {
		iseek(Header.Offsets[i+1], IL_SEEK_SET);
		if (iread(Mipmaps[i]->Data, Mipmaps[i]->Width * Mipmaps[i]->Height, 1) != 1)
			goto cleanup_error;
	}

	iCurImage->NumMips = 3;

	// Fixes all images, even mipmaps.
	ilFixImage();

	return IL_TRUE;

cleanup_error:
	for (i = 0; i < 3; i++) {
		ilCloseImage(Mipmaps[i]);
	}
	return IL_FALSE;
}


#endif//IL_NO_WAL
