//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 06/13/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_convert.c
//
// Description: Converts between several image formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_manip.h"
#include <limits.h>


ILimage *iConvertPalette(ILimage *Image, ILenum DestFormat)
{
	static const ILfloat LumFactor[3] = { 0.212671f, 0.715160f, 0.072169f };  // http://www.inforamp.net/~poynton/ and libpng's libpng.txt - Used for conversion to luminance.
	ILimage		*NewImage = NULL, *CurImage = NULL;
	ILuint		i, j, k, c, Size, LumBpp = 1;
	ILfloat		Resultf;
	ILubyte		*Temp = NULL;
	ILboolean	Converted;

	NewImage = (ILimage*)icalloc(1, sizeof(ILimage));  // Much better to have it all set to 0.
	if (NewImage == NULL) {
		return IL_FALSE;
	}

	ilCopyImageAttr(NewImage, Image);

	if (!iCurImage->Pal.Palette || !iCurImage->Pal.PalSize || iCurImage->Pal.PalType == IL_PAL_NONE || iCurImage->Bpp != 1) {
		ilCloseImage(NewImage);
		ilSetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	if (DestFormat == IL_LUMINANCE || DestFormat == IL_LUMINANCE_ALPHA) {
		if (NewImage->Pal.Palette)
			ifree(NewImage->Pal.Palette);
		if (DestFormat == IL_LUMINANCE_ALPHA)
			LumBpp = 2;

		switch (iCurImage->Pal.PalType)
		{
			case IL_PAL_RGB24:
			case IL_PAL_RGB32:
			case IL_PAL_RGBA32:
				Temp = (ILubyte*)ialloc(LumBpp * Image->Pal.PalSize / ilGetBppPal(Image->Pal.PalType));
				if (Temp == NULL)
					goto alloc_error;

				Size = ilGetBppPal(Image->Pal.PalType);
				for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += LumBpp) {
					Resultf = 0.0f;
					for (c = 0; c < Size; c++) {
						Resultf += Image->Pal.Palette[i + c] * LumFactor[c];
					}
					Temp[k] = (ILubyte)Resultf;
					if (LumBpp == 2) {
						if (iCurImage->Pal.PalType == IL_PAL_RGBA32)
							Temp[k+1] = Image->Pal.Palette[i + 3];
						else
							Temp[k+1] = 0xff;
					}
				}

				break;

			case IL_PAL_BGR24:
			case IL_PAL_BGR32:
			case IL_PAL_BGRA32:
				Temp = (ILubyte*)ialloc(LumBpp * Image->Pal.PalSize / ilGetBppPal(Image->Pal.PalType));
				if (Temp == NULL)
					goto alloc_error;

				Size = ilGetBppPal(Image->Pal.PalType);
				for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += LumBpp) {
					Resultf = 0.0f;  j = 2;
					for (c = 0; c < Size; c++, j--) {
						Resultf += Image->Pal.Palette[i + c] * LumFactor[j];
					}
					Temp[k] = (ILubyte)Resultf;
					if (LumBpp == 2) {
						if (iCurImage->Pal.PalType == IL_PAL_RGBA32)
							Temp[k+1] = Image->Pal.Palette[i + 3];
						else
							Temp[k+1] = 0xff;
					}
				}

				break;
		}

		NewImage->Pal.Palette = NULL;
		NewImage->Pal.PalSize = 0;
		NewImage->Pal.PalType = IL_PAL_NONE;
		NewImage->Format = DestFormat;
		NewImage->Bpp = LumBpp;
		NewImage->Bps = NewImage->Width * LumBpp;
		NewImage->SizeOfData = NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->Data = (ILubyte*)ialloc(NewImage->SizeOfData);
		if (NewImage->Data == NULL)
			goto alloc_error;

		if (LumBpp == 2) {
			for (i = 0; i < Image->SizeOfData; i++) {
				NewImage->Data[i*2] = Temp[Image->Data[i] * 2];
				NewImage->Data[i*2+1] = Temp[Image->Data[i] * 2 + 1];
			}
		}
		else {
			for (i = 0; i < Image->SizeOfData; i++) {
				NewImage->Data[i] = Temp[Image->Data[i]];
			}
		}

		ifree(Temp);

		return NewImage;
	}

	//NewImage->Format = ilGetPalBaseType(iCurImage->Pal.PalType);
	NewImage->Format = DestFormat;

	if (ilGetBppFormat(NewImage->Format) == 0) {
		ilCloseImage(NewImage);
		ilSetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	CurImage = iCurImage;
	ilSetCurImage(NewImage);

	switch (DestFormat)
	{
		case IL_RGB:
			Converted = ilConvertPal(IL_PAL_RGB24);
			break;

		case IL_BGR:
			Converted = ilConvertPal(IL_PAL_BGR24);
			break;

		case IL_RGBA:
			Converted = ilConvertPal(IL_PAL_RGB32);
			break;

		case IL_BGRA:
			Converted = ilConvertPal(IL_PAL_BGR32);
			break;

		case IL_COLOUR_INDEX:
			// Just copy the original image over.
			NewImage->Data = (ILubyte*)ialloc(CurImage->SizeOfData);
			if (NewImage->Data == NULL)
				goto alloc_error;
			NewImage->Pal.Palette = (ILubyte*)ialloc(iCurImage->Pal.PalSize);
			if (NewImage->Pal.Palette == NULL)
				goto alloc_error;
			memcpy(NewImage->Data, CurImage->Data, CurImage->SizeOfData);
			memcpy(NewImage->Pal.Palette, Image->Pal.Palette, Image->Pal.PalSize);
			NewImage->Pal.PalSize = Image->Pal.PalSize;
			NewImage->Pal.PalType = Image->Pal.PalType;
			ilSetCurImage(CurImage);
			return NewImage;

		default:
			ilCloseImage(NewImage);
			ilSetError(IL_INVALID_CONVERSION);
			return NULL;
	}

	// Resize to new bpp
	ilResizeImage(NewImage, NewImage->Width, NewImage->Height, NewImage->Depth, ilGetBppFormat(DestFormat), /*ilGetBppType(DestType)*/1);

	// ilConvertPal already sets the error message - no need to confuse the user.
	if (!Converted) {
		ilSetCurImage(CurImage);
		ilCloseImage(NewImage);
		return NULL;
	}

	Size = ilGetBppPal(NewImage->Pal.PalType);
	for (i = 0; i < Image->SizeOfData; i++) {
		for (c = 0; c < NewImage->Bpp; c++) {
			NewImage->Data[i * NewImage->Bpp + c] = NewImage->Pal.Palette[Image->Data[i] * Size + c];
		}
	}

	ifree(NewImage->Pal.Palette);

	NewImage->Pal.Palette = NULL;
	NewImage->Pal.PalSize = 0;
	NewImage->Pal.PalType = IL_PAL_NONE;
	ilSetCurImage(CurImage);

	return NewImage;

alloc_error:
	ifree(Temp);
	if (NewImage)
		ilCloseImage(NewImage);
	if (CurImage != iCurImage)
		ilSetCurImage(CurImage);
	return NULL;
}


