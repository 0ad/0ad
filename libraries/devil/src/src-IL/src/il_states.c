//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/24/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_states.c
//
// Description: State machine
//
//
// 20040223 XIX : now has a ilPngAlphaIndex member, so we can spit out png files with a transparent index, set to -1 for none
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_states.h"
//#include <malloc.h>
#include <stdlib.h>

const ILstring _ilVendor		= IL_TEXT("Abysmal Software");
const ILstring _ilVersion		= IL_TEXT("Developer's Image Library (DevIL) 1.6.7 ") IL_TEXT(__DATE__);

const ILstring _ilLoadExt		= IL_TEXT("") IL_BMP_EXT IL_CUT_EXT IL_DCX_EXT IL_DDS_EXT
									IL_GIF_EXT IL_ICO_EXT IL_JPG_EXT IL_LIF_EXT
									IL_MDL_EXT IL_MNG_EXT IL_PCX_EXT IL_PIC_EXT
									IL_PIX_EXT IL_PNG_EXT IL_PNM_EXT IL_PSD_EXT
									IL_PSP_EXT IL_PXR_EXT IL_SGI_EXT IL_TGA_EXT
									IL_TIF_EXT IL_WAL_EXT IL_XPM_EXT;
const ILstring _ilSaveExt		= IL_TEXT("") IL_BMP_EXT IL_CHEAD_EXT IL_DDS_EXT IL_JPG_EXT
									IL_PCX_EXT IL_PNG_EXT IL_PNM_EXT IL_PSD_EXT
									IL_SGI_EXT IL_TGA_EXT IL_TIF_EXT;


//! Set all states to their defaults.
ILvoid ilDefaultStates()
{
	ilStates[ilCurrentPos].ilOriginSet = IL_FALSE;
	ilStates[ilCurrentPos].ilOriginMode = IL_ORIGIN_LOWER_LEFT;
	ilStates[ilCurrentPos].ilFormatSet = IL_FALSE;
	ilStates[ilCurrentPos].ilFormatMode = IL_BGRA;
	ilStates[ilCurrentPos].ilTypeSet = IL_FALSE;
	ilStates[ilCurrentPos].ilTypeMode = IL_UNSIGNED_BYTE;
	ilStates[ilCurrentPos].ilOverWriteFiles = IL_FALSE;
	ilStates[ilCurrentPos].ilAutoConvPal = IL_FALSE;
	ilStates[ilCurrentPos].ilDefaultOnFail = IL_FALSE;
	ilStates[ilCurrentPos].ilUseKeyColour = IL_FALSE;
	ilStates[ilCurrentPos].ilCompression = IL_COMPRESS_ZLIB;
	ilStates[ilCurrentPos].ilInterlace = IL_FALSE;

	ilStates[ilCurrentPos].ilTgaCreateStamp = IL_FALSE;
	ilStates[ilCurrentPos].ilJpgQuality = 99;
	ilStates[ilCurrentPos].ilPngInterlace = IL_FALSE;
	ilStates[ilCurrentPos].ilTgaRle = IL_FALSE;
	ilStates[ilCurrentPos].ilBmpRle = IL_FALSE;
	ilStates[ilCurrentPos].ilSgiRle = IL_FALSE;
	ilStates[ilCurrentPos].ilJpgFormat = IL_JFIF;
	ilStates[ilCurrentPos].ilDxtcFormat = IL_DXT1;
	ilStates[ilCurrentPos].ilPcdPicNum = 2;
	ilStates[ilCurrentPos].ilPngAlphaIndex = -1;

	ilStates[ilCurrentPos].ilTgaId = NULL;
	ilStates[ilCurrentPos].ilTgaAuthName = NULL;
	ilStates[ilCurrentPos].ilTgaAuthComment = NULL;
	ilStates[ilCurrentPos].ilPngAuthName = NULL;
	ilStates[ilCurrentPos].ilPngTitle = NULL;
	ilStates[ilCurrentPos].ilPngDescription = NULL;

	//2003-09-01: added tiff strings
	ilStates[ilCurrentPos].ilTifDescription = NULL;
	ilStates[ilCurrentPos].ilTifHostComputer = NULL;
	ilStates[ilCurrentPos].ilTifDocumentName = NULL;
	ilStates[ilCurrentPos].ilTifAuthName = NULL;
	ilStates[ilCurrentPos].ilCHeader = NULL;

	ilStates[ilCurrentPos].ilQuantMode = IL_WU_QUANT;
	ilStates[ilCurrentPos].ilNeuSample = 15;
	ilStates[ilCurrentPos].ilQuantMaxIndexs = 256;

	ilStates[ilCurrentPos].ilKeepDxtcData = IL_FALSE;




	ilHints.MemVsSpeedHint = IL_FASTEST;
	ilHints.CompressHint = IL_USE_COMPRESSION;

	while (ilGetError() != IL_NO_ERROR);

	return;
}


