//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_mdl.c
//
// Description: Reads a Half-Life model file (.mdl).
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_MDL
#include "il_mdl.h"


ILboolean iLoadMdlInternal(ILvoid);


//! Reads a .Mdl file
ILboolean ilLoadMdl(const ILstring FileName)
{
	ILHANDLE	MdlFile;
	ILboolean	bMdl = IL_FALSE;

	MdlFile = iopenr(FileName);
	if (MdlFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bMdl;
	}

	bMdl = ilLoadMdlF(MdlFile);
	icloser(MdlFile);

	return bMdl;
}


//! Reads an already-opened .Mdl file
ILboolean ilLoadMdlF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadMdlInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .Mdl
ILboolean ilLoadMdlL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadMdlInternal();
}


ILboolean iLoadMdlInternal()
{
	ILuint		Id, Version, NumTex, TexOff, TexDataOff, Position, ImageNum;
	ILubyte		*TempPal;
	TEX_HEAD	TexHead;
	ILimage		*BaseImage=NULL;
	ILboolean	BaseCreated = IL_FALSE;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Id = GetLittleUInt();
	Version = GetLittleUInt();

	// 0x54534449 == "IDST"
	if (Id != 0x54534449 || Version != 10) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Skips the actual model header.
	iseek(172, IL_SEEK_CUR);

	NumTex = GetLittleUInt();
	TexOff = GetLittleUInt();
	TexDataOff = GetLittleUInt();

	if (NumTex == 0 || TexOff == 0 || TexDataOff == 0) {
		ilSetError(IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	iseek(TexOff, IL_SEEK_SET);

	for (ImageNum = 0; ImageNum < NumTex; ImageNum++) {
		if (iread(TexHead.Name, 1, 64) != 64)
			return IL_FALSE;
		TexHead.Flags = GetLittleUInt();
		TexHead.Width = GetLittleUInt();
		TexHead.Height = GetLittleUInt();
		TexHead.Offset = GetLittleUInt();
		Position = itell();

		if (TexHead.Offset == 0) {
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}

		if (!BaseCreated) {
			ilTexImage(TexHead.Width, TexHead.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL);
			iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;
			BaseCreated = IL_TRUE;
			BaseImage = iCurImage;
			iCurImage->NumNext = NumTex - 1;  // Don't count the first image.
		}
		else {
			iCurImage->Next = ilNewImage(TexHead.Width, TexHead.Height, 1, 1, 1);
			iCurImage = iCurImage->Next;
			iCurImage->Format = IL_COLOUR_INDEX;
			iCurImage->Type = IL_UNSIGNED_BYTE;
		}

		TempPal	= (ILubyte*)ialloc(768);
		if (TempPal == NULL) {
			iCurImage = BaseImage;
			return IL_FALSE;
		}
		iCurImage->Pal.Palette = TempPal;
		iCurImage->Pal.PalSize = 768;
		iCurImage->Pal.PalType = IL_PAL_RGB24;

		iseek(TexHead.Offset, IL_SEEK_SET);
		if (iread(iCurImage->Data, TexHead.Width * TexHead.Height, 1) != 1)
			return IL_FALSE;
		if (iread(iCurImage->Pal.Palette, 1, 768) != 768)
			return IL_FALSE;

		if (ilGetBoolean(IL_CONV_PAL) == IL_TRUE) {
			ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
		}

		iseek(Position, IL_SEEK_SET);
	}

	iCurImage = BaseImage;

	ilFixImage();

	return IL_TRUE;
}

#endif//IL_NO_MDL
