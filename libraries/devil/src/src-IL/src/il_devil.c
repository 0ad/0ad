//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 02/19/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_devil.c
//
// Description: Functions for working with the ILimage's and the current image
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include <string.h>
#include <limits.h>
#include "il_manip.h"


// Creates a new ILimage based on the specifications given
ILAPI ILimage* ILAPIENTRY ilNewImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc)
{
	ILimage *Image = (ILimage*)ialloc(sizeof(ILimage));
	if (Image == NULL) {
		return NULL;
	}
	Image->Width = Width == 0 ? 1 : Width;
	Image->Height = Height == 0 ? 1 : Height;
	Image->Depth = Depth == 0 ? 1 : Depth;
	Image->Bpp = Bpp;
	Image->Bpc = Bpc;
	Image->Bps = Bpp * Image->Bpc * Width;
	Image->SizeOfPlane = Image->Bps * Height;
	Image->SizeOfData = Image->SizeOfPlane * Depth;
	Image->Layers = NULL;
	Image->Mipmaps = NULL;
	Image->Next = NULL;
	Image->NumLayers = 0;
	Image->NumMips = 0;
	Image->NumNext = 0;
	Image->Duration = 0;
	Image->CubeFlags = 0;
	Image->Type = IL_UNSIGNED_BYTE;
	Image->Origin = IL_ORIGIN_LOWER_LEFT;
	Image->AnimList = NULL;
	Image->AnimSize = 0;
	Image->Profile = NULL;
	Image->ProfileSize = 0;
	Image->Pal.Palette = NULL;
	Image->Pal.PalSize = 0;
	Image->Pal.PalType = IL_PAL_NONE;
	Image->OffX = 0;
	Image->OffY = 0;
	Image->DxtcData = NULL;
	Image->DxtcFormat = IL_DXT_NO_COMP;
	Image->DxtcSize = 0;

	switch (Bpp)
	{
		case 1:
			Image->Format = IL_LUMINANCE;
			break;
		case 2:
			Image->Format = IL_LUMINANCE_ALPHA;
			break;
		case 3:
			Image->Format = IL_RGB;
			break;
		case 4:
			Image->Format = IL_RGBA;
			break;
		default:
			ilSetError(IL_INVALID_PARAM);
			ifree(Image);
			return NULL;
	}

	Image->Data = (ILubyte*)ialloc(Image->SizeOfData);
	if (Image->Data == NULL) {
		ifree(Image);
		return NULL;
	}

	return Image;
}


//! Changes the current bound image to use these new dimensions (current data is destroyed).
ILboolean ILAPIENTRY ilTexImage(ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, ILvoid *Data)
{
	return ilTexImage_(iCurImage, Width, Height, Depth, Bpp, Format, Type, Data);
}


// Internal version of ilTexImage.
ILAPI ILboolean ILAPIENTRY ilTexImage_(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILenum Format, ILenum Type, ILvoid *Data)
{
	ILubyte BppType;

	if (Image == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Also check against format?
	/*if (Width == 0 || Height == 0 || Depth == 0 || Bpp == 0) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}*/

	BppType = ilGetBppType(Type);
	if (BppType == 0) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}
	Image->Width = Width == 0 ? 1 : Width;
	Image->Height = Height == 0 ? 1 : Height;
	Image->Depth = Depth == 0 ? 1 : Depth;
	Image->Bpp = Bpp;
	Image->Bpc = BppType;
	Image->Bps = Width * Bpp * Image->Bpc;
	Image->SizeOfPlane = Image->Bps * Height;
	Image->SizeOfData = Image->SizeOfPlane * Depth;
	Image->Format = Format;
	Image->Type = Type;
	Image->Origin = IL_ORIGIN_LOWER_LEFT;

	// Not sure if we should be getting rid of the palette...
	if (Image->Pal.Palette && Image->Pal.PalSize && Image->Pal.PalType != IL_PAL_NONE) {
		ifree(Image->Pal.Palette);
		Image->Pal.Palette = NULL;
		Image->Pal.PalSize = 0;
		Image->Pal.PalType = IL_PAL_NONE;
	}

	// Added 05-23-2002.
	ilCloseImage(Image->Mipmaps);
	ilCloseImage(Image->Next);
	ilCloseImage(Image->Layers);
	if (Image->AnimList)
		ifree(Image->AnimList);
	if (Image->Profile)
		ifree(Image->Profile);
	if (Image->DxtcData)
		ifree(Image->DxtcData);

	Image->Mipmaps = NULL;
	Image->NumMips = 0;
	Image->Next = NULL;
	Image->NumNext = 0;
	Image->Layers = NULL;
	Image->NumLayers = 0;
	Image->AnimList = NULL;
	Image->AnimSize = 0;
	Image->Profile = NULL;
	Image->ProfileSize = 0;
	Image->DxtcData = NULL;
	Image->DxtcFormat = IL_DXT_NO_COMP;
	Image->DxtcSize = 0;

	if (Image->Data)
		ifree(Image->Data);
	Image->Data = (ILubyte*)ialloc(Image->SizeOfData);
	if (Image->Data == NULL) {
		return IL_FALSE;
	}

	if (Data != NULL) {
		memcpy(Image->Data, Data, Image->SizeOfData);
	}

	return IL_TRUE;
}