//! Returns a constant string detailing aspects about this library.
const ILstring ILAPIENTRY ilGetString(ILenum StringName)
{
	switch (StringName)
	{
		case IL_VENDOR:
			return (const ILstring)_ilVendor;
		case IL_VERSION_NUM: //changed 2003-08-30: IL_VERSION changes
												//to reflect the il version...not a good
												//switch define ;-)
			return (const ILstring)_ilVersion;
		case IL_LOAD_EXT:
			return (const ILstring)_ilLoadExt;
		case IL_SAVE_EXT:
			return (const ILstring)_ilSaveExt;
		case IL_TGA_ID_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilTgaId;
		case IL_TGA_AUTHNAME_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilTgaAuthName;
		case IL_TGA_AUTHCOMMENT_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilTgaAuthComment;
		case IL_PNG_AUTHNAME_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilPngAuthName;
		case IL_PNG_TITLE_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilPngTitle;
		case IL_PNG_DESCRIPTION_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilPngDescription;
		//2003-08-31: added tif strings
		case IL_TIF_DESCRIPTION_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilTifDescription;
		case IL_TIF_HOSTCOMPUTER_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilTifHostComputer;
		case IL_TIF_DOCUMENTNAME_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilTifDocumentName;
		case IL_TIF_AUTHNAME_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilTifAuthName;
		case IL_CHEAD_HEADER_STRING:
			return (const ILstring)ilStates[ilCurrentPos].ilCHeader;
		default:
			ilSetError(IL_INVALID_ENUM);
			break;
	}
	return NULL;
}


// Clips a string to a certain length and returns a new string.
char *iClipString(char *String, ILuint MaxLen)
{
	char	*Clipped;
	ILuint	Length;

	if (String == NULL)
		return NULL;

	Length = strlen(String);

	Clipped = (char*)ialloc(MaxLen + 1);  // Terminating NULL makes it +1.
	if (Clipped == NULL) {
		return NULL;
	}

	memcpy(Clipped, String, Length);
	Clipped[Length] = 0;

	return Clipped;
}


// Returns format-specific strings, truncated to MaxLen (not counting the terminating NULL).
char *iGetString(ILenum StringName)
{
	switch (StringName)
	{
		case IL_TGA_ID_STRING:
			return iClipString(ilStates[ilCurrentPos].ilTgaId, 254);
		case IL_TGA_AUTHNAME_STRING:
			return iClipString(ilStates[ilCurrentPos].ilTgaAuthName, 40);
		case IL_TGA_AUTHCOMMENT_STRING:
			return iClipString(ilStates[ilCurrentPos].ilTgaAuthComment, 80);
		case IL_PNG_AUTHNAME_STRING:
			return iClipString(ilStates[ilCurrentPos].ilPngAuthName, 255);
		case IL_PNG_TITLE_STRING:
			return iClipString(ilStates[ilCurrentPos].ilPngTitle, 255);
		case IL_PNG_DESCRIPTION_STRING:
			return iClipString(ilStates[ilCurrentPos].ilPngDescription, 255);

		//changed 2003-08-31...here was a serious copy and paste bug ;-)
		case IL_TIF_DESCRIPTION_STRING:
			return iClipString(ilStates[ilCurrentPos].ilTifDescription, 255);
		case IL_TIF_HOSTCOMPUTER_STRING:
			return iClipString(ilStates[ilCurrentPos].ilTifHostComputer, 255);
		case IL_TIF_DOCUMENTNAME_STRING:
			return iClipString(ilStates[ilCurrentPos].ilTifDocumentName, 255);
		case IL_TIF_AUTHNAME_STRING:
			return iClipString(ilStates[ilCurrentPos].ilTifAuthName, 255);
		case IL_CHEAD_HEADER_STRING:
			return iClipString(ilStates[ilCurrentPos].ilCHeader, 32);
		default:
			ilSetError(IL_INVALID_ENUM);
	}
	return NULL;
}


//! Enables a mode
ILboolean ILAPIENTRY ilEnable(ILenum Mode)
{
	return ilAble(Mode, IL_TRUE);
}


//! Disables a mode
ILboolean ILAPIENTRY ilDisable(ILenum Mode)
{
	return ilAble(Mode, IL_FALSE);
}


// Internal function that sets the Mode equal to Flag
ILboolean ilAble(ILenum Mode, ILboolean Flag)
{
	switch (Mode)
	{
		case IL_ORIGIN_SET:
			ilStates[ilCurrentPos].ilOriginSet = Flag;
			break;
		case IL_FORMAT_SET:
			ilStates[ilCurrentPos].ilFormatSet = Flag;
			break;
		case IL_TYPE_SET:
			ilStates[ilCurrentPos].ilTypeSet = Flag;
			break;
		case IL_FILE_OVERWRITE:
			ilStates[ilCurrentPos].ilOverWriteFiles = Flag;
			break;
		case IL_CONV_PAL:
			ilStates[ilCurrentPos].ilAutoConvPal = Flag;
			break;
		case IL_DEFAULT_ON_FAIL:
			ilStates[ilCurrentPos].ilDefaultOnFail = Flag;
			break;
		case IL_USE_KEY_COLOUR:
			ilStates[ilCurrentPos].ilUseKeyColour = Flag;
			break;
		case IL_SAVE_INTERLACED:
			ilStates[ilCurrentPos].ilInterlace = Flag;
			break;

		default:
			ilSetError(IL_INVALID_ENUM);
			return IL_FALSE;
	}

	return IL_TRUE;
}


