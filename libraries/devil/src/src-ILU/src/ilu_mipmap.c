//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 08/11/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_manip.c
//
// Description: Generates mipmaps for the current image
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include "ilu_mipmap.h"
#include "ilu_states.h"


ILimage *Original;  // So we can increment NumMips


// Note:  If any image is a non-power-of-2 image, it will automatically be
//	converted to a power-of-2 image by iluScaleImage, which is likely to
//	uglify the image. =]


// @TODO:  Check for 1 bpp textures!


// Currently changes all textures to powers of 2...should we change?
ILboolean iluBuild2DMipmaps()
{
	ILimage		*Temp;
	ILboolean	Resized = IL_FALSE;
	ILuint		Width, Height;
	ILenum		OldFilter;

	iluCurImage = Original = ilGetCurImage();
	if (Original == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Get rid of any existing mipmaps.
	if (Original->Mipmaps) {
		ilCloseImage(Original->Mipmaps);
		Original->Mipmaps = NULL;
	}
	Original->NumMips = 0;

	Width = ilNextPower2(iluCurImage->Width);
	Height = ilNextPower2(iluCurImage->Height);
	if (iluCurImage->Width != Width || iluCurImage->Height != Height) {
		Resized = IL_TRUE;
		Temp = ilCopyImage_(ilGetCurImage());
		ilSetCurImage(Temp);
		OldFilter = iluFilter;
		iluImageParameter(ILU_FILTER, ILU_BILINEAR);
		iluScale(Width, Height, 1);
		iluImageParameter(ILU_FILTER, OldFilter);
		iluCurImage = ilGetCurImage();
	}

	CurMipMap = NULL;
	iBuild2DMipmaps_(iluCurImage->Width >> 1, iluCurImage->Height >> 1);

	if (Resized) {
		Original->Mipmaps = iluCurImage->Mipmaps;
		iluCurImage->Mipmaps = NULL;
		ilCloseImage(iluCurImage);
		ilSetCurImage(Original);
	}

	return IL_TRUE;
}


ILboolean iluBuild3DMipmaps()
{
	ILimage		*Temp;
	ILboolean	Resized = IL_FALSE;
	ILuint		Width, Height, Depth;

	iluCurImage = Original = ilGetCurImage();
	if (Original == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Get rid of any existing mipmaps.
	if (Original->Mipmaps) {
		ilCloseImage(Original->Mipmaps);
		Original->Mipmaps = NULL;
	}
	Original->NumMips = 0;

	Width = ilNextPower2(iluCurImage->Width);
	Height = ilNextPower2(iluCurImage->Height);
	Depth = ilNextPower2(iluCurImage->Depth);
	if (iluCurImage->Width != Width || iluCurImage->Height != Height || iluCurImage->Depth != Depth) {
		Resized = IL_TRUE;
		Temp = ilCopyImage_(ilGetCurImage());
		ilSetCurImage(Temp);
		iluImageParameter(ILU_FILTER, ILU_BILINEAR);
		iluScale(Width, Height, Depth);
		iluImageParameter(ILU_FILTER, iluFilter);
		iluCurImage = ilGetCurImage();
	}

	CurMipMap = NULL;
	iBuild3DMipmaps_(iluCurImage->Width >> 1, iluCurImage->Height >> 1, iluCurImage->Depth >> 1);

	if (Resized) {
		Original->Mipmaps = iluCurImage->Mipmaps;
		iluCurImage->Mipmaps = NULL;
		ilCloseImage(iluCurImage);
		ilSetCurImage(Original);
	}

	return IL_TRUE;
}


ILboolean ILAPIENTRY iluBuildMipmaps()
{
	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iluCurImage->Depth > 1)
		return iluBuild3DMipmaps();
	/*if (iluCurImage->Height <= 1)
		return iluBuild1DMipmaps();*/  // 8-8-2001
	return iluBuild2DMipmaps();
}


ILboolean iBuild1DMipmaps_(ILuint Width)
{
	ILimage *MipMap;
	ILuint i, j, c;

	if (CurMipMap->Width <= 1) {  // Already at the last mipmap
		CurMipMap->Next = NULL;  // Terminate the list
		return IL_TRUE;
	}

	MipMap = ilNewImage(Width, 1, 1, iluCurImage->Bpp, iluCurImage->Bpc);
	if (MipMap == NULL) {
		if (CurMipMap != NULL)
			CurMipMap->Next = NULL;
		return IL_FALSE;
	}

	// Copies attributes
	MipMap->Origin = iluCurImage->Origin;  // 8-11-2001
	MipMap->Format = iluCurImage->Format;
	MipMap->Type = iluCurImage->Type;
	MipMap->Pal.PalSize = iluCurImage->Pal.PalSize;
	MipMap->Pal.PalType = iluCurImage->Pal.PalType;
	if (iluCurImage->Pal.Palette && MipMap->Pal.PalSize > 0 && MipMap->Pal.PalType != IL_PAL_NONE) {
		MipMap->Pal.Palette = (ILubyte*)ialloc(iluCurImage->Pal.PalSize);
		if (MipMap->Pal.Palette == NULL) {
			ilCloseImage(MipMap);
			return IL_FALSE;
		}
		memcpy(MipMap->Pal.Palette, iluCurImage->Pal.Palette, MipMap->Pal.PalSize);
	}

	if (CurMipMap == NULL) {  // First mipmap
		iluCurImage->Mipmaps = MipMap;
	}
	else {
		CurMipMap->Next = MipMap;
	}

	for (c = 0; c < CurMipMap->Bpp; c++) {  // 8-12-2001
		for (i = 0, j = 0; i < Width; i++) {
			MipMap->Data[i * MipMap->Bpp + c] =  CurMipMap->Data[(j++ * MipMap->Bpp) + c];
						MipMap->Data[i * MipMap->Bpp + c] += CurMipMap->Data[(j++ * MipMap->Bpp) + c];
						MipMap->Data[i * MipMap->Bpp + c] >>= 1;
		}
	}
	// 8-11-2001
	CurMipMap = MipMap;
	iBuild1DMipmaps_(MipMap->Width >> 1);
	Original->NumMips++;

	return IL_TRUE;
}


ILboolean iBuild1DMipmapsVertical_(ILuint Height)
{
	ILimage *MipMap, *Src;
	ILuint i = 0, j = 0, c;

	if (CurMipMap->Height <= 1) {  // Already at the last mipmap
		CurMipMap->Next = NULL;  // Terminate the list
		return IL_TRUE;
	}

	MipMap = ilNewImage(1, Height, 1, iluCurImage->Bpp, iluCurImage->Bpc);
	if (MipMap == NULL) {
		if (CurMipMap != NULL)
			CurMipMap->Next = NULL;
		return IL_FALSE;
	}

	// Copies attributes
	MipMap->Origin = iluCurImage->Origin;  // 8-11-2001
	MipMap->Format = iluCurImage->Format;
	MipMap->Type = iluCurImage->Type;
	MipMap->Pal.PalSize = iluCurImage->Pal.PalSize;
	MipMap->Pal.PalType = iluCurImage->Pal.PalType;
	if (iluCurImage->Pal.Palette && MipMap->Pal.PalSize > 0 && MipMap->Pal.PalType != IL_PAL_NONE) {
		MipMap->Pal.Palette = (ILubyte*)ialloc(iluCurImage->Pal.PalSize);
		if (MipMap->Pal.Palette == NULL) {
			ilCloseImage(MipMap);
			return IL_FALSE;
		}
		memcpy(MipMap->Pal.Palette, iluCurImage->Pal.Palette, MipMap->Pal.PalSize);
	}

	if (CurMipMap == NULL) {  // First mipmap
		iluCurImage->Mipmaps = MipMap;
		Src = iluCurImage;
	}
	else {
		CurMipMap->Next = MipMap;
		Src = CurMipMap;
	}

	for (c = 0; c < CurMipMap->Bpp; c++) {  // 8-12-2001
		//j = 0;
		for (i = 0, j = 0; i < Height; i++) {
			MipMap->Data[i * MipMap->Bpp + c] =  CurMipMap->Data[(j++ * MipMap->Bpp) + c];
						MipMap->Data[i * MipMap->Bpp + c] += CurMipMap->Data[(j++ * MipMap->Bpp) + c];
						MipMap->Data[i * MipMap->Bpp + c] >>= 1;
		}
	}
	// 8-11-2001
	CurMipMap = MipMap;
	iBuild1DMipmapsVertical_(MipMap->Height >> 1);
	Original->NumMips++;

	return IL_TRUE;
}


ILboolean iBuild2DMipmaps_(ILuint Width, ILuint Height)
{
	ILimage *MipMap, *Src;
	ILuint	x1 = 0, x2 = 0, y1 = 0, y2 = 0, c;

	if (CurMipMap) {
		if (CurMipMap->Width == 1 && CurMipMap->Height == 1) {  // Already at the last mipmap
			CurMipMap->Next = NULL;  // Terminate the list
			return IL_TRUE;
		}

		if (/*CurMipMap->*/Height == 1) {
			return iBuild1DMipmaps_(Width);
		}

		if (/*CurMipMap->*/Width == 1) {
			return iBuild1DMipmapsVertical_(Height);
		}
	}
	else if (iluCurImage->Width <= 1 && iluCurImage->Height <= 1) {  // 8-9-2001 - Not sure...
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Height == 0 && Width == 0) {
		ilSetError(ILU_INTERNAL_ERROR);
		return IL_FALSE;
	}
	// 8-9-2001 - Should these two if statements read == 0 || == 1?
	if (Height == 0) {
		return iBuild1DMipmaps_(Width);
	}
	if (Width == 0) {
		return iBuild1DMipmapsVertical_(Height);  // 8-9-2001
	}

	MipMap = ilNewImage(Width, Height, 1, iluCurImage->Bpp, iluCurImage->Bpc);
	if (MipMap == NULL) {
		if (CurMipMap != NULL)
			CurMipMap->Next = NULL;
		return IL_FALSE;
	}

	// Copies attributes
	MipMap->Origin = iluCurImage->Origin;  // 8-11-2001
	MipMap->Format = iluCurImage->Format;
	MipMap->Type = iluCurImage->Type;
	MipMap->Pal.PalSize = iluCurImage->Pal.PalSize;
	MipMap->Pal.PalType = iluCurImage->Pal.PalType;
	if (iluCurImage->Pal.Palette && MipMap->Pal.PalSize > 0 && MipMap->Pal.PalType != IL_PAL_NONE) {
		MipMap->Pal.Palette = (ILubyte*)ialloc(iluCurImage->Pal.PalSize);
		if (MipMap->Pal.Palette == NULL) {
			ilCloseImage(MipMap);
			return IL_FALSE;
		}
		memcpy(MipMap->Pal.Palette, iluCurImage->Pal.Palette, MipMap->Pal.PalSize);
	}

	if (CurMipMap == NULL) {  // First mipmap
		iluCurImage->Mipmaps = MipMap;
		Src = iluCurImage;
	}
	else {
		CurMipMap->Next = MipMap;
		Src = CurMipMap;
	}

	if (MipMap->Type == IL_FLOAT) {
		ILfloat *DestFData = (ILfloat*)MipMap->Data;
		ILfloat *SrcFData = (ILfloat*)Src->Data;
		ILuint DestStride = MipMap->Bps / 4;
		ILuint SrcStride = Src->Bps / 4;
		for (y1 = 0; y1 < Height; y1++, y2 += 2) {
			x1 = 0;  x2 = 0;
			for (; x1 < Width; x1++, x2 += 2) {
				for (c = 0; c < MipMap->Bpp; c++) {
					DestFData[y1 * DestStride + x1 * MipMap->Bpp + c] = (
						SrcFData[y2 * SrcStride + x2 * MipMap->Bpp + c] +
						SrcFData[y2 * SrcStride + (x2 + 1) * MipMap->Bpp + c] +
						SrcFData[(y2 + 1) * SrcStride + x2 * MipMap->Bpp + c] +
						SrcFData[(y2 + 1) * SrcStride + (x2 + 1) * MipMap->Bpp + c]) * .25f;
				}
			}
		}
	}
	else {
		for (y1 = 0; y1 < Height; y1++, y2 += 2) {
			x1 = 0;  x2 = 0;
			for (; x1 < Width; x1++, x2 += 2) {
				for (c = 0; c < MipMap->Bpp; c++) {
					MipMap->Data[y1 * MipMap->Bps + x1 * MipMap->Bpp + c] = (
						Src->Data[y2 * Src->Bps + x2 * MipMap->Bpp + c] +
						Src->Data[y2 * Src->Bps + (x2 + 1) * MipMap->Bpp + c] +
						Src->Data[(y2 + 1) * Src->Bps + x2 * MipMap->Bpp + c] +
						Src->Data[(y2 + 1) * Src->Bps + (x2 + 1) * MipMap->Bpp + c]) >> 2;
				}
			}
		}
	}

	CurMipMap = MipMap;
	iBuild2DMipmaps_(MipMap->Width >> 1, MipMap->Height >> 1);
	Original->NumMips++;

	return IL_TRUE;
}


ILboolean iBuild3DMipmaps_(ILuint Width, ILuint Height, ILuint Depth)
{
	ILimage *MipMap, *Src;
	ILuint	x1 = 0, x2 = 0, y1 = 0, y2 = 0, z1 = 0, z2 = 0, z3, z4, z5, c;

	if (CurMipMap) {
		// Already at the last mipmap
		if (CurMipMap->Width == 1 && CurMipMap->Height == 1 && CurMipMap->Depth == 1) {
			CurMipMap->Next = NULL;  // Terminate the list
			return IL_TRUE;
		}

		if (CurMipMap->Depth == 1) {
			return iBuild2DMipmaps_(Width, Height);
		}
		if (CurMipMap->Height == 1) {
			return iBuild3DMipmapsHorizontal_(Width, Depth);
		}

		if (CurMipMap->Width == 1) {
			return iBuild3DMipmapsVertical_(Height, Depth);
		}
	}
	else if (iluCurImage->Width <= 1 && iluCurImage->Height <= 1 && iluCurImage->Height <= 1) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Height == 0 && Width == 0 && Depth == 0) {
		ilSetError(ILU_INTERNAL_ERROR);
		return IL_FALSE;
	}
	if (Depth == 0) {
		return iBuild2DMipmaps_(Width, Height);
	}
	if (Height == 0) {
		return iBuild3DMipmapsHorizontal_(Width, Depth);
	}
	if (Width == 0) {
		return iBuild3DMipmapsVertical_(Height, Depth);
	}

	MipMap = ilNewImage(Width, Height, Depth, iluCurImage->Bpp, iluCurImage->Bpc);
	if (MipMap == NULL) {
		if (CurMipMap != NULL)
			CurMipMap->Next = NULL;
		return IL_FALSE;
	}

	// Copies attributes
	MipMap->Origin = iluCurImage->Origin;  // 8-11-2001
	MipMap->Format = iluCurImage->Format;
	MipMap->Type = iluCurImage->Type;
	MipMap->Pal.PalSize = iluCurImage->Pal.PalSize;
	MipMap->Pal.PalType = iluCurImage->Pal.PalType;
	if (iluCurImage->Pal.Palette && MipMap->Pal.PalSize > 0 && MipMap->Pal.PalType != IL_PAL_NONE) {
		MipMap->Pal.Palette = (ILubyte*)ialloc(iluCurImage->Pal.PalSize);
		if (MipMap->Pal.Palette == NULL) {
			ilCloseImage(MipMap);
			return IL_FALSE;
		}
		memcpy(MipMap->Pal.Palette, iluCurImage->Pal.Palette, MipMap->Pal.PalSize);
	}

	if (CurMipMap == NULL) {  // First mipmap
		iluCurImage->Mipmaps = MipMap;
		Src = iluCurImage;
	}
	else {
		CurMipMap->Next = MipMap;
		Src = CurMipMap;
	}

	for (z1 = 0; z1 < Depth; z1++, z2 += 2) {
		z3 = z1 * MipMap->SizeOfPlane;
		z4 = z2 * Src->SizeOfPlane;
		z5 = (z2 + 1) * Src->SizeOfPlane;

		y1 = 0;  y2 = 0;
		for (; y1 < Height; y1++, y2 += 2) {
 			x1 = 0;  x2 = 0;
			for (; x1 < Width; x1++, x2 += 2) {
				for (c = 0; c < MipMap->Bpp; c++) {
					MipMap->Data[z1 * MipMap->SizeOfPlane + y1 * MipMap->Bps + x1 * MipMap->Bpp + c] = (
						Src->Data[z2 * Src->SizeOfPlane + y2 * Src->Bps + x2 * Src->Bpp + c] +
						Src->Data[z2 * Src->SizeOfPlane + y2 * Src->Bps + (x2 + 1) * Src->Bpp + c] +
						Src->Data[z2 * Src->SizeOfPlane + (y2 + 1) * Src->Bps + x2 * Src->Bpp + c] +
						Src->Data[z2 * Src->SizeOfPlane + (y2 + 1) * Src->Bps + (x2 + 1) * Src->Bpp + c] +
						Src->Data[(z2 + 1) * Src->SizeOfPlane + y2 * Src->Bps + x2 * Src->Bpp + c] +
						Src->Data[(z2 + 1) * Src->SizeOfPlane + y2 * Src->Bps + (x2 + 1) * Src->Bpp + c] +
						Src->Data[(z2 + 1) * Src->SizeOfPlane + (y2 + 1) * Src->Bps + x2 * Src->Bpp + c] +
						Src->Data[(z2 + 1) * Src->SizeOfPlane + (y2 + 1) * Src->Bps + (x2 + 1) * Src->Bpp + c]) / 8;
				}
			}
		}
	}

	CurMipMap = MipMap;
	iBuild3DMipmaps_(MipMap->Width >> 1, MipMap->Height >> 1, MipMap->Depth >> 1);
	Original->NumMips++;

	return IL_TRUE;
}