//! Uploads Data of the same size to replace the current image's data.
ILboolean ILAPIENTRY ilSetData(ILvoid *Data)
{
	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	return ilTexSubImage_(iCurImage, Data);
}


// Internal version of ilTexSubImage.
ILAPI ILboolean ILAPIENTRY ilTexSubImage_(ILimage *Image, ILvoid *Data)
{
	if (Image == NULL || Data == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}
	if (!Image->Data) {
		Image->Data = (ILubyte*)ialloc(Image->SizeOfData);
		if (Image->Data == NULL)
			return IL_FALSE;
	}
	memcpy(Image->Data, Data, Image->SizeOfData);
	return IL_TRUE;
}


ILubyte* ILAPIENTRY ilGetData()
{
	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	return iCurImage->Data;
}


ILubyte* ILAPIENTRY ilGetPalette()
{
	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return NULL;
	}

	return iCurImage->Pal.Palette;
}


//ILfloat ClearRed = 0.0f, ClearGreen = 0.0f, ClearBlue = 0.0f, ClearAlpha = 0.0f;

// Changed to the colour of the Universe
//	(http://www.newscientist.com/news/news.jsp?id=ns99991775)
//	*(http://www.space.com/scienceastronomy/universe_color_020308.html)*
//ILfloat ClearRed = 0.269f, ClearGreen = 0.388f, ClearBlue = 0.342f, ClearAlpha = 0.0f;
ILfloat ClearRed = 1.0f, ClearGreen = 0.972549f, ClearBlue = 0.90588f, ClearAlpha = 0.0f,
		ClearLum = 1.0f;

ILvoid ILAPIENTRY ilClearColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha)
{
	// Clamp to 0.0f - 1.0f.
	ClearRed	= Red < 0.0f ? 0.0f : (Red > 1.0f ? 1.0f : Red);
	ClearGreen	= Green < 0.0f ? 0.0f : (Green > 1.0f ? 1.0f : Green);
	ClearBlue	= Blue < 0.0f ? 0.0f : (Blue > 1.0f ? 1.0f : Blue);
	ClearAlpha	= Alpha < 0.0f ? 0.0f : (Alpha > 1.0f ? 1.0f : Alpha);
	
	if ((Red == Green) && (Red == Blue) && (Green == Blue)) {
		ClearLum = Red < 0.0f ? 0.0f : (Red > 1.0f ? 1.0f : Red);
	}
	else {
		ClearLum = 0.212671f * ClearRed + 0.715160f * ClearGreen + 0.072169f * ClearBlue;
		ClearLum = ClearLum < 0.0f ? 0.0f : (ClearLum > 1.0f ? 1.0f : ClearLum);
	}

	return;
}