//! Checks whether the mode is enabled.
ILboolean ILAPIENTRY ilIsEnabled(ILenum Mode)
{
	switch (Mode)
	{
		case IL_ORIGIN_SET:
			return ilStates[ilCurrentPos].ilOriginSet;
		case IL_FORMAT_SET:
			return ilStates[ilCurrentPos].ilFormatSet;
		case IL_TYPE_SET:
			return ilStates[ilCurrentPos].ilTypeSet;
		case IL_FILE_OVERWRITE:
			return ilStates[ilCurrentPos].ilOverWriteFiles;
		case IL_CONV_PAL:
			return ilStates[ilCurrentPos].ilAutoConvPal;
		case IL_DEFAULT_ON_FAIL:
			return ilStates[ilCurrentPos].ilDefaultOnFail;
		case IL_USE_KEY_COLOUR:
			return ilStates[ilCurrentPos].ilUseKeyColour;

		default:
			ilSetError(IL_INVALID_ENUM);
	}

	return IL_FALSE;
}


//! Checks whether the mode is disabled.
ILboolean ILAPIENTRY ilIsDisabled(ILenum Mode)
{
	return !ilIsEnabled(Mode);
}


//! Sets Param equal to the current value of the Mode
ILvoid ILAPIENTRY ilGetBooleanv(ILenum Mode, ILboolean *Param)
{
	if (Param == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return;
	}

	*Param = ilGetInteger(Mode);

	return;
}


//! Returns the current value of the Mode
ILboolean ILAPIENTRY ilGetBoolean(ILenum Mode)
{
	ILboolean Temp;
	Temp = IL_FALSE;
	ilGetBooleanv(Mode, &Temp);
	return Temp;
}


ILimage *iGetBaseImage(ILvoid);

ILuint iGetActiveNum(ILenum Type)
{
	ILimage *BaseImage;
	ILuint Num = 0;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return 0;
	}

	BaseImage = iGetBaseImage();

	if (BaseImage == iCurImage)
		return 0;

	switch (Type)
	{
		case IL_ACTIVE_IMAGE:
			BaseImage = BaseImage->Next;
			break;
		case IL_ACTIVE_MIPMAP:
			BaseImage = BaseImage->Mipmaps;
			break;
		case IL_ACTIVE_LAYER:
			BaseImage = BaseImage->Layers;
			break;
	}

	do {
		if (BaseImage == NULL)
			return 0;
		Num++;
		if (BaseImage == iCurImage)
			return Num;
	} while ((BaseImage = BaseImage->Next));

	//ilSetError(IL_ILLEGAL_OPERATION);

	return 0;
}


