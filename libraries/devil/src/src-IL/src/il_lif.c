//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_lif.c
//
// Description: Reads a Homeworld image.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_LIF
#include "il_lif.h"


//! Checks if the file specified in FileName is a valid Lif file.
ILboolean ilIsValidLif(const ILstring FileName)
{
	ILHANDLE	LifFile;
	ILboolean	bLif = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("lif"))) {
		ilSetError(IL_INVALID_EXTENSION);
		return bLif;
	}

	LifFile = iopenr(FileName);
	if (LifFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bLif;
	}

	bLif = ilIsValidLifF(LifFile);
	icloser(LifFile);

	return bLif;
}


//! Checks if the ILHANDLE contains a valid Lif file at the current position.
ILboolean ilIsValidLifF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iIsValidLif();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid Lif lump.
ILboolean ilIsValidLifL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iIsValidLif();
}


// Internal function used to get the Lif header from the current file.
ILboolean iGetLifHead(LIF_HEAD *Header)
{
	if (iread(Header, sizeof(LIF_HEAD), 1) != 1)
		return IL_FALSE;

	UInt(&Header->Version);
	UInt(&Header->Flags);
	UInt(&Header->Width);
	UInt(&Header->Height);
	UInt(&Header->PaletteCRC);
	UInt(&Header->ImageCRC);
	UInt(&Header->PalOffset);
	UInt(&Header->TeamEffect0);
	UInt(&Header->TeamEffect1);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidLif()
{
	LIF_HEAD	Head;

	if (!iGetLifHead(&Head))
		return IL_FALSE;
	iseek(-(ILint)sizeof(LIF_HEAD), IL_SEEK_CUR);

	return iCheckLif(&Head);
}


// Internal function used to check if the HEADER is a valid Lif header.
ILboolean iCheckLif(LIF_HEAD *Header)
{
	if (Header->Version != 260 || Header->Flags != 50)
		return IL_FALSE;
	if (stricmp(Header->Id, "Willy 7"))
		return IL_FALSE;
	return IL_TRUE;
}


//! Reads a .Lif file
ILboolean ilLoadLif(const ILstring FileName)
{
	ILHANDLE	LifFile;
	ILboolean	bLif = IL_FALSE;

	LifFile = iopenr(FileName);
	if (LifFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bLif;
	}

	bLif = ilLoadLifF(LifFile);
	icloser(LifFile);

	return bLif;
}


//! Reads an already-opened .Lif file
ILboolean ilLoadLifF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadLifInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .Lif
ILboolean ilLoadLifL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadLifInternal();
}


ILboolean iLoadLifInternal()
{
	LIF_HEAD	LifHead;
	ILuint		i;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (!iGetLifHead(&LifHead))
		return IL_FALSE;

	if (!ilTexImage(LifHead.Width, LifHead.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	iCurImage->Pal.Palette = (ILubyte*)ialloc(1024);
	if (iCurImage->Pal.Palette == NULL)
		return IL_FALSE;
	iCurImage->Pal.PalSize = 1024;
	iCurImage->Pal.PalType = IL_PAL_RGBA32;

	if (iread(iCurImage->Data, LifHead.Width * LifHead.Height, 1) != 1)
		return IL_FALSE;
	if (iread(iCurImage->Pal.Palette, 1, 1024) != 1024)
		return IL_FALSE;

	// Each data offset is offset by -1, so we add one.
	for (i = 0; i < iCurImage->SizeOfData; i++) {
		iCurImage->Data[i]++;
	}

	ilFixImage();

	return IL_TRUE;
}

#endif//IL_NO_LIF