ILAPI ILvoid ILAPIENTRY ilGetClear(ILvoid *Colours, ILenum Format, ILenum Type)
{
	ILubyte		*BytePtr;
	ILushort	*ShortPtr;
	ILuint		*IntPtr;
	ILfloat		*FloatPtr;
	ILdouble	*DblPtr;

	switch (Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			BytePtr = (ILubyte*)Colours;
			switch (Format)
			{
				case IL_RGB:
					BytePtr[0] = (ILubyte)(ClearRed * UCHAR_MAX);
					BytePtr[1] = (ILubyte)(ClearGreen * UCHAR_MAX);
					BytePtr[2] = (ILubyte)(ClearBlue * UCHAR_MAX);
					break;

				case IL_RGBA:
					BytePtr[0] = (ILubyte)(ClearRed * UCHAR_MAX);
					BytePtr[1] = (ILubyte)(ClearGreen * UCHAR_MAX);
					BytePtr[2] = (ILubyte)(ClearBlue * UCHAR_MAX);
					BytePtr[3] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					break;

				case IL_BGR:
					BytePtr[2] = (ILubyte)(ClearRed * UCHAR_MAX);
					BytePtr[1] = (ILubyte)(ClearGreen * UCHAR_MAX);
					BytePtr[0] = (ILubyte)(ClearBlue * UCHAR_MAX);
					BytePtr[3] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					break;

				case IL_BGRA:
					BytePtr[2] = (ILubyte)(ClearRed * UCHAR_MAX);
					BytePtr[1] = (ILubyte)(ClearGreen * UCHAR_MAX);
					BytePtr[0] = (ILubyte)(ClearBlue * UCHAR_MAX);
					BytePtr[3] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					break;

				case IL_LUMINANCE:
					BytePtr[0] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					break;

				case IL_LUMINANCE_ALPHA:
					BytePtr[0] = (ILubyte)(ClearLum * UCHAR_MAX);
					BytePtr[1] = (ILubyte)(ClearAlpha * UCHAR_MAX);

				case IL_COLOUR_INDEX:
					BytePtr[0] = (ILubyte)(ClearAlpha * UCHAR_MAX);
					break;

				default:
					ilSetError(IL_INTERNAL_ERROR);
					return;
			}
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			ShortPtr = (ILushort*)Colours;
			switch (Format)
			{
				case IL_RGB:
					ShortPtr[0] = (ILushort)(ClearRed * USHRT_MAX);
					ShortPtr[1] = (ILushort)(ClearGreen * USHRT_MAX);
					ShortPtr[2] = (ILushort)(ClearBlue * USHRT_MAX);
					break;

				case IL_RGBA:
					ShortPtr[0] = (ILushort)(ClearRed * USHRT_MAX);
					ShortPtr[1] = (ILushort)(ClearGreen * USHRT_MAX);
					ShortPtr[2] = (ILushort)(ClearBlue * USHRT_MAX);
					ShortPtr[3] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;

				case IL_BGR:
					ShortPtr[2] = (ILushort)(ClearRed * USHRT_MAX);
					ShortPtr[1] = (ILushort)(ClearGreen * USHRT_MAX);
					ShortPtr[0] = (ILushort)(ClearBlue * USHRT_MAX);
					ShortPtr[3] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;

				case IL_BGRA:
					ShortPtr[2] = (ILushort)(ClearRed * USHRT_MAX);
					ShortPtr[1] = (ILushort)(ClearGreen * USHRT_MAX);
					ShortPtr[0] = (ILushort)(ClearBlue * USHRT_MAX);
					ShortPtr[3] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;

				case IL_LUMINANCE:
					ShortPtr[0] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;

				case IL_LUMINANCE_ALPHA:
					ShortPtr[0] = (ILushort)(ClearLum * USHRT_MAX);
					ShortPtr[1] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;

				case IL_COLOUR_INDEX:
					ShortPtr[0] = (ILushort)(ClearAlpha * USHRT_MAX);
					break;

				default:
					ilSetError(IL_INTERNAL_ERROR);
					return;
			}
			break;

		case IL_INT:
		case IL_UNSIGNED_INT:
			IntPtr = (ILuint*)Colours;
			switch (Format)
			{
				case IL_RGB:
					IntPtr[0] = (ILuint)(ClearRed * UINT_MAX);
					IntPtr[1] = (ILuint)(ClearGreen * UINT_MAX);
					IntPtr[2] = (ILuint)(ClearBlue * UINT_MAX);
					break;

				case IL_RGBA:
					IntPtr[0] = (ILuint)(ClearRed * UINT_MAX);
					IntPtr[1] = (ILuint)(ClearGreen * UINT_MAX);
					IntPtr[2] = (ILuint)(ClearBlue * UINT_MAX);
					IntPtr[3] = (ILuint)(ClearAlpha * UINT_MAX);
					break;

				case IL_BGR:
					IntPtr[2] = (ILuint)(ClearRed * UINT_MAX);
					IntPtr[1] = (ILuint)(ClearGreen * UINT_MAX);
					IntPtr[0] = (ILuint)(ClearBlue * UINT_MAX);
					IntPtr[3] = (ILuint)(ClearAlpha * UINT_MAX);
					break;

				case IL_BGRA:
					IntPtr[2] = (ILuint)(ClearRed * UINT_MAX);
					IntPtr[1] = (ILuint)(ClearGreen * UINT_MAX);
					IntPtr[0] = (ILuint)(ClearBlue * UINT_MAX);
					IntPtr[3] = (ILuint)(ClearAlpha * UINT_MAX);
					break;

				case IL_LUMINANCE:
					IntPtr[0] = (ILuint)(ClearAlpha * UINT_MAX);
					break;

				case IL_LUMINANCE_ALPHA:
					IntPtr[0] = (ILuint)(ClearLum * UINT_MAX);
					IntPtr[1] = (ILuint)(ClearAlpha * UINT_MAX);
					break;

				case IL_COLOUR_INDEX:
					IntPtr[0] = (ILuint)(ClearAlpha * UINT_MAX);
					break;

				default:
					ilSetError(IL_INTERNAL_ERROR);
					return;
			}
			break;

		case IL_FLOAT:
			FloatPtr = (ILfloat*)Colours;
			switch (Format)
			{
				case IL_RGB:
					FloatPtr[0] = ClearRed;
					FloatPtr[1] = ClearGreen;
					FloatPtr[2] = ClearBlue;
					break;

				case IL_RGBA:
					FloatPtr[0] = ClearRed;
					FloatPtr[1] = ClearGreen;
					FloatPtr[2] = ClearBlue;
					FloatPtr[3] = ClearAlpha;
					break;

				case IL_BGR:
					FloatPtr[2] = ClearRed;
					FloatPtr[1] = ClearGreen;
					FloatPtr[0] = ClearBlue;
					FloatPtr[3] = ClearAlpha;
					break;

				case IL_BGRA:
					FloatPtr[2] = ClearRed;
					FloatPtr[1] = ClearGreen;
					FloatPtr[0] = ClearBlue;
					FloatPtr[3] = ClearAlpha;
					break;

				case IL_LUMINANCE:
					FloatPtr[0] = ClearAlpha;
					break;

				case IL_LUMINANCE_ALPHA:
					FloatPtr[0] = ClearLum;
					FloatPtr[0] = ClearAlpha;
					break;

				case IL_COLOUR_INDEX:
					FloatPtr[0] = ClearAlpha;
					break;

				default:
					ilSetError(IL_INTERNAL_ERROR);
					return;
			}
			break;

		case IL_DOUBLE:
			DblPtr = (ILdouble*)Colours;
			switch (Format)
			{
				case IL_RGB:
					DblPtr[0] = ClearRed;
					DblPtr[1] = ClearGreen;
					DblPtr[2] = ClearBlue;
					break;

				case IL_RGBA:
					DblPtr[0] = ClearRed;
					DblPtr[1] = ClearGreen;
					DblPtr[2] = ClearBlue;
					DblPtr[3] = ClearAlpha;
					break;

				case IL_BGR:
					DblPtr[2] = ClearRed;
					DblPtr[1] = ClearGreen;
					DblPtr[0] = ClearBlue;
					DblPtr[3] = ClearAlpha;
					break;

				case IL_BGRA:
					DblPtr[2] = ClearRed;
					DblPtr[1] = ClearGreen;
					DblPtr[0] = ClearBlue;
					DblPtr[3] = ClearAlpha;
					break;

				case IL_LUMINANCE:
					DblPtr[0] = ClearAlpha;
					break;

				case IL_LUMINANCE_ALPHA:
					DblPtr[0] = ClearLum;
					DblPtr[1] = ClearAlpha;
					break;

				case IL_COLOUR_INDEX:
					DblPtr[0] = ClearAlpha;
					break;

				default:
					ilSetError(IL_INTERNAL_ERROR);
					return;
			}
			break;

		default:
			ilSetError(IL_INTERNAL_ERROR);
			return;
	}

	return;
}