// In il_quantizer.c
ILimage *iQuantizeImage(ILimage *Image, ILuint NumCols);
// In il_neuquant.c
ILimage *iNeuQuant(ILimage *Image);

// Converts an image from one format to another
ILAPI ILimage* ILAPIENTRY iConvertImage(ILimage *Image, ILenum DestFormat, ILenum DestType)
{
	ILimage	*NewImage, *CurImage;
	ILuint	i;
	ILubyte	*NewData;

	CurImage = Image;
	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// We don't support 16-bit color indices (or higher).
	if (DestFormat == IL_COLOUR_INDEX && DestType >= IL_SHORT) {
		ilSetError(IL_INVALID_CONVERSION);
		return NULL;
	}

	if (Image->Format == IL_COLOUR_INDEX) {
		NewImage = iConvertPalette(Image, DestFormat);

		//added test 2003-09-01
		if(NewImage == NULL)
			return NULL;

		if (DestType == NewImage->Type)
			return NewImage;

		NewData = ilConvertBuffer(NewImage->SizeOfData, NewImage->Format, DestFormat, NewImage->Type, DestType, NewImage->Data);
		if (NewData == NULL) {
			ifree(NewImage);  // ilCloseImage not needed.
			return NULL;
		}
		ifree(NewImage->Data);
		NewImage->Data = NewData;

		ilCopyImageAttr(NewImage, Image);
		NewImage->Format = DestFormat;
		NewImage->Type = DestType;
		NewImage->Bpc = ilGetBppType(DestType);
		NewImage->Bpp = ilGetBppFormat(DestFormat);
		NewImage->Bps = NewImage->Bpp * NewImage->Bpc * NewImage->Width;
		NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->SizeOfData = NewImage->SizeOfPlane * NewImage->Depth;
	}
	else if (DestFormat == IL_COLOUR_INDEX && Image->Format != IL_LUMINANCE) {
		if (iGetInt(IL_QUANTIZATION_MODE) == IL_NEU_QUANT)
			return iNeuQuant(Image);
		else // Assume IL_WU_QUANT otherwise.
			return iQuantizeImage(Image, iGetInt(IL_MAX_QUANT_INDEXS));
	}
	else {
		NewImage = (ILimage*)icalloc(1, sizeof(ILimage));  // Much better to have it all set to 0.
		if (NewImage == NULL) {
			return NULL;
		}

		if (ilGetBppFormat(DestFormat) == 0) {
			ilSetError(IL_INVALID_PARAM);
			ifree(NewImage);
			return NULL;
		}

		ilCopyImageAttr(NewImage, Image);
		NewImage->Format = DestFormat;
		NewImage->Type = DestType;
		NewImage->Bpc = ilGetBppType(DestType);
		NewImage->Bpp = ilGetBppFormat(DestFormat);
		NewImage->Bps = NewImage->Bpp * NewImage->Bpc * NewImage->Width;
		NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->SizeOfData = NewImage->SizeOfPlane * NewImage->Depth;

		if (DestFormat == IL_COLOUR_INDEX && Image->Format == IL_LUMINANCE) {
			NewImage->Pal.PalSize = 768;
			NewImage->Pal.PalType = IL_PAL_RGB24;
			NewImage->Pal.Palette = (ILubyte*)ialloc(768);
			for (i = 0; i < 256; i++) {
				NewImage->Pal.Palette[i * 3] = i;
				NewImage->Pal.Palette[i * 3 + 1] = i;
				NewImage->Pal.Palette[i * 3 + 2] = i;
			}
			NewImage->Data = (ILubyte*)ialloc(Image->SizeOfData);
			if (NewImage->Data == NULL) {
				ilCloseImage(NewImage);
				return NULL;
			}
			memcpy(NewImage->Data, Image->Data, Image->SizeOfData);
		}
		else {
			NewImage->Data = ilConvertBuffer(Image->SizeOfData, Image->Format, DestFormat, Image->Type, DestType, Image->Data);
			if (NewImage->Data == NULL) {
				ifree(NewImage);  // ilCloseImage not needed.
				return NULL;
			}
		}
	}

	return NewImage;
}


