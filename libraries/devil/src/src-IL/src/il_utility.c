//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_utility.c
//
// Description: Utility functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"


// Returns the bpp of any Format
ILAPI ILubyte ILAPIENTRY ilGetBppFormat(ILenum Format)
{
	switch (Format)
	{
		case IL_COLOUR_INDEX:
			return 1;
		case IL_LUMINANCE:
			return 1;
		case IL_LUMINANCE_ALPHA:
			return 2;
		case IL_RGB:
			return 3;
		case IL_BGR:
			return 3;
		case IL_RGBA:
			return 4;
		case IL_BGRA:
			return 4;
	}
	return 0;
}


// Returns the bpp of any Type
ILAPI ILubyte ILAPIENTRY ilGetBppType(ILenum Type)
{
	switch (Type)
	{
		case IL_BYTE:
			return 1;
		case IL_UNSIGNED_BYTE:
			return 1;
		case IL_SHORT:
			return 2;
		case IL_UNSIGNED_SHORT:
			return 2;
		case IL_INT:
			return 4;
		case IL_UNSIGNED_INT:
			return 4;
		case IL_FLOAT:
			return 4;
		case IL_DOUBLE:
			return 8;
	}
	return 0;
}


// Returns the type matching a bpc
ILAPI ILenum ILAPIENTRY ilGetTypeBpc(ILubyte Bpc)
{
	switch (Bpc)
	{
		case 1:
			return IL_UNSIGNED_BYTE;
		case 2:
			return IL_UNSIGNED_SHORT;
		case 4:
			return IL_UNSIGNED_INT;
		case 8:
			return IL_DOUBLE;
	}
	return 0;
}


// Returns the bpp of any palette type (PalType)
ILAPI ILubyte ILAPIENTRY ilGetBppPal(ILenum PalType)
{
	switch (PalType)
	{
		case IL_PAL_RGB24:
			return 3;
		case IL_PAL_BGR24:
			return 3;
		case IL_PAL_RGB32:
			return 4;
		case IL_PAL_RGBA32:
			return 4;
		case IL_PAL_BGR32:
			return 4;
		case IL_PAL_BGRA32:
			return 4;
	}
	return 0;
}


// Returns the base format of a palette type (PalType)
ILAPI ILenum ILAPIENTRY ilGetPalBaseType(ILenum PalType)
{
	switch (PalType)
	{
		case IL_PAL_RGB24:
			return IL_RGB;
		case IL_PAL_RGB32:
			return IL_RGBA;  // Not sure
		case IL_PAL_RGBA32:
			return IL_RGBA;
		case IL_PAL_BGR24:
			return IL_BGR;
		case IL_PAL_BGR32:
			return IL_BGRA;  // Not sure
		case IL_PAL_BGRA32:
			return IL_BGRA;
	}

	return 0;
}


// Returns the next power of 2 if Num isn't 2^n or returns Num if Num is 2^n
ILAPI ILuint ILAPIENTRY ilNextPower2(ILuint Num)
{
	ILuint Power2 = 1;
	if (Num == 0) return 1;
	for (; Power2 < Num; Power2 <<= 1);
	return Power2;
}
