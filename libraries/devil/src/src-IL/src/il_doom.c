//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_doom.c
//
// Description: Reads Doom textures and flats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_DOOM
#include "il_manip.h"
#include "il_pal.h"
#include "il_doompal.h"


ILboolean iLoadDoomInternal(ILvoid);
ILboolean iLoadDoomFlatInternal(ILvoid);


//
// READ A DOOM IMAGE
//

//! Reads a Doom file
ILboolean ilLoadDoom(const ILstring FileName)
{
	ILHANDLE	DoomFile;
	ILboolean	bDoom = IL_FALSE;

	// Not sure of any kind of specified extension...maybe .lmp?
	/*if (!iCheckExtension(FileName, "")) {
		ilSetError(IL_INVALID_EXTENSION);
		return NULL;
	}*/

	DoomFile = iopenr(FileName);
	if (DoomFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bDoom;
	}

	bDoom = ilLoadDoomF(DoomFile);
	icloser(DoomFile);

	return bDoom;
}


//! Reads an already-opened Doom file
ILboolean ilLoadDoomF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadDoomInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a Doom texture
ILboolean ilLoadDoomL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadDoomInternal();
}


// From the DTE sources (mostly by Denton Woods with corrections by Randy Heit)
ILboolean iLoadDoomInternal()
{
	ILshort	width, height, graphic_header[2], column_loop, row_loop;
	ILint	column_offset, pointer_position, first_pos;
	ILubyte	post, topdelta, length;
	ILubyte	*NewData;
	ILuint	i;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	first_pos = itell();  // Needed to go back to the offset table
	width = GetLittleShort();
	height = GetLittleShort();
	graphic_header[0] = GetLittleShort();  // Not even used
	graphic_header[1] = GetLittleShort();  // Not even used

	if (!ilTexImage(width, height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	iCurImage->Pal.Palette = (ILubyte*)ialloc(IL_DOOMPAL_SIZE);
	if (iCurImage->Pal.Palette == NULL) {
		return IL_FALSE;
	}
	iCurImage->Pal.PalSize = IL_DOOMPAL_SIZE;
	iCurImage->Pal.PalType = IL_PAL_RGB24;
	memcpy(iCurImage->Pal.Palette, ilDefaultDoomPal, IL_DOOMPAL_SIZE);

	// 247 is always the transparent colour (usually cyan)
	memset(iCurImage->Data, 247, iCurImage->SizeOfData);

	for (column_loop = 0; column_loop < width; column_loop++) {
		column_offset = GetLittleInt();
		pointer_position = itell();
		iseek(first_pos + column_offset, IL_SEEK_SET);

		while (1) {
			if (iread(&topdelta, 1, 1) != 1)
				return IL_FALSE;
			if (topdelta == 255)
				break;
			if (iread(&length, 1, 1) != 1)
				return IL_FALSE;
			if (iread(&post, 1, 1) != 1)
				return IL_FALSE; // Skip extra byte for scaling

			for (row_loop = 0; row_loop < length; row_loop++) {
				if (iread(&post, 1, 1) != 1)
					return IL_FALSE;
				if (row_loop + topdelta < height)
					iCurImage->Data[(row_loop+topdelta) * width + column_loop] = post;
			}
			iread(&post, 1, 1); // Skip extra scaling byte
		}

		iseek(pointer_position, IL_SEEK_SET);
	}

	// Converts palette entry 247 (cyan) to transparent.
	if (ilGetBoolean(IL_CONV_PAL) == IL_TRUE) {
		NewData = (ILubyte*)ialloc(iCurImage->SizeOfData * 4);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		for (i = 0; i < iCurImage->SizeOfData; i++) {
			NewData[i * 4] = iCurImage->Pal.Palette[iCurImage->Data[i]];
			NewData[i * 4] = iCurImage->Pal.Palette[iCurImage->Data[i]];
			NewData[i * 4] = iCurImage->Pal.Palette[iCurImage->Data[i]];
			NewData[i * 4 + 3] = iCurImage->Data[i] != 247 ? 255 : 0;
		}

		if (!ilTexImage(iCurImage->Width, iCurImage->Height, iCurImage->Depth,
			4, IL_RGBA, iCurImage->Type, NewData)) {
			ifree(NewData);
			return IL_FALSE;
		}
		iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
		ifree(NewData);
	}

	ilFixImage();

	return IL_TRUE;
}


//
// READ A DOOM FLAT
//

//! Reads a Doom flat file
ILboolean ilLoadDoomFlat(const ILstring FileName)
{
	ILHANDLE	FlatFile;
	ILboolean	bFlat = IL_FALSE;

	// Not sure of any kind of specified extension...maybe .lmp?
	/*if (!iCheckExtension(FileName, "")) {
		ilSetError(IL_INVALID_EXTENSION);
		return NULL;
	}*/

	FlatFile = iopenr(FileName);
	if (FlatFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bFlat;
	}

	bFlat = ilLoadDoomF(FlatFile);
	icloser(FlatFile);

	return bFlat;
}


//! Reads an already-opened Doom flat file
ILboolean ilLoadDoomFlatF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadDoomFlatInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a Doom flat
ILboolean ilLoadDoomFlatL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadDoomFlatInternal();
}


// Basically just ireads 4096 bytes and copies the palette
ILboolean iLoadDoomFlatInternal()
{
	ILubyte	*NewData;
	ILuint	i;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!ilTexImage(64, 64, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	iCurImage->Pal.Palette = (ILubyte*)ialloc(IL_DOOMPAL_SIZE);
	if (iCurImage->Pal.Palette == NULL) {
		return IL_FALSE;
	}
	iCurImage->Pal.PalSize = IL_DOOMPAL_SIZE;
	iCurImage->Pal.PalType = IL_PAL_RGB24;
	memcpy(iCurImage->Pal.Palette, ilDefaultDoomPal, IL_DOOMPAL_SIZE);

	if (iread(iCurImage->Data, 1, 4096) != 4096)
		return IL_FALSE;

	if (ilGetBoolean(IL_CONV_PAL) == IL_TRUE) {
		NewData = (ILubyte*)ialloc(iCurImage->SizeOfData * 4);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		for (i = 0; i < iCurImage->SizeOfData; i++) {
			NewData[i * 4] = iCurImage->Pal.Palette[iCurImage->Data[i]];
			NewData[i * 4] = iCurImage->Pal.Palette[iCurImage->Data[i]];
			NewData[i * 4] = iCurImage->Pal.Palette[iCurImage->Data[i]];
			NewData[i * 4 + 3] = iCurImage->Data[i] != 247 ? 255 : 0;
		}

		if (!ilTexImage(iCurImage->Width, iCurImage->Height, iCurImage->Depth,
			4, IL_RGBA, iCurImage->Type, NewData)) {
			ifree(NewData);
			return IL_FALSE;
		}
		iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
		ifree(NewData);
	}

	ilFixImage();

	return IL_TRUE;
}


#endif