//! Sets Param equal to the current value of the Mode
ILvoid ILAPIENTRY ilGetIntegerv(ILenum Mode, ILint *Param)
{
	if (Param == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return;
	}

	*Param = 0;

	switch (Mode)
	{
		// Integer values
		case IL_VERSION_NUM:
			*Param = IL_VERSION;
			break;
		case IL_ORIGIN_MODE:
			*Param = ilStates[ilCurrentPos].ilOriginMode;
			break;
		case IL_FORMAT_MODE:
			*Param = ilStates[ilCurrentPos].ilFormatMode;
			break;
		case IL_TYPE_MODE:
			*Param = ilStates[ilCurrentPos].ilTypeMode;
			break;
		case IL_COMPRESS_MODE:
			*Param = ilStates[ilCurrentPos].ilCompression;
			break;
		case IL_INTERLACE_MODE:
			*Param = ilStates[ilCurrentPos].ilInterlace;
			break;
		case IL_QUANTIZATION_MODE:
			*Param = ilStates[ilCurrentPos].ilQuantMode;
			break;
		case IL_NEU_QUANT_SAMPLE:
			*Param = ilStates[ilCurrentPos].ilNeuSample;
			break;
		case IL_MAX_QUANT_INDEXS:
			*Param = ilStates[ilCurrentPos].ilQuantMaxIndexs;
			break;
		case IL_KEEP_DXTC_DATA:
			*Param = ilStates[ilCurrentPos].ilKeepDxtcData;
			break;

		case IL_IMAGE_WIDTH:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Width;
			break;
		case IL_IMAGE_HEIGHT:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Height;
			break;
		case IL_IMAGE_DEPTH:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Depth;
			break;
		case IL_IMAGE_BYTES_PER_PIXEL:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Bpp;
			break;
		case IL_IMAGE_BITS_PER_PIXEL:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Bpp << 3;
			break;
		case IL_IMAGE_SIZE_OF_DATA:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->SizeOfData;
			break;
		case IL_IMAGE_FORMAT:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Format;
			break;
		case IL_IMAGE_TYPE:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Type;
			break;
		case IL_NUM_IMAGES:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->NumNext;
			break;
		case IL_NUM_MIPMAPS:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->NumMips;
			break;
		case IL_NUM_LAYERS:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->NumMips;
			break;
		case IL_IMAGE_DURATION:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Duration;
			break;
		case IL_IMAGE_PLANESIZE:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->SizeOfPlane;
			break;
		case IL_IMAGE_BPC:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Bpc;
			break;
		case IL_IMAGE_OFFX:
		case IL_IMAGE_OFFY:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			if (Mode == IL_IMAGE_OFFX)
				*Param = iCurImage->OffX;
			else  // IL_IMAGE_OFFY
				*Param = iCurImage->OffY;
			break;
		case IL_IMAGE_CUBEFLAGS:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->CubeFlags;
			break;
		case IL_IMAGE_ORIGIN:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Origin;
			break;

		case IL_ACTIVE_IMAGE:
		case IL_ACTIVE_MIPMAP:
		case IL_ACTIVE_LAYER:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iGetActiveNum(Mode);
			break;

		case IL_CUR_IMAGE:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = ilGetCurName();
			break;

		case IL_PALETTE_TYPE:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = iCurImage->Pal.PalType;
			break;
		case IL_PALETTE_BPP:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = ilGetBppPal(iCurImage->Pal.PalType);
			break;
		case IL_PALETTE_NUM_COLS:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			if (!iCurImage->Pal.Palette || !iCurImage->Pal.PalSize || iCurImage->Pal.PalType == IL_PAL_NONE)
				*Param = 0;
			else
				*Param = iCurImage->Pal.PalSize / ilGetBppPal(iCurImage->Pal.PalType);
			break;
		case IL_PALETTE_BASE_TYPE:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			switch (iCurImage->Pal.PalType)
			{
				case IL_PAL_RGB24:
					*Param = IL_RGB;
				case IL_PAL_RGB32:
					*Param = IL_RGBA;  // Not sure
				case IL_PAL_RGBA32:
					*Param = IL_RGBA;
				case IL_PAL_BGR24:
					*Param = IL_BGR;
				case IL_PAL_BGR32:
					*Param = IL_BGRA;  // Not sure
				case IL_PAL_BGRA32:
					*Param = IL_BGRA;
			}
			break;
		case IL_DXTC_DATA_FORMAT:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			if (iCurImage->DxtcData == NULL || iCurImage->DxtcSize == 0) {
				*Param = IL_DXT_NO_COMP;
				break;
			}
			*Param = iCurImage->DxtcFormat;
			break;


		// Format-specific values
		case IL_TGA_CREATE_STAMP:
			*Param = ilStates[ilCurrentPos].ilTgaCreateStamp;
			break;
		case IL_JPG_QUALITY:
			*Param = ilStates[ilCurrentPos].ilJpgQuality;
			break;
		case IL_PNG_INTERLACE:
			*Param = ilStates[ilCurrentPos].ilPngInterlace;
			break;
		case IL_TGA_RLE:
			*Param = ilStates[ilCurrentPos].ilTgaRle;
			break;
		case IL_BMP_RLE:
			*Param = ilStates[ilCurrentPos].ilBmpRle;
			break;
		case IL_SGI_RLE:
			*Param = ilStates[ilCurrentPos].ilSgiRle;
			break;
		case IL_JPG_SAVE_FORMAT:
			*Param = ilStates[ilCurrentPos].ilJpgFormat;
			break;
		case IL_DXTC_FORMAT:
			*Param = ilStates[ilCurrentPos].ilDxtcFormat;
			break;
		case IL_PCD_PICNUM:
			*Param = ilStates[ilCurrentPos].ilPcdPicNum;
			break;

		case IL_PNG_ALPHA_INDEX:
			*Param = ilStates[ilCurrentPos].ilPngAlphaIndex;
			break;


		// Boolean values
		case IL_CONV_PAL:
			*Param = ilStates[ilCurrentPos].ilAutoConvPal;
			break;
		case IL_DEFAULT_ON_FAIL:
			*Param = ilStates[ilCurrentPos].ilDefaultOnFail;
			break;
		case IL_FILE_MODE:
			*Param = ilStates[ilCurrentPos].ilOverWriteFiles;
			break;
		case IL_FORMAT_SET:
			*Param = ilStates[ilCurrentPos].ilFormatSet;
			break;
		case IL_ORIGIN_SET:
			*Param = ilStates[ilCurrentPos].ilOriginSet;
			break;
		case IL_TYPE_SET:
			*Param = ilStates[ilCurrentPos].ilTypeSet;
			break;
		case IL_USE_KEY_COLOUR:
			*Param = ilStates[ilCurrentPos].ilUseKeyColour;
			break;


		default:
			ilSetError(IL_INVALID_ENUM);
	}

	return;
}