//! Converts the current image to the DestFormat format.
ILboolean ILAPIENTRY ilConvertImage(ILenum DestFormat, ILenum DestType)
{
	ILimage *Image;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (DestFormat == iCurImage->Format && DestType == iCurImage->Type)
		return IL_TRUE;  // No conversion needed.

	if (DestType == iCurImage->Type) {
		if (iFastConvert(DestFormat)) {
			iCurImage->Format = DestFormat;
			return IL_TRUE;
		}
	}

	if (ilIsEnabled(IL_USE_KEY_COLOUR)) {
		ilAddAlphaKey(iCurImage);
	}
	Image = iConvertImage(iCurImage, DestFormat, DestType);
	if (Image == NULL)
		return IL_FALSE;

	//ilCopyImageAttr(iCurImage, Image);  // Destroys subimages.

	// We don't copy the colour profile here, since it stays the same.
	//	Same with the DXTC data.
	iCurImage->Format = DestFormat;
	iCurImage->Type = DestType;
	iCurImage->Bpc = ilGetBppType(DestType);
	iCurImage->Bpp = ilGetBppFormat(DestFormat);
	iCurImage->Bps = iCurImage->Width * iCurImage->Bpc * iCurImage->Bpp;
	iCurImage->SizeOfPlane = iCurImage->Bps * iCurImage->Height;
	iCurImage->SizeOfData = iCurImage->Depth * iCurImage->SizeOfPlane;
	if (iCurImage->Pal.Palette && iCurImage->Pal.PalSize && iCurImage->Pal.PalType != IL_PAL_NONE)
		ifree(iCurImage->Pal.Palette);
	iCurImage->Pal.Palette = Image->Pal.Palette;
	iCurImage->Pal.PalSize = Image->Pal.PalSize;
	iCurImage->Pal.PalType = Image->Pal.PalType;
	Image->Pal.Palette = NULL;
	ifree(iCurImage->Data);
	iCurImage->Data = Image->Data;
	Image->Data = NULL;
	ilCloseImage(Image);

	return IL_TRUE;
}