ILboolean iBuild3DMipmapsVertical_(ILuint Height, ILuint Depth)
{
	ILimage *MipMap, *Src;
	ILuint	y1 = 0, y2 = 0, z1 = 0, z2 = 0, z3, z4, z5, c;

	if (CurMipMap) {
		// Already at the last mipmap
		if (CurMipMap->Width == 1 && CurMipMap->Height == 1 && CurMipMap->Depth == 1) {
			CurMipMap->Next = NULL;  // Terminate the list
			return IL_TRUE;
		}

		if (CurMipMap->Depth == 1) {
			return iBuild1DMipmapsVertical_(Height);
		}
	}
	else if (iluCurImage->Height <= 1 && iluCurImage->Height <= 1) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Height == 0 && Depth == 0) {
		ilSetError(ILU_INTERNAL_ERROR);
		return IL_FALSE;
	}
	if (Depth == 0) {
		return iBuild1DMipmapsVertical_(Height);
	}

	MipMap = ilNewImage(1, Height, Depth, iluCurImage->Bpp, iluCurImage->Bpc);
	if (MipMap == NULL) {
		if (CurMipMap != NULL)
			CurMipMap->Next = NULL;
		return IL_FALSE;
	}

	// Copies attributes
	MipMap->Format = iluCurImage->Format;
	MipMap->Type = iluCurImage->Type;
	MipMap->Pal.PalSize = iluCurImage->Pal.PalSize;
	MipMap->Pal.PalType = iluCurImage->Pal.PalType;
	if (iluCurImage->Pal.Palette && MipMap->Pal.PalSize > 0 && MipMap->Pal.PalType != IL_PAL_NONE) {
		MipMap->Pal.Palette = (ILubyte*)ialloc(iluCurImage->Pal.PalSize);
		if (MipMap->Pal.Palette == NULL) {
			ilCloseImage(MipMap);
			return IL_FALSE;
		}
		memcpy(MipMap->Pal.Palette, iluCurImage->Pal.Palette, MipMap->Pal.PalSize);
	}

	if (CurMipMap == NULL) {  // First mipmap
		iluCurImage->Mipmaps = MipMap;
		Src = iluCurImage;
	}
	else {
		CurMipMap->Next = MipMap;
		Src = CurMipMap;
	}

	for (z1 = 0; z1 < Depth; z1++, z2 += 2) {
		z3 = z1 * iluCurImage->SizeOfPlane;
		z4 = z2 * iluCurImage->SizeOfPlane;
		z5 = (z2 + 1) * iluCurImage->SizeOfPlane;

		for (y1 = 0; y1 < Height; y1++, y2 += 2) {
			for (c = 0; c < MipMap->Bpp; c++) {
				MipMap->Data[z3 + y1 * MipMap->Bps + c] = (
					Src->Data[z4 + y2 * Src->Bps + c] +
					Src->Data[z4 + y2 * Src->Bps + c] +
					Src->Data[z4 + (y2 + 1) * Src->Bps + c] +
					Src->Data[z4 + (y2 + 1) * Src->Bps + c]) >> 2;
			}
		}
	}

	CurMipMap = MipMap;
	iBuild3DMipmapsVertical_(MipMap->Height >> 1, MipMap->Depth >> 1);
	Original->NumMips++;

	return IL_TRUE;
}