//! Returns the current value of the Mode
ILint ILAPIENTRY ilGetInteger(ILenum Mode)
{
	ILint Temp;
	Temp = 0;
	ilGetIntegerv(Mode, &Temp);
	return Temp;
}


//! Sets the default origin to be used.
ILboolean ILAPIENTRY ilOriginFunc(ILenum Mode)
{
	switch (Mode)
	{
		case IL_ORIGIN_LOWER_LEFT:
		case IL_ORIGIN_UPPER_LEFT:
			ilStates[ilCurrentPos].ilOriginMode = Mode;
			break;
		default:
			ilSetError(IL_INVALID_PARAM);
			return IL_FALSE;
	}
	return IL_TRUE;
}


//! Sets the default format to be used.
ILboolean ILAPIENTRY ilFormatFunc(ILenum Mode)
{
	switch (Mode)
	{
		//case IL_COLOUR_INDEX:
		case IL_RGB:
		case IL_RGBA:
		case IL_BGR:
		case IL_BGRA:
		case IL_LUMINANCE:
			ilStates[ilCurrentPos].ilFormatMode = Mode;
			break;
		default:
			ilSetError(IL_INVALID_PARAM);
			return IL_FALSE;
	}
	return IL_TRUE;
}


//! Sets the default type to be used.
ILboolean ILAPIENTRY ilTypeFunc(ILenum Mode)
{
	switch (Mode)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
		case IL_INT:
		case IL_UNSIGNED_INT:
		case IL_FLOAT:
		case IL_DOUBLE:
			ilStates[ilCurrentPos].ilTypeMode = Mode;
			break;
		default:
			ilSetError(IL_INVALID_PARAM);
			return IL_FALSE;
	}
	return IL_TRUE;
}


ILboolean ILAPIENTRY ilCompressFunc(ILenum Mode)
{
	switch (Mode)
	{
		case IL_COMPRESS_NONE:
		case IL_COMPRESS_RLE:
		//case IL_COMPRESS_LZO:
			case IL_COMPRESS_ZLIB:
			ilStates[ilCurrentPos].ilCompression = Mode;
			break;
		default:
			ilSetError(IL_INVALID_PARAM);
			return IL_FALSE;
	}
	return IL_TRUE;
}