// Swaps the colour order of the current image (rgb(a)->bgr(a) or vice-versa).
//	Must be either an 8, 24 or 32-bit (coloured) image (or palette).
ILboolean ilSwapColours()
{
	ILuint		i = 0, Size = iCurImage->Bps * iCurImage->Height;
	ILbyte		PalBpp = ilGetBppPal(iCurImage->Pal.PalType);
	ILushort	*ShortPtr;
	ILuint		*IntPtr, Temp;
	ILdouble	*DoublePtr, DoubleTemp;

	if ((iCurImage->Bpp != 1 && iCurImage->Bpp != 3 && iCurImage->Bpp != 4)) {
		ilSetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	// Just check before we change the format.
	if (iCurImage->Format == IL_COLOUR_INDEX) {
		if (PalBpp == 0 || iCurImage->Format != IL_COLOUR_INDEX) {
			ilSetError(IL_ILLEGAL_OPERATION);
			return IL_FALSE;
		}
	}

	switch (iCurImage->Format)
	{
		case IL_RGB:
			iCurImage->Format = IL_BGR;
			break;
		case IL_RGBA:
			iCurImage->Format = IL_BGRA;
			break;
		case IL_BGR:
			iCurImage->Format = IL_RGB;
			break;
		case IL_BGRA:
			iCurImage->Format = IL_RGBA;
			break;
		case IL_LUMINANCE:
			return IL_TRUE;  // No need to do anything to luminance images.
		case IL_COLOUR_INDEX:
			switch (iCurImage->Pal.PalType)
			{
				case IL_PAL_RGB24:
					iCurImage->Pal.PalType = IL_PAL_BGR24;
					break;
				case IL_PAL_RGB32:
					iCurImage->Pal.PalType = IL_PAL_BGR32;
					break;
				case IL_PAL_RGBA32:
					iCurImage->Pal.PalType = IL_PAL_BGRA32;
					break;
				case IL_PAL_BGR24:
					iCurImage->Pal.PalType = IL_PAL_RGB24;
					break;
				case IL_PAL_BGR32:
					iCurImage->Pal.PalType = IL_PAL_RGB32;
					break;
				case IL_PAL_BGRA32:
					iCurImage->Pal.PalType = IL_PAL_RGBA32;
					break;
				default:
					ilSetError(IL_ILLEGAL_OPERATION);
					return IL_FALSE;
			}
			break;
		default:
			ilSetError(IL_ILLEGAL_OPERATION);
			return IL_FALSE;
	}

	if (iCurImage->Bpp == IL_COLOUR_INDEX) {
		for (; i < iCurImage->Pal.PalSize; i += PalBpp) {
				Temp = iCurImage->Pal.Palette[i];
				iCurImage->Pal.Palette[i] = iCurImage->Pal.Palette[i+2];
				iCurImage->Pal.Palette[i+2] = Temp;
		}
	}
	else {
		ShortPtr = (ILushort*)iCurImage->Data;
		IntPtr = (ILuint*)iCurImage->Data;
		DoublePtr = (ILdouble*)iCurImage->Data;
		switch (iCurImage->Bpc)
		{
			case 1:
				for (; i < Size; i += iCurImage->Bpp) {
					Temp = iCurImage->Data[i];
					iCurImage->Data[i] = iCurImage->Data[i+2];
					iCurImage->Data[i+2] = Temp;
				}
				break;
			case 2:
				for (; i < Size; i += iCurImage->Bpp) {
					Temp = ShortPtr[i];
					ShortPtr[i] = ShortPtr[i+2];
					ShortPtr[i+2] = Temp;
				}
				break;
			case 4:  // Works fine with ILint, ILuint and ILfloat.
				for (; i < Size; i += iCurImage->Bpp) {
					Temp = IntPtr[i];
					IntPtr[i] = IntPtr[i+2];
					IntPtr[i+2] = Temp;
				}
				break;
			case 8:
				for (; i < Size; i += iCurImage->Bpp) {
					DoubleTemp = DoublePtr[i];
					DoublePtr[i] = DoublePtr[i+2];
					DoublePtr[i+2] = DoubleTemp;
				}
				break;
		}
	}

	return IL_TRUE;
}


// Adds an opaque alpha channel to a 24-bit image
ILboolean ilAddAlpha()
{
	ILubyte		*NewData, NewBpp;
	ILuint		i = 0, j = 0, Size;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iCurImage->Bpp != 3) {
		ilSetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	Size = iCurImage->Bps * iCurImage->Height / iCurImage->Bpc;
	NewBpp = (ILubyte)(iCurImage->Bpp + 1);
	
	NewData = (ILubyte*)ialloc(NewBpp * iCurImage->Bpc * iCurImage->Width * iCurImage->Height);
	if (NewData == NULL) {
		return IL_FALSE;
	}

	switch (iCurImage->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			for (; i < Size; i += iCurImage->Bpp, j += NewBpp) {
				NewData[j]   = iCurImage->Data[i];
				NewData[j+1] = iCurImage->Data[i+1];
				NewData[j+2] = iCurImage->Data[i+2];
				NewData[j+3] = UCHAR_MAX;  // Max opaqueness
			}
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			for (; i < Size; i += iCurImage->Bpp, j += NewBpp) {
				((ILushort*)NewData)[j]   = ((ILushort*)iCurImage->Data)[i];
				((ILushort*)NewData)[j+1] = ((ILushort*)iCurImage->Data)[i+1];
				((ILushort*)NewData)[j+2] = ((ILushort*)iCurImage->Data)[i+2];
				((ILushort*)NewData)[j+3] = USHRT_MAX;
			}
			break;

		case IL_INT:
		case IL_UNSIGNED_INT:
			for (; i < Size; i += iCurImage->Bpp, j += NewBpp) {
				((ILuint*)NewData)[j]   = ((ILuint*)iCurImage->Data)[i];
				((ILuint*)NewData)[j+1] = ((ILuint*)iCurImage->Data)[i+1];
				((ILuint*)NewData)[j+2] = ((ILuint*)iCurImage->Data)[i+2];
				((ILuint*)NewData)[j+3] = UINT_MAX;
			}
			break;

		case IL_FLOAT:
			for (; i < Size; i += iCurImage->Bpp, j += NewBpp) {
				((ILfloat*)NewData)[j]   = ((ILfloat*)iCurImage->Data)[i];
				((ILfloat*)NewData)[j+1] = ((ILfloat*)iCurImage->Data)[i+1];
				((ILfloat*)NewData)[j+2] = ((ILfloat*)iCurImage->Data)[i+2];
				((ILfloat*)NewData)[j+3] = 1.0f;
			}
			break;

		case IL_DOUBLE:
			for (; i < Size; i += iCurImage->Bpp, j += NewBpp) {
				((ILdouble*)NewData)[j]   = ((ILdouble*)iCurImage->Data)[i];
				((ILdouble*)NewData)[j+1] = ((ILdouble*)iCurImage->Data)[i+1];
				((ILdouble*)NewData)[j+2] = ((ILdouble*)iCurImage->Data)[i+2];
				((ILdouble*)NewData)[j+3] = 1.0;
			}
			break;

		default:
			ifree(NewData);
			ilSetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
	}


	iCurImage->Bpp = NewBpp;
	iCurImage->Bps = iCurImage->Width * iCurImage->Bpc * NewBpp;
	iCurImage->SizeOfPlane = iCurImage->Bps * iCurImage->Height;
	iCurImage->SizeOfData = iCurImage->SizeOfPlane * iCurImage->Depth;
	ifree(iCurImage->Data);
	iCurImage->Data = NewData;

	switch (iCurImage->Format)
	{
		case IL_RGB:
			iCurImage->Format = IL_RGBA;
			break;
		case IL_BGR:
			iCurImage->Format = IL_BGRA;
			break;
	}

	return IL_TRUE;
}


ILfloat KeyRed = 0, KeyGreen = 0, KeyBlue = 0, KeyAlpha = 0;

ILvoid ILAPIENTRY ilKeyColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha)
{
	KeyRed		= Red;
	KeyGreen	= Green;
	KeyBlue		= Blue;
	KeyAlpha	= Alpha;
	return;
}