ILboolean iBuild3DMipmapsHorizontal_(ILuint Width, ILuint Depth)
{
	ILimage *MipMap, *Src;
	ILuint	x1 = 0, x2 = 0, z1 = 0, z2 = 0, z3, z4, z5, c;

	if (CurMipMap) {
		// Already at the last mipmap
		if (CurMipMap->Width == 1 && CurMipMap->Height == 1 && CurMipMap->Depth == 1) {
			CurMipMap->Next = NULL;  // Terminate the list
			return IL_TRUE;
		}

		if (CurMipMap->Depth == 1) {
			return iBuild1DMipmaps_(Width);
		}
	}
	else if (iluCurImage->Width <= 1 && iluCurImage->Height <= 1) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Width == 0 && Depth == 0) {
		ilSetError(ILU_INTERNAL_ERROR);
		return IL_FALSE;
	}
	if (Depth == 0) {
		return iBuild1DMipmaps_(Width);
	}

	MipMap = ilNewImage(Width, 1, Depth, iluCurImage->Bpp, iluCurImage->Bpc);
	if (MipMap == NULL) {
		if (CurMipMap != NULL)
			CurMipMap->Next = NULL;
		return IL_FALSE;
	}

	// Copies attributes
	MipMap->Format = iluCurImage->Format;
	MipMap->Type = iluCurImage->Type;
	MipMap->Pal.PalSize = iluCurImage->Pal.PalSize;
	MipMap->Pal.PalType = iluCurImage->Pal.PalType;
	if (iluCurImage->Pal.Palette && MipMap->Pal.PalSize > 0 && MipMap->Pal.PalType != IL_PAL_NONE) {
		MipMap->Pal.Palette = (ILubyte*)ialloc(iluCurImage->Pal.PalSize);
		if (MipMap->Pal.Palette == NULL) {
			ilCloseImage(MipMap);
			return IL_FALSE;
		}
		memcpy(MipMap->Pal.Palette, iluCurImage->Pal.Palette, MipMap->Pal.PalSize);
	}

	if (CurMipMap == NULL) {  // First mipmap
		iluCurImage->Mipmaps = MipMap;
		Src = iluCurImage;
	}
	else {
		CurMipMap->Next = MipMap;
		Src = CurMipMap;
	}

	for (z1 = 0; z1 < Depth; z1++, z2 += 2) {
		z3 = z1 * iluCurImage->SizeOfPlane;
		z4 = z2 * iluCurImage->SizeOfPlane;
		z5 = (z2 + 1) * iluCurImage->SizeOfPlane;

		x1 = 0;  x2 = 0;
		for (; x1 < Width; x1++, x2 += 2) {
			for (c = 0; c < MipMap->Bpp; c++) {
				MipMap->Data[z3 + x1 * MipMap->Bpp + c] = (
					Src->Data[z4 + x2 * MipMap->Bpp + c] +
					Src->Data[z4 + (x2 + 1) * MipMap->Bpp + c] +
					Src->Data[z4 + x2 * MipMap->Bpp + c] +
					Src->Data[z4 + (x2 + 1) * MipMap->Bpp + c]) >> 2;
			}
		}
	}

	CurMipMap = MipMap;
	iBuild3DMipmapsHorizontal_(MipMap->Width >> 1, MipMap->Depth >> 1);
	Original->NumMips++;

	return IL_TRUE;
}