//! Pushes the states indicated by Bits onto the state stack
ILvoid ILAPIENTRY ilPushAttrib(ILuint Bits)
{
	// Should we check here to see if ilCurrentPos is negative?

	if (ilCurrentPos >= IL_ATTRIB_STACK_MAX - 1) {
		ilCurrentPos = IL_ATTRIB_STACK_MAX - 1;
		ilSetError(IL_STACK_OVERFLOW);
		return;
	}

	ilCurrentPos++;

	//	memcpy(&ilStates[ilCurrentPos], &ilStates[ilCurrentPos - 1], sizeof(IL_STATES));

	ilDefaultStates();

	if (Bits & IL_ORIGIN_BIT) {
		ilStates[ilCurrentPos].ilOriginMode = ilStates[ilCurrentPos-1].ilOriginMode;
		ilStates[ilCurrentPos].ilOriginSet  = ilStates[ilCurrentPos-1].ilOriginSet;
	}
	if (Bits & IL_FORMAT_BIT) {
		ilStates[ilCurrentPos].ilFormatMode = ilStates[ilCurrentPos-1].ilFormatMode;
		ilStates[ilCurrentPos].ilFormatSet  = ilStates[ilCurrentPos-1].ilFormatSet;
	}
	if (Bits & IL_TYPE_BIT) {
		ilStates[ilCurrentPos].ilTypeMode = ilStates[ilCurrentPos-1].ilTypeMode;
		ilStates[ilCurrentPos].ilTypeSet  = ilStates[ilCurrentPos-1].ilTypeSet;
	}
	if (Bits & IL_FILE_BIT) {
		ilStates[ilCurrentPos].ilOverWriteFiles = ilStates[ilCurrentPos-1].ilOverWriteFiles;
	}
	if (Bits & IL_PAL_BIT) {
		ilStates[ilCurrentPos].ilAutoConvPal = ilStates[ilCurrentPos-1].ilAutoConvPal;
	}
	if (Bits & IL_LOADFAIL_BIT) {
		ilStates[ilCurrentPos].ilDefaultOnFail = ilStates[ilCurrentPos-1].ilDefaultOnFail;
	}
	if (Bits & IL_COMPRESS_BIT) {
		ilStates[ilCurrentPos].ilCompression = ilStates[ilCurrentPos-1].ilCompression;
	}
	if (Bits & IL_FORMAT_SPECIFIC_BIT) {
		ilStates[ilCurrentPos].ilTgaCreateStamp = ilStates[ilCurrentPos-1].ilTgaCreateStamp;
		ilStates[ilCurrentPos].ilJpgQuality = ilStates[ilCurrentPos-1].ilJpgQuality;
		ilStates[ilCurrentPos].ilPngInterlace = ilStates[ilCurrentPos-1].ilPngInterlace;
		ilStates[ilCurrentPos].ilTgaRle = ilStates[ilCurrentPos-1].ilTgaRle;
		ilStates[ilCurrentPos].ilBmpRle = ilStates[ilCurrentPos-1].ilBmpRle;
		ilStates[ilCurrentPos].ilSgiRle = ilStates[ilCurrentPos-1].ilSgiRle;
		ilStates[ilCurrentPos].ilJpgFormat = ilStates[ilCurrentPos-1].ilJpgFormat;
		ilStates[ilCurrentPos].ilDxtcFormat = ilStates[ilCurrentPos-1].ilDxtcFormat;
		ilStates[ilCurrentPos].ilPcdPicNum = ilStates[ilCurrentPos-1].ilPcdPicNum;

		ilStates[ilCurrentPos].ilPngAlphaIndex = ilStates[ilCurrentPos-1].ilPngAlphaIndex;

		// Strings
		if (ilStates[ilCurrentPos].ilTgaId)
			ifree(ilStates[ilCurrentPos].ilTgaId);
		if (ilStates[ilCurrentPos].ilTgaAuthName)
			ifree(ilStates[ilCurrentPos].ilTgaAuthName);
		if (ilStates[ilCurrentPos].ilTgaAuthComment)
			ifree(ilStates[ilCurrentPos].ilTgaAuthComment);
		if (ilStates[ilCurrentPos].ilPngAuthName)
			ifree(ilStates[ilCurrentPos].ilPngAuthName);
		if (ilStates[ilCurrentPos].ilPngTitle)
			ifree(ilStates[ilCurrentPos].ilPngTitle);
		if (ilStates[ilCurrentPos].ilPngDescription)
			ifree(ilStates[ilCurrentPos].ilPngDescription);

		//2003-09-01: added tif strings
		if (ilStates[ilCurrentPos].ilTifDescription)
			ifree(ilStates[ilCurrentPos].ilTifDescription);
		if (ilStates[ilCurrentPos].ilTifHostComputer)
			ifree(ilStates[ilCurrentPos].ilTifHostComputer);
		if (ilStates[ilCurrentPos].ilTifDocumentName)
			ifree(ilStates[ilCurrentPos].ilTifDocumentName);
		if (ilStates[ilCurrentPos].ilTifAuthName)
			ifree(ilStates[ilCurrentPos].ilTifAuthName);

		if (ilStates[ilCurrentPos].ilCHeader)
			ifree(ilStates[ilCurrentPos].ilCHeader);

		ilStates[ilCurrentPos].ilTgaId = ilStrDup(ilStates[ilCurrentPos-1].ilTgaId);
		ilStates[ilCurrentPos].ilTgaAuthName = ilStrDup(ilStates[ilCurrentPos-1].ilTgaAuthName);
		ilStates[ilCurrentPos].ilTgaAuthComment = ilStrDup(ilStates[ilCurrentPos-1].ilTgaAuthComment);
		ilStates[ilCurrentPos].ilPngAuthName = ilStrDup(ilStates[ilCurrentPos-1].ilPngAuthName);
		ilStates[ilCurrentPos].ilPngTitle = ilStrDup(ilStates[ilCurrentPos-1].ilPngTitle);
		ilStates[ilCurrentPos].ilPngDescription = ilStrDup(ilStates[ilCurrentPos-1].ilPngDescription);

		//2003-09-01: added tif strings
		ilStates[ilCurrentPos].ilTifDescription = ilStrDup(ilStates[ilCurrentPos-1].ilTifDescription);
		ilStates[ilCurrentPos].ilTifHostComputer = ilStrDup(ilStates[ilCurrentPos-1].ilTifHostComputer);
		ilStates[ilCurrentPos].ilTifDocumentName = ilStrDup(ilStates[ilCurrentPos-1].ilTifDocumentName);
		ilStates[ilCurrentPos].ilTifAuthName = ilStrDup(ilStates[ilCurrentPos-1].ilTifAuthName);

		ilStates[ilCurrentPos].ilCHeader = ilStrDup(ilStates[ilCurrentPos-1].ilCHeader);
	}

	return;
}


// @TODO:  Find out how this affects strings!!!

//! Pops the last entry off the state stack into the current states
ILvoid ILAPIENTRY ilPopAttrib()
{
	if (ilCurrentPos <= 0) {
		ilCurrentPos = 0;
		ilSetError(IL_STACK_UNDERFLOW);
		return;
	}

	// Should we check here to see if ilCurrentPos is too large?
	ilCurrentPos--;

	return;
}


