//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2002 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_rotate.c
//
// Description: Rotates an image.
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include "ilu_states.h"


ILboolean ILAPIENTRY iluRotate(ILfloat Angle)
{
	ILimage	*Temp, *Temp1, *CurImage = NULL;
	ILenum	PalType = 0;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iluCurImage->Format == IL_COLOUR_INDEX) {
		PalType = iluCurImage->Pal.PalType;
		CurImage = iluCurImage;
		iluCurImage = iConvertImage(iluCurImage, ilGetPalBaseType(CurImage->Pal.PalType), IL_UNSIGNED_BYTE);
	}

	Temp = iluRotate_(iluCurImage, Angle);
	if (Temp != NULL) {
		if (PalType != 0) {
			ilCloseImage(iluCurImage);
			Temp1 = iConvertImage(Temp, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE);
			ilCloseImage(Temp);
			Temp = Temp1;
			ilSetCurImage(CurImage);
		}
		ilTexImage(Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data);
		if (PalType != 0) {
			iluCurImage = ilGetCurImage();
			iluCurImage->Pal.PalSize = Temp->Pal.PalSize;
			iluCurImage->Pal.PalType = Temp->Pal.PalType;
			iluCurImage->Pal.Palette = (ILubyte*)ialloc(Temp->Pal.PalSize);
			if (iluCurImage->Pal.Palette == NULL) {
				ilCloseImage(Temp);
				return IL_FALSE;
			}
			memcpy(iluCurImage->Pal.Palette, Temp->Pal.Palette, Temp->Pal.PalSize);
		}

		iluCurImage->Origin = Temp->Origin;
		ilCloseImage(Temp);
		return IL_TRUE;
	}
	return IL_FALSE;
}


ILboolean ILAPIENTRY iluRotate3D(ILfloat x, ILfloat y, ILfloat z, ILfloat Angle)
{
	ILimage *Temp;

return IL_FALSE;

	iluCurImage = ilGetCurImage();
	Temp = iluRotate3D_(iluCurImage, x, y, z, Angle);
	if (Temp != NULL) {
		ilTexImage(Temp->Width, Temp->Height, Temp->Depth, Temp->Bpp, Temp->Format, Temp->Type, Temp->Data);
		iluCurImage->Origin = Temp->Origin;
		ilSetPal(&Temp->Pal);
		ilCloseImage(Temp);
		return IL_TRUE;
	}
	return IL_FALSE;
}