// Adds an alpha channel to an 8 or 24-bit image,
//	making the image transparent where Key is equal to the pixel.
ILboolean ilAddAlphaKey(ILimage *Image)
{
	ILubyte		*NewData, NewBpp;
	ILfloat		KeyColour[3];
	ILuint		i = 0, j = 0, c, Size;
	ILboolean	Same;

	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Image->Format != IL_COLOUR_INDEX) {
		if (Image->Bpp != 3) {
			ilSetError(IL_INVALID_VALUE);
			return IL_FALSE;
		}

		if (Image->Format == IL_BGR || Image->Format == IL_BGRA) {
			KeyColour[0] = KeyBlue;
			KeyColour[1] = KeyGreen;
			KeyColour[2] = KeyRed;
		}
		else {
			KeyColour[0] = KeyRed;
			KeyColour[1] = KeyGreen;
			KeyColour[2] = KeyBlue;
		}

		Size = Image->Bps * Image->Height / Image->Bpc;

		NewBpp = (ILubyte)(Image->Bpp + 1);
		
		NewData = (ILubyte*)ialloc(NewBpp * Image->Bpc * Image->Width * Image->Height);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		switch (Image->Type)
		{
			case IL_BYTE:
			case IL_UNSIGNED_BYTE:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					NewData[j]   = Image->Data[i];
					NewData[j+1] = Image->Data[i+1];
					NewData[j+2] = Image->Data[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (Image->Data[i+c] != KeyColour[c] * UCHAR_MAX)
							Same = IL_FALSE;
					}

					if (Same)
						NewData[j+3] = 0;  // Transparent - matches key colour
					else
						NewData[j+3] = UCHAR_MAX;
				}
				break;

			case IL_SHORT:
			case IL_UNSIGNED_SHORT:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILushort*)NewData)[j]   = ((ILushort*)Image->Data)[i];
					((ILushort*)NewData)[j+1] = ((ILushort*)Image->Data)[i+1];
					((ILushort*)NewData)[j+2] = ((ILushort*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILushort*)Image->Data)[i+c] != KeyColour[c] * USHRT_MAX)
							Same = IL_FALSE;
					}

					if (Same)
						((ILushort*)NewData)[j+3] = 0;
					else
						((ILushort*)NewData)[j+3] = USHRT_MAX;
				}
				break;

			case IL_INT:
			case IL_UNSIGNED_INT:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILuint*)NewData)[j]   = ((ILuint*)Image->Data)[i];
					((ILuint*)NewData)[j+1] = ((ILuint*)Image->Data)[i+1];
					((ILuint*)NewData)[j+2] = ((ILuint*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILuint*)Image->Data)[i+c] != KeyColour[c] * UINT_MAX)
							Same = IL_FALSE;
					}

					if (Same)
						((ILuint*)NewData)[j+3] = 0;
					else
						((ILuint*)NewData)[j+3] = UINT_MAX;
				}
				break;

			case IL_FLOAT:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILfloat*)NewData)[j]   = ((ILfloat*)Image->Data)[i];
					((ILfloat*)NewData)[j+1] = ((ILfloat*)Image->Data)[i+1];
					((ILfloat*)NewData)[j+2] = ((ILfloat*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILfloat*)Image->Data)[i+c] != KeyColour[c])
							Same = IL_FALSE;
					}

					if (Same)
						((ILfloat*)NewData)[j+3] = 0.0f;
					else
						((ILfloat*)NewData)[j+3] = 1.0f;
				}
				break;

			case IL_DOUBLE:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILdouble*)NewData)[j]   = ((ILdouble*)Image->Data)[i];
					((ILdouble*)NewData)[j+1] = ((ILdouble*)Image->Data)[i+1];
					((ILdouble*)NewData)[j+2] = ((ILdouble*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILdouble*)Image->Data)[i+c] != KeyColour[c])
							Same = IL_FALSE;
					}

					if (Same)
						((ILdouble*)NewData)[j+3] = 0.0;
					else
						((ILdouble*)NewData)[j+3] = 1.0;
				}
				break;

			default:
				ifree(NewData);
				ilSetError(IL_INTERNAL_ERROR);
				return IL_FALSE;
		}


		Image->Bpp = NewBpp;
		Image->Bps = Image->Width * Image->Bpc * NewBpp;
		Image->SizeOfPlane = Image->Bps * Image->Height;
		Image->SizeOfData = Image->SizeOfPlane * Image->Depth;
		ifree(Image->Data);
		Image->Data = NewData;

		switch (Image->Format)
		{
			case IL_RGB:
				Image->Format = IL_RGBA;
				break;
			case IL_BGR:
				Image->Format = IL_BGRA;
				break;
		}
	}
	else {  // IL_COLOUR_INDEX
		if (Image->Bpp != 1) {
			ilSetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
		}

		Size = ilGetInteger(IL_PALETTE_NUM_COLS);
		if (Size == 0) {
			ilSetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
		}

		if ((ILuint)(KeyAlpha * UCHAR_MAX) > Size) {
			ilSetError(IL_INVALID_VALUE);
			return IL_FALSE;
		}

		switch (Image->Pal.PalType)
		{
			case IL_PAL_RGB24:
			case IL_PAL_RGB32:
			case IL_PAL_RGBA32:
				if (!ilConvertPal(IL_PAL_RGBA32))
					return IL_FALSE;
				break;
			case IL_PAL_BGR24:
			case IL_PAL_BGR32:
			case IL_PAL_BGRA32:
				if (!ilConvertPal(IL_PAL_BGRA32))
					return IL_FALSE;
				break;
			default:
				ilSetError(IL_INTERNAL_ERROR);
				return IL_FALSE;
		}

		// Set the colour index to be transparent.
		Image->Pal.Palette[(ILuint)(KeyAlpha * UCHAR_MAX) * 4 + 3] = 0;

		// @TODO: Check if this is the required behaviour.

		if (Image->Pal.PalType == IL_PAL_RGBA32)
			ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
		else
			ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);
	}

	return IL_TRUE;
}