//! Specifies implementation-dependent performance hints
ILvoid ILAPIENTRY ilHint(ILenum Target, ILenum Mode)
{
	switch (Target)
	{
		case IL_MEM_SPEED_HINT:
			switch (Mode)
			{
				case IL_FASTEST:
					ilHints.MemVsSpeedHint = Mode;
					break;
				case IL_LESS_MEM:
					ilHints.MemVsSpeedHint = Mode;
					break;
				case IL_DONT_CARE:
					ilHints.MemVsSpeedHint = IL_FASTEST;
					break;
				default:
					ilSetError(IL_INVALID_ENUM);
					return;
			}
			break;

		case IL_COMPRESSION_HINT:
			switch (Mode)
			{
				case IL_USE_COMPRESSION:
					ilHints.CompressHint = Mode;
					break;
				case IL_NO_COMPRESSION:
					ilHints.CompressHint = Mode;
					break;
				case IL_DONT_CARE:
					ilHints.CompressHint = IL_NO_COMPRESSION;
					break;
				default:
					ilSetError(IL_INVALID_ENUM);
					return;
			}
			break;

			
		default:
			ilSetError(IL_INVALID_ENUM);
			return;
	}

	return;
}


ILenum iGetHint(ILenum Target)
{
	switch (Target)
	{
		case IL_MEM_SPEED_HINT:
			return ilHints.MemVsSpeedHint;
		case IL_COMPRESSION_HINT:
			return ilHints.CompressHint;
		default:
			ilSetError(IL_INTERNAL_ERROR);
			return 0;
	}
}


ILvoid ILAPIENTRY ilSetString(ILenum Mode, const char *String)
{
	if (String == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return;
	}

	switch (Mode)
	{
		case IL_TGA_ID_STRING:
			if (ilStates[ilCurrentPos].ilTgaId)
				ifree(ilStates[ilCurrentPos].ilTgaId);
			ilStates[ilCurrentPos].ilTgaId = ilStrDup(String);
			break;
		case IL_TGA_AUTHNAME_STRING:
			if (ilStates[ilCurrentPos].ilTgaAuthName)
				ifree(ilStates[ilCurrentPos].ilTgaAuthName);
			ilStates[ilCurrentPos].ilTgaAuthName = ilStrDup(String);
			break;
		case IL_TGA_AUTHCOMMENT_STRING:
			if (ilStates[ilCurrentPos].ilTgaAuthComment)
				ifree(ilStates[ilCurrentPos].ilTgaAuthComment);
			ilStates[ilCurrentPos].ilTgaAuthComment = ilStrDup(String);
			break;
		case IL_PNG_AUTHNAME_STRING:
			if (ilStates[ilCurrentPos].ilPngAuthName)
				ifree(ilStates[ilCurrentPos].ilPngAuthName);
			ilStates[ilCurrentPos].ilPngAuthName = ilStrDup(String);
			break;
		case IL_PNG_TITLE_STRING:
			if (ilStates[ilCurrentPos].ilPngTitle)
				ifree(ilStates[ilCurrentPos].ilPngTitle);
			ilStates[ilCurrentPos].ilPngTitle = ilStrDup(String);
			break;
		case IL_PNG_DESCRIPTION_STRING:
			if (ilStates[ilCurrentPos].ilPngDescription)
				ifree(ilStates[ilCurrentPos].ilPngDescription);
			ilStates[ilCurrentPos].ilPngDescription = ilStrDup(String);
			break;

		//2003-09-01: added tif strings
		case IL_TIF_DESCRIPTION_STRING:
			if (ilStates[ilCurrentPos].ilTifDescription)
				ifree(ilStates[ilCurrentPos].ilTifDescription);
			ilStates[ilCurrentPos].ilTifDescription = ilStrDup(String);
			break;
		case IL_TIF_HOSTCOMPUTER_STRING:
			if (ilStates[ilCurrentPos].ilTifHostComputer)
				ifree(ilStates[ilCurrentPos].ilTifHostComputer);
			ilStates[ilCurrentPos].ilTifHostComputer = ilStrDup(String);
			break;
		case IL_TIF_DOCUMENTNAME_STRING:
						if (ilStates[ilCurrentPos].ilTifDocumentName)
				ifree(ilStates[ilCurrentPos].ilTifDocumentName);
			ilStates[ilCurrentPos].ilTifDocumentName = ilStrDup(String);
			break;
		case IL_TIF_AUTHNAME_STRING:
			if (ilStates[ilCurrentPos].ilTifAuthName)
				ifree(ilStates[ilCurrentPos].ilTifAuthName);
			ilStates[ilCurrentPos].ilTifAuthName = ilStrDup(String);
			break;

		case IL_CHEAD_HEADER_STRING:
			if (ilStates[ilCurrentPos].ilCHeader)
				ifree(ilStates[ilCurrentPos].ilCHeader);
			ilStates[ilCurrentPos].ilCHeader = ilStrDup(String);
			break;



		default:
			ilSetError(IL_INVALID_ENUM);
	}


	return;
}