ILAPI ILimage* ILAPIENTRY iluRotate_(ILimage *Image, ILfloat Angle)
{
	ILimage		*Rotated = NULL;
	ILuint		x, y, c;
	ILfloat		x0, y0, x1, y1;
	ILfloat		HalfRotW, HalfRotH, HalfImgW, HalfImgH, Cos, Sin;
	ILuint		RotOffset, ImgOffset;
	ILint		XCorner[4], YCorner[4], MaxX, MaxY;
	ILushort	*ShortPtr;
	ILuint		*IntPtr;

	Rotated = (ILimage*)icalloc(1, sizeof(ILimage));
	if (Rotated == NULL)
		return NULL;
	if (ilCopyImageAttr(Rotated, Image) == IL_FALSE) {
		ilCloseImage(Rotated);
		return NULL;
	}
	// Precalculate shit
	HalfImgW = Image->Width / 2.0f;
	HalfImgH = Image->Height / 2.0f;
	Cos = ilCos(Angle);
	Sin = ilSin(Angle);

	// Find where edges are in new image (origin in center).
	XCorner[0] = ilRound(-HalfImgW * Cos - -HalfImgH * Sin);
	YCorner[0] = ilRound(-HalfImgW * Sin + -HalfImgH * Cos);
	XCorner[1] = ilRound(HalfImgW * Cos - -HalfImgH * Sin);
	YCorner[1] = ilRound(HalfImgW * Sin + -HalfImgH * Cos);
	XCorner[2] = ilRound(HalfImgW * Cos - HalfImgH * Sin);
	YCorner[2] = ilRound(HalfImgW * Sin + HalfImgH * Cos);
	XCorner[3] = ilRound(-HalfImgW * Cos - HalfImgH * Sin);
	YCorner[3] = ilRound(-HalfImgW * Sin + HalfImgH * Cos);

	MaxX = 0;  MaxY = 0;
	for (x = 0; x < 4; x++) {
		if (XCorner[x] > MaxX)
			MaxX = XCorner[x];
		if (YCorner[x] > MaxY)
			MaxY = YCorner[x];
	}

	if (ilResizeImage(Rotated, MaxX * 2, MaxY * 2, 1, Image->Bpp, Image->Bpc) == IL_FALSE) {
		ilCloseImage(Rotated);
		return IL_FALSE;
	}

	HalfRotW = Rotated->Width / 2.0f;
	HalfRotH = Rotated->Height / 2.0f;

	ilClearImage_(Rotated);

	ShortPtr = (ILushort*)iluCurImage->Data;
	IntPtr = (ILuint*)iluCurImage->Data;

	//if (iluFilter == ILU_NEAREST) {
	switch (iluCurImage->Bpc)
	{
		case 1:
			for (y = 0; y < Rotated->Height; y++) {
				y0 = y - HalfRotH;
				for (x = 0; x < Rotated->Width; x++) {
					x0 = x - HalfRotW;
					x1 = x0 * Cos - y0 * Sin;
					y1 = x0 * Sin + y0 * Cos;
					x1 += HalfImgW;
					y1 += HalfImgH;

					if (x1 < Image->Width && x1 >= 0 && y1 < Image->Height && y1 >= 0) {
						RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = (ILuint)y1 * Image->Bps + (ILuint)x1 * Image->Bpp;
						for (c = 0; c < Image->Bpp; c++) {
							Rotated->Data[RotOffset + c] = Image->Data[ImgOffset + c];
						}
					}
				}
			}
			break;
		case 2:
			Image->Bps /= 2;
			Rotated->Bps /= 2;
			for (y = 0; y < Rotated->Height; y++) {
				y0 = y - HalfRotH;
				for (x = 0; x < Rotated->Width; x++) {
					x0 = x - HalfRotW;
					x1 = x0 * Cos - y0 * Sin;
					y1 = x0 * Sin + y0 * Cos;
					x1 += HalfImgW;
					y1 += HalfImgH;

					if (x1 < Image->Width && x1 >= 0 && y1 < Image->Height && y1 >= 0) {
						RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = (ILuint)y1 * Image->Bps + (ILuint)x1 * Image->Bpp;
						for (c = 0; c < Image->Bpp; c++) {
							((ILushort*)(Rotated->Data))[RotOffset + c] = ShortPtr[ImgOffset + c];
						}
					}
				}
			}
			Image->Bps *= 2;
			Rotated->Bps *= 2;
			break;
		case 4:
			Image->Bps /= 4;
			Rotated->Bps /= 4;
			for (y = 0; y < Rotated->Height; y++) {
				y0 = y - HalfRotH;
				for (x = 0; x < Rotated->Width; x++) {
					x0 = x - HalfRotW;
					x1 = x0 * Cos - y0 * Sin;
					y1 = x0 * Sin + y0 * Cos;
					x1 += HalfImgW;
					y1 += HalfImgH;

					if (x1 < Image->Width && x1 >= 0 && y1 < Image->Height && y1 >= 0) {
						RotOffset = y * Rotated->Bps + x * Rotated->Bpp;
						ImgOffset = (ILuint)y1 * Image->Bps + (ILuint)x1 * Image->Bpp;
						for (c = 0; c < Image->Bpp; c++) {
							((ILuint*)(Rotated->Data))[RotOffset + c] = IntPtr[ImgOffset + c];
						}
					}
				}
			}
			Image->Bps *= 4;
			Rotated->Bps *= 4;
			break;
	}
	//}

	return Rotated;
}


ILAPI ILimage* ILAPIENTRY iluRotate3D_(ILimage *Image, ILfloat x, ILfloat y, ILfloat z, ILfloat Angle)
{

	return NULL;
}