//! Clears the current bound image to the values specified in ilClearColour
ILboolean ILAPIENTRY ilClearImage()
{
	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	return ilClearImage_(iCurImage);
}


ILAPI ILboolean ILAPIENTRY ilClearImage_(ILimage *Image)
{
	ILuint		i, c, NumBytes;
	ILubyte		Colours[32];  // Maximum is sizeof(double) * 4 = 32
	ILubyte		*BytePtr;
	ILushort	*ShortPtr;
	ILuint		*IntPtr;
	ILfloat		*FloatPtr;
	ILdouble	*DblPtr;

	NumBytes = Image->Bpp * Image->Bpc;
	ilGetClear(Colours, Image->Format, Image->Type);

	if (Image->Format != IL_COLOUR_INDEX) {
		switch (Image->Type)
		{
			case IL_BYTE:
			case IL_UNSIGNED_BYTE:
				BytePtr = (ILubyte*)Colours;
				for (c = 0; c < NumBytes; c += Image->Bpc) {
					for (i = c; i < Image->SizeOfData; i += NumBytes) {
						Image->Data[i] = BytePtr[c];
					}
				}
				break;

			case IL_SHORT:
			case IL_UNSIGNED_SHORT:
				ShortPtr = (ILushort*)Colours;
				for (c = 0; c < NumBytes; c += Image->Bpc) {
					for (i = c; i < Image->SizeOfData; i += NumBytes) {
						*((ILushort*)(Image->Data + i)) = ShortPtr[c];
					}
				}
				break;

			case IL_INT:
			case IL_UNSIGNED_INT:
				IntPtr = (ILuint*)Colours;
				for (c = 0; c < NumBytes; c += Image->Bpc) {
					for (i = c; i < Image->SizeOfData; i += NumBytes) {
						*((ILuint*)(Image->Data + i)) = IntPtr[c];
					}
				}
				break;

			case IL_FLOAT:
				FloatPtr = (ILfloat*)Colours;
				for (c = 0; c < NumBytes; c += Image->Bpc) {
					for (i = c; i < Image->SizeOfData; i += NumBytes) {
						*((ILfloat*)(Image->Data + i)) = FloatPtr[c];
					}
				}
				break;

			case IL_DOUBLE:
				DblPtr = (ILdouble*)Colours;
				for (c = 0; c < NumBytes; c += Image->Bpc) {
					for (i = c; i < Image->SizeOfData; i += NumBytes) {
						*((ILdouble*)(Image->Data + i)) = DblPtr[c];
					}
				}
				break;
		}
	}
	else {
                imemclear(Image->Data, Image->SizeOfData);
                
		if (Image->Pal.Palette)
			ifree(Image->Pal.Palette);
		Image->Pal.Palette = (ILubyte*)ialloc(4);
		if (Image->Pal.Palette == NULL) {
			return IL_FALSE;
		}

		Image->Pal.PalType = IL_PAL_RGBA32;
		Image->Pal.PalSize = 4;

		Image->Pal.Palette[0] = Colours[0] * UCHAR_MAX;
		Image->Pal.Palette[1] = Colours[1] * UCHAR_MAX;
		Image->Pal.Palette[2] = Colours[2] * UCHAR_MAX;
		Image->Pal.Palette[3] = Colours[3] * UCHAR_MAX;
	}

	return IL_TRUE;
}