ILvoid ILAPIENTRY ilSetInteger(ILenum Mode, ILint Param)
{
	switch (Mode)
	{
		case IL_TGA_CREATE_STAMP:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				ilStates[ilCurrentPos].ilTgaCreateStamp = Param;
				return;
			}
			break;
		case IL_JPG_QUALITY:
			if (Param >= 0 && Param <= 99) {
				ilStates[ilCurrentPos].ilJpgQuality = Param;
				return;
			}
			break;
		case IL_PNG_INTERLACE:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				ilStates[ilCurrentPos].ilPngInterlace = Param;
				return;
			}
			break;
		case IL_PNG_ALPHA_INDEX:
			if (Param >= -1 || Param <= 255) {
				ilStates[ilCurrentPos].ilPngAlphaIndex=Param;
				return;
			}
			break;
		case IL_TGA_RLE:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				ilStates[ilCurrentPos].ilTgaRle = Param;
				return;
			}
			break;
		case IL_BMP_RLE:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				ilStates[ilCurrentPos].ilBmpRle = Param;
				return;
			}
			break;
		case IL_SGI_RLE:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				ilStates[ilCurrentPos].ilSgiRle = Param;
				return;
			}
			break;
		case IL_JPG_SAVE_FORMAT:
			if (Param == IL_JFIF || Param == IL_EXIF) {
				ilStates[ilCurrentPos].ilJpgFormat = Param;
				return;
			}
			break;
		case IL_DXTC_FORMAT:
			if (Param >= IL_DXT1 || Param <= IL_DXT_NO_COMP) {
				ilStates[ilCurrentPos].ilDxtcFormat = Param;
				return;
			}
			break;
		case IL_PCD_PICNUM:
			if (Param >= 0 || Param <= 2) {
				ilStates[ilCurrentPos].ilPcdPicNum = Param;
				return;
			}
			break;
		case IL_IMAGE_OFFX:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			iCurImage->OffX = Param;
			return;
		case IL_IMAGE_OFFY:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			iCurImage->OffY = Param;
			return;
		case IL_IMAGE_DURATION:
			if (iCurImage == NULL) {
				ilSetError(IL_ILLEGAL_OPERATION);
				break;
			}
			iCurImage->Duration = Param;
			return;
		case IL_FORMAT_MODE:
			ilFormatFunc(Param);
			return;
		case IL_TYPE_MODE:
			ilTypeFunc(Param);
			return;
		case IL_ORIGIN_MODE:
			ilOriginFunc(Param);
			return;
		case IL_QUANTIZATION_MODE:
			if (Param == IL_WU_QUANT || Param == IL_NEU_QUANT) {
				ilStates[ilCurrentPos].ilQuantMode = Param;
				return;
			}
			break;
		case IL_NEU_QUANT_SAMPLE:
			if (Param >= 1 && Param <= 30) {
				ilStates[ilCurrentPos].ilNeuSample = Param;
				return;
			}
			break;

		case IL_MAX_QUANT_INDEXS:
			if (Param >= 2 && Param <= 256) {
				ilStates[ilCurrentPos].ilQuantMaxIndexs = Param;
				return;
			}
			break;

		case IL_KEEP_DXTC_DATA:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				ilStates[ilCurrentPos].ilKeepDxtcData = Param;
				return;
			}
			break;


		default:
			ilSetError(IL_INVALID_ENUM);
			return;
	}

	ilSetError(IL_INVALID_PARAM);  // Parameter not in valid bounds.
	return;
}


ILint iGetInt(ILenum Mode)
{
	switch (Mode)
	{
		case IL_TGA_CREATE_STAMP:
			return (ILint)ilStates[ilCurrentPos].ilTgaCreateStamp;
		case IL_JPG_QUALITY:
			return ilStates[ilCurrentPos].ilJpgQuality;
		case IL_PNG_INTERLACE:
			return ilStates[ilCurrentPos].ilPngInterlace;
		case IL_TGA_RLE:
			return ilStates[ilCurrentPos].ilTgaRle;
		case IL_BMP_RLE:
			return ilStates[ilCurrentPos].ilBmpRle;
		case IL_SGI_RLE:
			return ilStates[ilCurrentPos].ilSgiRle;
		case IL_JPG_SAVE_FORMAT:
			return ilStates[ilCurrentPos].ilJpgFormat;
		case IL_DXTC_FORMAT:
			return ilStates[ilCurrentPos].ilDxtcFormat;
		case IL_PCD_PICNUM:
			return ilStates[ilCurrentPos].ilPcdPicNum;
		case IL_QUANTIZATION_MODE:
			return ilStates[ilCurrentPos].ilQuantMode;
		case IL_NEU_QUANT_SAMPLE:
			return ilStates[ilCurrentPos].ilNeuSample;
		case IL_KEEP_DXTC_DATA:
			return ilStates[ilCurrentPos].ilKeepDxtcData;
		case IL_MAX_QUANT_INDEXS:
			return ilStates[ilCurrentPos].ilQuantMaxIndexs ;
	}

	ilSetError(IL_INTERNAL_ERROR);
	return 0;
}
