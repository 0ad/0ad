//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_utilities.c
//
// Description: Utility functions
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"


ILvoid ILAPIENTRY iluDeleteImage(ILuint Id)
{
	ilDeleteImages(1, &Id);
	return;
}


ILuint ILAPIENTRY iluGenImage()
{
	ILuint Id;
	ilGenImages(1, &Id);
	ilBindImage(Id);
	return Id;
}


//! Retrieves information about the current bound image.
ILvoid ILAPIENTRY iluGetImageInfo(ILinfo *Info)
{
	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL || Info == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return;
	}

	Info->Id			= ilGetCurName();
	Info->Data			= ilGetData();
	Info->Width			= iluCurImage->Width;
	Info->Height		= iluCurImage->Height;
	Info->Depth			= iluCurImage->Depth;
	Info->Bpp			= iluCurImage->Bpp;
	Info->SizeOfData	= iluCurImage->SizeOfData;
	Info->Format		= iluCurImage->Format;
	Info->Type			= iluCurImage->Type;
	Info->Origin		= iluCurImage->Origin;
	Info->Palette		= iluCurImage->Pal.Palette;
	Info->PalType		= iluCurImage->Pal.PalType;
	Info->PalSize		= iluCurImage->Pal.PalSize;
	Info->NumNext		= iluCurImage->NumNext;
	Info->NumMips		= iluCurImage->NumMips;
	Info->NumLayers		= iluCurImage->NumLayers;

	return;
}