//! Overlays the image found in Src on top of the current bound image at the coords specified.
ILboolean ILAPIENTRY ilOverlayImage(ILuint Source, ILint XCoord, ILint YCoord, ILint ZCoord)
{
	ILuint		x, y, z, SrcIndex, DestIndex, ConvBps, ConvSizePlane;
	ILint		c;
	ILimage		*Dest;//, *Src;
	ILubyte		*Converted;
	ILuint		DestName = ilGetCurName();
	ILfloat		FrontPer, BackPer;
	ILenum		DestOrigin, SrcOrigin;
	ILuint		StartX, StartY, StartZ, AlphaOff;
	ILubyte		*SrcTemp = NULL;
	ILboolean	DestFlipped = IL_FALSE;


	if (DestName == 0 || iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iCurImage->Origin == IL_ORIGIN_LOWER_LEFT) {  // Dest
		DestFlipped = IL_TRUE;
		ilFlipImage();
	}
	
	Dest = iCurImage;
	DestOrigin = iCurImage->Origin;
	ilBindImage(Source);
	SrcOrigin = iCurImage->Origin;

	if (iCurImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		SrcTemp = iGetFlipped(iCurImage);
		if (SrcTemp == NULL) {
			ilBindImage(DestName);
			if (DestFlipped)
				ilFlipImage();
			return IL_FALSE;
		}
	}
	else {
		SrcTemp = iCurImage->Data;
	}

	if (Dest == NULL || iCurImage == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	Converted = ilConvertBuffer(iCurImage->SizeOfData, iCurImage->Format, Dest->Format, iCurImage->Type, Dest->Type, SrcTemp);
	if (Converted == NULL)
		return IL_FALSE;

	ConvBps = Dest->Bpp * iCurImage->Width;
	ConvSizePlane = ConvBps * iCurImage->Height;

	StartX = XCoord >= 0 ? 0 : -XCoord;
	StartY = YCoord >= 0 ? 0 : -YCoord;
	StartZ = ZCoord >= 0 ? 0 : -ZCoord;

	if (iCurImage->Format == IL_RGBA || iCurImage->Format == IL_BGRA || iCurImage->Format == IL_LUMINANCE_ALPHA) {
		if (iCurImage->Format == IL_LUMINANCE_ALPHA)
			AlphaOff = 1;
		else
			AlphaOff = 3;

		for (z = StartZ; z < iCurImage->Depth && (ILint)z + ZCoord < (ILint)Dest->Depth; z++) {
			for (y = StartY; y < iCurImage->Height && (ILint)y + YCoord < (ILint)Dest->Height; y++) {
				for (x = StartX; x < iCurImage->Width && (ILint)x + XCoord < (ILint)Dest->Width; x++) {
					SrcIndex = z * ConvSizePlane + y * ConvBps + x * Dest->Bpp;
					DestIndex = (z + ZCoord) * Dest->SizeOfPlane + (y + YCoord) * Dest->Bps + (x + XCoord) * Dest->Bpp;
					FrontPer = iCurImage->Data[z * iCurImage->SizeOfPlane + y * iCurImage->Bps + x * iCurImage->Bpp + AlphaOff] / 255.0f;
					BackPer = 1.0f - FrontPer;
					for (c = 0; c < iCurImage->Bpp - 1; c++) {
						Dest->Data[DestIndex + c] =	(ILubyte)(Converted[SrcIndex + c] * FrontPer
							+ Dest->Data[DestIndex + c] * BackPer);
					}
					// Keep the original alpha.
					//Dest->Data[DestIndex + c + 1] = Dest->Data[DestIndex + c + 1];
				}
			}
		}
	}
	else {
		for (z = StartZ; z < iCurImage->Depth && z + ZCoord < Dest->Depth; z++) {
			for (y = StartY; y < iCurImage->Height && y + YCoord < Dest->Height; y++) {
				for (x = StartX; x < iCurImage->Width && x + XCoord < Dest->Width; x++) {
					for (c = 0; c < iCurImage->Bpp; c++) {
						Dest->Data[(z + ZCoord) * Dest->SizeOfPlane + (y + YCoord) * Dest->Bps + (x + XCoord) * Dest->Bpp + c] =
							Converted[z * ConvSizePlane + y * ConvBps + x * Dest->Bpp + c];
					}
				}
			}
		}
	}

	if (SrcTemp != iCurImage->Data)
		ifree(SrcTemp);

	ilBindImage(DestName);
	if (DestFlipped)
		ilFlipImage();

	ifree(Converted);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilBlit(ILuint Source, ILint DestX, ILint DestY, ILint DestZ, ILuint SrcX, ILuint SrcY, ILuint SrcZ, ILuint Width, ILuint Height, ILuint Depth)
{
	register ILuint		x, y, z, SrcIndex, DestIndex, ConvBps, ConvSizePlane;
	ILimage		*Dest;
	ILubyte		*Converted;
	ILuint		DestName = ilGetCurName();
	ILint		c;
	ILfloat		FrontPer, BackPer;
	ILenum		DestOrigin, SrcOrigin;
	ILuint		StartX, StartY, StartZ, AlphaOff;
	ILboolean	DestFlipped = IL_FALSE;
	ILubyte		*SrcTemp;


	if (DestName == 0 || iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iCurImage->Origin == IL_ORIGIN_LOWER_LEFT) {  // Dest
		DestFlipped = IL_TRUE;
		ilFlipImage();
	}
	
	Dest = iCurImage;
	DestOrigin = iCurImage->Origin;
	ilBindImage(Source);
	SrcOrigin = iCurImage->Origin;

	if (iCurImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		SrcTemp = iGetFlipped(iCurImage);
		if (SrcTemp == NULL) {
			ilBindImage(DestName);
			if (DestFlipped)
				ilFlipImage();
			return IL_FALSE;
		}
	}
	else {
		SrcTemp = iCurImage->Data;
	}

	if (Dest == NULL || iCurImage == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	Converted = ilConvertBuffer(iCurImage->SizeOfData, iCurImage->Format, Dest->Format, iCurImage->Type, Dest->Type, SrcTemp);
	if (Converted == NULL)
		return IL_FALSE;

	ConvBps = Dest->Bpp * iCurImage->Width;
	ConvSizePlane = ConvBps * iCurImage->Height;

	StartX = DestX >= 0 ? 0 : -DestX;
	StartY = DestY >= 0 ? 0 : -DestY;
	StartZ = DestZ >= 0 ? 0 : -DestZ;

	Width = Width + SrcX < Dest->Width ? Width + SrcX : Dest->Width;
	Height = Height + SrcY < Dest->Height ? Height + SrcY : Dest->Height;
	Depth = Depth + SrcZ < Dest->Depth ? Depth + SrcZ : Dest->Depth;

	if (iCurImage->Format == IL_RGBA || iCurImage->Format == IL_BGRA || iCurImage->Format == IL_LUMINANCE_ALPHA) {
		if (iCurImage->Format == IL_LUMINANCE_ALPHA)
			AlphaOff = 1;
		else
			AlphaOff = 3;

		for (z = StartZ; z < Depth && z + DestZ < Dest->Depth; z++) {
			for (y = StartY; y < Height && y + DestY < Dest->Height; y++) {
				for (x = StartX; x < Width && x + DestX < Dest->Width; x++) {
					SrcIndex = (z + SrcZ) * ConvSizePlane + (y + SrcY) * ConvBps + (x + SrcX) * Dest->Bpp;
					DestIndex = (z + DestZ) * Dest->SizeOfPlane + (y + DestY) * Dest->Bps + (x + DestX) * Dest->Bpp;
					// Use the original alpha
					FrontPer = iCurImage->Data[(z + SrcZ) * iCurImage->SizeOfPlane + (y + SrcY) * iCurImage->Bps + (x + SrcX) * iCurImage->Bpp + 3] / 255.0f;
					BackPer = 1.0f - FrontPer;
					for (c = 0; c < Dest->Bpp - 1; c++) {
						Dest->Data[DestIndex + c] =	(ILubyte)(Converted[SrcIndex + c] * FrontPer
							+ Dest->Data[DestIndex + c] * BackPer);
					}
					// Keep the original alpha.
					//Dest->Data[DestIndex + c + 1] = Dest->Data[DestIndex + c + 1];
				}
			}
		}
	}
	else {
		for (z = StartZ; z < Depth && z + DestZ < Dest->Depth; z++) {
			for (y = StartY; y < Height && y + DestY < Dest->Height; y++) {
				for (x = StartX; x < Width && x + DestX < Dest->Width; x++) {
					for (c = 0; c < Dest->Bpp; c++) {
						Dest->Data[(z + DestZ) * Dest->SizeOfPlane + (y + DestY) * Dest->Bps + (x + DestX) * Dest->Bpp + c] =
							Converted[(z + SrcZ) * ConvSizePlane + (y + SrcY) * ConvBps + (x + SrcX) * Dest->Bpp + c];
					}
				}
			}
		}
	}

	if (SrcTemp != iCurImage->Data)
		ifree(SrcTemp);

	ilBindImage(DestName);
	if (DestFlipped)
		ilFlipImage();

	ifree(Converted);

	return IL_TRUE;
}


ILboolean iCopySubImage(ILimage *Dest, ILimage *Src)
{
	ILimage *DestTemp, *SrcTemp;

	DestTemp = Dest;
	SrcTemp = Src;

	do {
		ilCopyImageAttr(DestTemp, SrcTemp);
		DestTemp->Data = (ILubyte*)ialloc(SrcTemp->SizeOfData);
		if (DestTemp->Data == NULL) {
			return IL_FALSE;
		}
		memcpy(DestTemp->Data, SrcTemp->Data, SrcTemp->SizeOfData);

		if (SrcTemp->Next) {
			DestTemp->Next = (ILimage*)icalloc(1, sizeof(ILimage));
			if (!DestTemp->Next) {
				return IL_FALSE;
			}
		}
		else {
			DestTemp->Next = NULL;
		}

		DestTemp = DestTemp->Next;
	} while ((SrcTemp = SrcTemp->Next));

	return IL_TRUE;
}

ILboolean iCopySubImages(ILimage *Dest, ILimage *Src)
{
	if (Src->Layers) {
		Dest->Layers = (ILimage*)icalloc(1, sizeof(ILimage));
		if (!Dest->Layers) {
			return IL_FALSE;
		}
		if (!iCopySubImage(Dest->Layers, Src->Layers))
			return IL_FALSE;
	}
	Dest->NumLayers = Src->NumLayers;

	if (Src->Mipmaps) {
		Dest->Mipmaps = (ILimage*)icalloc(1, sizeof(ILimage));
		if (!Dest->Mipmaps) {
			return IL_FALSE;
		}
		if (!iCopySubImage(Dest->Mipmaps, Src->Mipmaps))
			return IL_FALSE;
	}
	Dest->NumMips = Src->NumMips;

	if (Src->Next) {
		Dest->Next = (ILimage*)icalloc(1, sizeof(ILimage));
		if (!Dest->Next) {
			return IL_FALSE;
		}
		if (!iCopySubImage(Dest->Next, Src->Next))
			return IL_FALSE;
	}
	Dest->NumNext = Src->NumNext;

	return IL_TRUE;
}


// Copies everything but the Data from Src to Dest.
ILAPI ILboolean ILAPIENTRY ilCopyImageAttr(ILimage *Dest, ILimage *Src)
{
	if (Dest == NULL || Src == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (Dest->Pal.Palette && Dest->Pal.PalSize && Dest->Pal.PalType != IL_PAL_NONE) {
		ifree(Dest->Pal.Palette);
		Dest->Pal.Palette = NULL;
	}
	if (Dest->Layers) {
		ilCloseImage(Dest->Layers);
		Dest->Layers = NULL;
	}
	if (Dest->Mipmaps) {
		ilCloseImage(Dest->Mipmaps);
		Dest->Mipmaps = NULL;
	}
	if (Dest->Next) {
		ilCloseImage(Dest->Next);
		Dest->Next = NULL;
	}
	if (Dest->Profile) {
		ifree(Dest->Profile);
		Dest->Profile = NULL;
		Dest->ProfileSize = 0;
	}
	if (Dest->DxtcData) {
		ifree(Dest->DxtcData);
		Dest->DxtcData = NULL;
		Dest->DxtcFormat = IL_DXT_NO_COMP;
		Dest->DxtcSize = 0;
	}

	if (Src->AnimList && Src->AnimSize) {
		Dest->AnimList = (ILuint*)ialloc(Src->AnimSize * sizeof(ILuint));
		if (Dest->AnimList == NULL) {
			return IL_FALSE;
		}
		memcpy(Dest->AnimList, Src->AnimList, Src->AnimSize * sizeof(ILuint));
	}
	if (Src->Profile) {
		Dest->Profile = (ILubyte*)ialloc(Src->ProfileSize);
		if (Dest->Profile == NULL) {
			return IL_FALSE;
		}
		memcpy(Dest->Profile, Src->Profile, Src->ProfileSize);
		Dest->ProfileSize = Src->ProfileSize;
	}
	if (Src->Pal.Palette) {
		Dest->Pal.Palette = (ILubyte*)ialloc(Src->Pal.PalSize);
		if (Dest->Pal.Palette == NULL) {
			return IL_FALSE;
		}
		memcpy(Dest->Pal.Palette, Src->Pal.Palette, Src->Pal.PalSize);
	}
	else {
		Dest->Pal.Palette = NULL;
	}

	Dest->Pal.PalSize = Src->Pal.PalSize;
	Dest->Pal.PalType = Src->Pal.PalType;
	Dest->Width = Src->Width;
	Dest->Height = Src->Height;
	Dest->Depth = Src->Depth;
	Dest->Bpp = Src->Bpp;
	Dest->Bpc = Src->Bpc;
	Dest->Bps = Src->Bps;
	Dest->SizeOfPlane = Src->SizeOfPlane;
	Dest->SizeOfData = Src->SizeOfData;
	Dest->Format = Src->Format;
	Dest->Type = Src->Type;
	Dest->Origin = Src->Origin;
	Dest->Duration = Src->Duration;
	Dest->CubeFlags = Src->CubeFlags;
	Dest->AnimSize = Src->AnimSize;
	Dest->OffX = Src->OffX;
	Dest->OffY = Src->OffY;

	return IL_TRUE/*iCopySubImages(Dest, Src)*/;
}


//! Copies everything from Src to the current bound image.
ILboolean ILAPIENTRY ilCopyImage(ILuint Src)
{
	ILuint DestName = ilGetCurName();
	ILimage *DestImage = iCurImage, *SrcImage;

	if (iCurImage == NULL || DestName == 0) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ilBindImage(Src);
	SrcImage = iCurImage;
	ilBindImage(DestName);
	ilTexImage(SrcImage->Width, SrcImage->Height, SrcImage->Depth, SrcImage->Bpp, SrcImage->Format, SrcImage->Type, SrcImage->Data);
	ilCopyImageAttr(DestImage, SrcImage);

	return IL_TRUE;
}


// Creates a copy of Src and returns it.
ILAPI ILimage* ILAPIENTRY ilCopyImage_(ILimage *Src)
{
	ILimage *Dest;

	if (Src == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return NULL;
	}

	Dest = ilNewImage(Src->Width, Src->Height, Src->Depth, Src->Bpp, Src->Bpc);
	if (Dest == NULL) {
		return NULL;
	}

	if (ilCopyImageAttr(Dest, Src) == IL_FALSE)
		return NULL;

	memcpy(Dest->Data, Src->Data, Src->SizeOfData);

	return Dest;
}


ILuint ILAPIENTRY ilCloneCurImage()
{
	ILuint Id;
	ILimage *CurImage;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return 0;
	}

	ilGenImages(1, &Id);
	if (Id == 0)
		return 0;

	CurImage = iCurImage;

	ilBindImage(Id);
	ilTexImage(CurImage->Width, CurImage->Height, CurImage->Depth, CurImage->Bpp, CurImage->Format, CurImage->Type, CurImage->Data);
	ilCopyImageAttr(iCurImage, CurImage);

	iCurImage = CurImage;
	
	return Id;
}


// Like ilTexImage but doesn't destroy the palette.
ILAPI ILboolean ILAPIENTRY ilResizeImage(ILimage *Image, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte Bpc)
{
	if (Image == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (Image->Data != NULL)
		ifree(Image->Data);

	Image->Depth = Depth;
	Image->Width = Width;
	Image->Height = Height;
	Image->Bpp = Bpp;
	Image->Bpc = Bpc;
	Image->Bps = Bpp * Bpc * Width;
	Image->SizeOfPlane = Image->Bps * Height;
	Image->SizeOfData = Image->SizeOfPlane * Depth;

	Image->Data = (ILubyte*)ialloc(Image->SizeOfData);
	if (Image->Data == NULL) {
		return IL_FALSE;
	}

	return IL_TRUE;
}