// Removes alpha from a 32-bit image
//	Should we maybe add an option that changes the image based on the alpha?
ILboolean ilRemoveAlpha()
{
	ILubyte *NewData, NewBpp;
	ILuint i = 0, j = 0, Size;

	if (iCurImage == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iCurImage->Bpp != 4) {
		ilSetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	Size = iCurImage->Bps * iCurImage->Height;
	NewBpp = (ILubyte)(iCurImage->Bpp - 1);
	
	NewData = (ILubyte*)ialloc(NewBpp * iCurImage->Bpc * iCurImage->Width * iCurImage->Height);
	if (NewData == NULL) {
		return IL_FALSE;
	}

	switch (iCurImage->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			for (; i < Size; i += iCurImage->Bpp, j += NewBpp) {
				NewData[j]   = iCurImage->Data[i];
				NewData[j+1] = iCurImage->Data[i+1];
				NewData[j+2] = iCurImage->Data[i+2];
			}
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			for (; i < Size; i += iCurImage->Bpp, j += NewBpp) {
				((ILushort*)NewData)[j]   = ((ILushort*)iCurImage->Data)[i];
				((ILushort*)NewData)[j+1] = ((ILushort*)iCurImage->Data)[i+1];
				((ILushort*)NewData)[j+2] = ((ILushort*)iCurImage->Data)[i+2];
			}
			break;

		case IL_INT:
		case IL_UNSIGNED_INT:
			for (; i < Size; i += iCurImage->Bpp, j += NewBpp) {
				((ILuint*)NewData)[j]   = ((ILuint*)iCurImage->Data)[i];
				((ILuint*)NewData)[j+1] = ((ILuint*)iCurImage->Data)[i+1];
				((ILuint*)NewData)[j+2] = ((ILuint*)iCurImage->Data)[i+2];
			}
			break;

		case IL_FLOAT:
			for (; i < Size; i += iCurImage->Bpp, j += NewBpp) {
				((ILfloat*)NewData)[j]   = ((ILfloat*)iCurImage->Data)[i];
				((ILfloat*)NewData)[j+1] = ((ILfloat*)iCurImage->Data)[i+1];
				((ILfloat*)NewData)[j+2] = ((ILfloat*)iCurImage->Data)[i+2];
			}
			break;

		case IL_DOUBLE:
			for (; i < Size; i += iCurImage->Bpp, j += NewBpp) {
				((ILdouble*)NewData)[j]   = ((ILdouble*)iCurImage->Data)[i];
				((ILdouble*)NewData)[j+1] = ((ILdouble*)iCurImage->Data)[i+1];
				((ILdouble*)NewData)[j+2] = ((ILdouble*)iCurImage->Data)[i+2];
			}
			break;

		default:
			ifree(NewData);
			ilSetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
	}


	iCurImage->Bpp = NewBpp;
	iCurImage->Bps = iCurImage->Width * iCurImage->Bpc * NewBpp;
	iCurImage->SizeOfPlane = iCurImage->Bps * iCurImage->Height;
	iCurImage->SizeOfData = iCurImage->SizeOfPlane * iCurImage->Depth;
	ifree(iCurImage->Data);
	iCurImage->Data = NewData;

	switch (iCurImage->Format)
	{
		case IL_RGBA:
			iCurImage->Format = IL_RGB;
			break;
		case IL_BGRA:
			iCurImage->Format = IL_BGR;
			break;
	}

	return IL_TRUE;
}


ILboolean ilFixCur()
{
	if (ilIsEnabled(IL_ORIGIN_SET)) {
		if ((ILenum)ilGetInteger(IL_ORIGIN_MODE) != iCurImage->Origin) {
			if (!ilFlipImage()) {
				return IL_FALSE;
			}
		}
	}

	if (ilIsEnabled(IL_TYPE_SET)) {
		if ((ILenum)ilGetInteger(IL_TYPE_MODE) != iCurImage->Type) {
			if (!ilConvertImage(iCurImage->Format, ilGetInteger(IL_TYPE_MODE))) {
				return IL_FALSE;
			}
		}
	}
	if (ilIsEnabled(IL_FORMAT_SET)) {
		if ((ILenum)ilGetInteger(IL_FORMAT_MODE) != iCurImage->Format) {
			if (!ilConvertImage(ilGetInteger(IL_FORMAT_MODE), iCurImage->Type)) {
				return IL_FALSE;
			}
		}
	}

	if (iCurImage->Format == IL_COLOUR_INDEX) {
		if (ilGetBoolean(IL_CONV_PAL) == IL_TRUE) {
			if (!ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE)) {
				return IL_FALSE;
			}
		}
	}
/*	Swap Colors on Big Endian !!!!!
#ifdef __BIG_ENDIAN__
	// Swap endian
	EndianSwapData(iCurImage);
#endif 
*/
	return IL_TRUE;
}


ILboolean ilFixImage()
{
	ILuint NumImages, i;

	NumImages = ilGetInteger(IL_NUM_IMAGES);
	for (i = 0; i < NumImages; i++) {
		ilBindImage(ilGetCurName());  // Set to parent image first.
		if (!ilActiveImage(i+1))
			return IL_FALSE;
		if (!ilFixCur())
			return IL_FALSE;
	}

	NumImages = ilGetInteger(IL_NUM_MIPMAPS);
	for (i = 0; i < NumImages; i++) {
		ilBindImage(ilGetCurName());  // Set to parent image first.
		if (!ilActiveMipmap(i+1))
			return IL_FALSE;
		if (!ilFixCur())
			return IL_FALSE;
	}

	NumImages = ilGetInteger(IL_NUM_LAYERS);
	for (i = 0; i < NumImages; i++) {
		ilBindImage(ilGetCurName());  // Set to parent image first.
		if (!ilActiveLayer(i+1))
			return IL_FALSE;
		if (!ilFixCur())
			return IL_FALSE;
	}

	ilBindImage(ilGetCurName());
	ilFixCur();

	return IL_TRUE;
}
