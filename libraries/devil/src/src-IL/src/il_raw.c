//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2001 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_raw.c
//
// Description: "Raw" file functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_RAW


ILboolean iLoadRawInternal(ILvoid);
ILboolean iSaveRawInternal(ILvoid);


//! Reads a raw file
ILboolean ilLoadRaw(const ILstring FileName)
{
	ILHANDLE	RawFile;
	ILboolean	bRaw = IL_FALSE;

	// No need to check for raw
	/*if (!iCheckExtension(FileName, "raw")) {
		ilSetError(IL_INVALID_EXTENSION);
		return bRaw;
	}*/

	RawFile = iopenr(FileName);
	if (RawFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bRaw;
	}

	bRaw = ilLoadRawF(RawFile);
	icloser(RawFile);

	return bRaw;
}


//! Reads an already-opened raw file
ILboolean ilLoadRawF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadRawInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a raw memory "lump"
ILboolean ilLoadRawL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadRawInternal();
}


// Internal function to load a raw image
ILboolean iLoadRawInternal()
{
	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	iread(&iCurImage->Width, sizeof(ILuint), 1);
	iread(&iCurImage->Height, sizeof(ILuint), 1);
	iread(&iCurImage->Depth, sizeof(ILuint), 1);
	iread(&iCurImage->Bpp, sizeof(ILubyte), 1);
	if (iread(&iCurImage->Bpc, sizeof(ILubyte), 1) != 1)
		return IL_FALSE;

	if (!ilTexImage(iCurImage->Width, iCurImage->Height, iCurImage->Depth, iCurImage->Bpp, 0, ilGetTypeBpc(iCurImage->Bpc), NULL)) {
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	// Tries to read the correct amount of data
	if (iread(iCurImage->Data, 1, iCurImage->SizeOfData) < iCurImage->SizeOfData)
		return IL_FALSE;

	if (ilIsEnabled(IL_ORIGIN_SET)) {
		iCurImage->Origin = ilGetInteger(IL_ORIGIN_MODE);
	}
	else {
		iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	if (iCurImage->Bpp == 1)
		iCurImage->Format = IL_LUMINANCE;
	else if (iCurImage->Bpp == 3)
		iCurImage->Format = IL_RGB;
	else  // 4
		iCurImage->Format = IL_RGBA;

	ilFixImage();

	return IL_TRUE;
}


//! Writes a Raw file
ILboolean ilSaveRaw(const ILstring FileName)
{
	ILHANDLE	RawFile;
	ILboolean	bRaw = IL_FALSE;

	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	RawFile = iopenw(FileName);
	if (RawFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bRaw;
	}

	bRaw = ilSaveRawF(RawFile);
	iclosew(RawFile);

	return bRaw;
}


//! Writes raw data to an already-opened file
ILboolean ilSaveRawF(ILHANDLE File)
{
	iSetOutputFile(File);
	return iSaveRawInternal();
}


//! Writes raw data to a memory "lump"
ILboolean ilSaveRawL(ILvoid *Lump, ILuint Size)
{
	iSetOutputLump(Lump, Size);
	return iSaveRawInternal();
}


// Internal function used to load the raw data.
ILboolean iSaveRawInternal()
{
	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SaveLittleUInt(iCurImage->Width);
	SaveLittleUInt(iCurImage->Height);
	SaveLittleUInt(iCurImage->Depth);
	iputc(iCurImage->Bpp);
	iputc(iCurImage->Bpc);
	iwrite(iCurImage->Data, 1, iCurImage->SizeOfData);

	return IL_TRUE;
}


#endif//IL_NO_RAW
