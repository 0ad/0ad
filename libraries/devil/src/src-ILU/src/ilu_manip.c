//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 02/16/2002 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_manip.c
//
// Description: Manipulates an image in several ways.
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include "ilu_states.h"
#include <float.h>
#include <limits.h>


ILboolean iluCrop2D(ILuint XOff, ILuint YOff, ILuint Width, ILuint Height)
{
	ILuint	x, y, c, OldBps;
	ILubyte	*Data;
	ILenum	Origin;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Uh-oh, what about 0 dimensions?!
	if (Width > iluCurImage->Width || Height > iluCurImage->Height) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Data = (ILubyte*)ialloc(iluCurImage->SizeOfData);
	if (Data == NULL) {
		return IL_FALSE;
	}

	OldBps = iluCurImage->Bps;
	Origin = iluCurImage->Origin;
	ilCopyPixels(0, 0, 0, iluCurImage->Width, iluCurImage->Height, 1, iluCurImage->Format, iluCurImage->Type, Data);
	if (!ilTexImage(Width, Height, iluCurImage->Depth, iluCurImage->Bpp, iluCurImage->Format, iluCurImage->Type, NULL)) {
		free(Data);
		return IL_FALSE;
	}
	iluCurImage->Origin = Origin;

	// @TODO:  Optimize!  (Especially XOff * iluCurImage->Bpp...get rid of it!)
	for (y = 0; y < iluCurImage->Height; y++) {
		for (x = 0; x < iluCurImage->Bps; x += iluCurImage->Bpp) {
			for (c = 0; c < iluCurImage->Bpp; c++) {
				iluCurImage->Data[y * iluCurImage->Bps + x + c] = 
					Data[(y + YOff) * OldBps + x + XOff * iluCurImage->Bpp + c];
			}
		}
	}

	ifree(Data);

	return IL_TRUE;
}


ILboolean iluCrop3D(ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth)
{
	ILuint	x, y, z, c, OldBps, OldPlane;
	ILubyte	*Data;
	ILenum	Origin;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Uh-oh, what about 0 dimensions?!
	if (Width > iluCurImage->Width || Height > iluCurImage->Height || Depth > iluCurImage->Depth) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Data = (ILubyte*)ialloc(iluCurImage->SizeOfData);
	if (Data == NULL) {
		return IL_FALSE;
	}

	OldBps = iluCurImage->Bps;
	OldPlane = iluCurImage->SizeOfPlane;
	Origin = iluCurImage->Origin;
	ilCopyPixels(0, 0, 0, iluCurImage->Width, iluCurImage->Height, iluCurImage->Depth, iluCurImage->Format, iluCurImage->Type, Data);
	if (!ilTexImage(Width - XOff, Height - YOff, Depth - ZOff, iluCurImage->Bpp, iluCurImage->Format, iluCurImage->Type, NULL)) {
		ifree(Data);
	}
	iluCurImage->Origin = Origin;

	for (z = 0; z < iluCurImage->Depth; z++) {
		for (y = 0; y < iluCurImage->Height; y++) {
			for (x = 0; x < iluCurImage->Bps; x += iluCurImage->Bpp) {
				for (c = 0; c < iluCurImage->Bpp; c++) {
					iluCurImage->Data[z * iluCurImage->SizeOfPlane + y * iluCurImage->Bps + x + c] = 
						Data[(z + ZOff) * OldPlane + (y + YOff) * OldBps + (x + XOff) + c];
				}
			}
		}
	}

	ifree(Data);

	return IL_TRUE;
}


ILboolean ILAPIENTRY iluCrop(ILuint XOff, ILuint YOff, ILuint ZOff, ILuint Width, ILuint Height, ILuint Depth)
{
	if (ZOff <= 1)
		return iluCrop2D(XOff, YOff, Width, Height);
	return iluCrop3D(XOff, YOff, ZOff, Width, Height, Depth);
}


//! Enlarges the canvas
ILboolean ILAPIENTRY iluEnlargeCanvas(ILuint Width, ILuint Height, ILuint Depth)
{
	ILubyte	*Data/*, Clear[4]*/;
	ILuint	x, y, z, OldBps, OldH, OldD, OldPlane, AddX, AddY;
	ILenum	Origin;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// Uh-oh, what about 0 dimensions?!
	if (Width < iluCurImage->Width || Height < iluCurImage->Height || Depth < iluCurImage->Depth) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iluCurImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		switch (iluPlacement)
		{
			case ILU_LOWER_LEFT:
				AddX = 0;
				AddY = 0;
				break;
			case ILU_LOWER_RIGHT:
				AddX = Width - iluCurImage->Width;
				AddY = 0;
				break;
			case ILU_UPPER_LEFT:
				AddX = 0;
				AddY = Height - iluCurImage->Height;
				break;
			case ILU_UPPER_RIGHT:
				AddX = Width - iluCurImage->Width;
				AddY = Height - iluCurImage->Height;
				break;
			case ILU_CENTER:
				AddX = (Width - iluCurImage->Width) >> 1;
				AddY = (Height - iluCurImage->Height) >> 1;
				break;
			default:
				ilSetError(ILU_INVALID_PARAM);
				return IL_FALSE;
		}
	}
	else {  // IL_ORIGIN_UPPER_LEFT
		switch (iluPlacement)
		{
			case ILU_LOWER_LEFT:
				AddX = 0;
				AddY = Height - iluCurImage->Height;
				break;
			case ILU_LOWER_RIGHT:
				AddX = Width - iluCurImage->Width;
				AddY = Height - iluCurImage->Height;
				break;
			case ILU_UPPER_LEFT:
				AddX = 0;
				AddY = 0;
				break;
			case ILU_UPPER_RIGHT:
				AddX = Width - iluCurImage->Width;
				AddY = 0;
				break;
			case ILU_CENTER:
				AddX = (Width - iluCurImage->Width) >> 1;
				AddY = (Height - iluCurImage->Height) >> 1;
				break;
			default:
				ilSetError(ILU_INVALID_PARAM);
				return IL_FALSE;
		}
	}

	AddX *= iluCurImage->Bpp;

	Data = (ILubyte*)ialloc(iluCurImage->SizeOfData);
	if (Data == NULL) {
		return IL_FALSE;
	}

	// Preserve old data.
	OldPlane = iluCurImage->SizeOfPlane;
	OldBps   = iluCurImage->Bps;
	OldH     = iluCurImage->Height;
	OldD     = iluCurImage->Depth;
	Origin   = iluCurImage->Origin;
	ilCopyPixels(0, 0, 0, iluCurImage->Width, iluCurImage->Height, OldD, iluCurImage->Format, iluCurImage->Type, Data);

	ilTexImage(Width, Height, Depth, iluCurImage->Bpp, iluCurImage->Format, iluCurImage->Type, NULL);
	iluCurImage->Origin = Origin;

	ilClearImage();
	/*ilGetClear(Clear);
	if (iluCurImage->Bpp == 1) {
		memset(iluCurImage->Data, Clear[3], iluCurImage->SizeOfData);
	}
	else {
		for (x = 0; x < iluCurImage->SizeOfData; x += iluCurImage->Bpp) {
			for (y = 0; y < iluCurImage->Bpp; y++) {
				iluCurImage->Data[y] = Clear[y];
			}
		}
	}*/

	for (z = 0; z < OldD; z++) {
		for (y = 0; y < OldH; y++) {
			for (x = 0; x < OldBps; x++) {
				iluCurImage->Data[z * iluCurImage->SizeOfPlane + (y + AddY) * iluCurImage->Bps + x + AddX] =
					Data[z * OldPlane + y * OldBps + x];
			}
		}
	}

	ifree(Data);

	return IL_TRUE;
}


//! Flips an image over its x axis
ILboolean ILAPIENTRY iluFlipImage()
{
	ILubyte *StartPtr, *EndPtr;
	ILuint x, y, d;
	ILubyte *Flipped;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Flipped = (ILubyte*)ialloc(iluCurImage->SizeOfData);
	if (Flipped == NULL) {
		return IL_FALSE;
	}

	for (d = 0; d < iluCurImage->Depth; d++) {
		StartPtr = Flipped + d * iluCurImage->SizeOfPlane;
		EndPtr = iluCurImage->Data + d * iluCurImage->SizeOfPlane + iluCurImage->SizeOfPlane;

		for (x = 0; x < iluCurImage->Height; x++) {
			EndPtr -= iluCurImage->Bps;
			for (y = 0; y < iluCurImage->Bps; y++) {
				StartPtr[y] = EndPtr[y];
			}
			StartPtr += iluCurImage->Bps;
		}
	}

	ifree(iluCurImage->Data);
	iluCurImage->Data = Flipped;

	return IL_TRUE;
}


//! Mirrors an image over its y axis
ILboolean ILAPIENTRY iluMirror()
{
	ILubyte		*Data, *DataPtr, *Temp;
	ILuint		y, d, PixLine;
	ILint		x, c;
	ILushort	*ShortPtr, *TempShort;
	ILuint		*IntPtr, *TempInt;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Data = (ILubyte*)ialloc(iluCurImage->SizeOfData);
	if (Data == NULL) {
		return IL_FALSE;
	}

	PixLine = iluCurImage->Bps / iluCurImage->Bpc;
	switch (iluCurImage->Bpc)
	{
		case 1:
			Temp = iluCurImage->Data;
			for (d = 0; d < iluCurImage->Depth; d++) {
				DataPtr = Data + d * iluCurImage->SizeOfPlane;
				for (y = 0; y < iluCurImage->Height; y++) {
					for (x = iluCurImage->Width - 1; x >= 0; x--) {
						for (c = 0; c < iluCurImage->Bpp; c++, Temp++) {
							DataPtr[y * PixLine + x * iluCurImage->Bpp + c] = *Temp;
						}
					}
				}
			}
			break;

		case 2:
			TempShort = (ILushort*)iluCurImage->Data;
			for (d = 0; d < iluCurImage->Depth; d++) {
				ShortPtr = (ILushort*)(Data + d * iluCurImage->SizeOfPlane);
				for (y = 0; y < iluCurImage->Height; y++) {
					for (x = iluCurImage->Width - 1; x >= 0; x--) {
						for (c = 0; c < iluCurImage->Bpp; c++, TempShort++) {
							ShortPtr[y * PixLine + x * iluCurImage->Bpp + c] = *TempShort;
						}
					}
				}
			}
			break;

		case 4:
			TempInt = (ILuint*)iluCurImage->Data;
			for (d = 0; d < iluCurImage->Depth; d++) {
				IntPtr = (ILuint*)(Data + d * iluCurImage->SizeOfPlane);
				for (y = 0; y < iluCurImage->Height; y++) {
					for (x = iluCurImage->Width - 1; x >= 0; x--) {
						for (c = 0; c < iluCurImage->Bpp; c++, TempInt++) {
							IntPtr[y * PixLine + x * iluCurImage->Bpp + c] = *TempInt;
						}
					}
				}
			}
			break;
	}

	ifree(iluCurImage->Data);
	iluCurImage->Data = Data;

	return IL_TRUE;
}


//! Inverts the alpha in the image
ILboolean ILAPIENTRY iluInvertAlpha()
{
	ILuint		i, *IntPtr, NumPix;
	ILubyte		*Data;
	ILushort	*ShortPtr;
	ILfloat		*FltPtr;
	ILdouble	*DblPtr;
	ILubyte		Bpp;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iluCurImage->Format != IL_RGBA &&
		iluCurImage->Format != IL_BGRA &&
		iluCurImage->Format != IL_LUMINANCE) {
			ilSetError(ILU_ILLEGAL_OPERATION);
			return IL_FALSE;
	}

	Data = iluCurImage->Data;
	Bpp = iluCurImage->Bpp;
	NumPix = iluCurImage->SizeOfData / iluCurImage->Bpc;

	switch (iluCurImage->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			if (iluCurImage->Format == IL_LUMINANCE) {
				for (i = 0; i < NumPix; i++, Data++) {
					*(Data) = ~*(Data);
				}
			}
			else {
				for (i = Bpp - 1; i < NumPix; i += Bpp, Data += Bpp) {
					*(Data) = ~*(Data);
				}
			}
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			ShortPtr = (ILushort*)Data;
			if (iluCurImage->Format == IL_LUMINANCE) {
				for (i = 0; i < NumPix; i++, ShortPtr++) {
					*(ShortPtr) = ~*(ShortPtr);
				}
			}
			else {
				Data += (Bpp - 1) * 2;
				for (i = Bpp - 1; i < NumPix; i += Bpp, ShortPtr += Bpp) {
					*(ShortPtr) = ~*(ShortPtr);
				}
			}
			break;

		case IL_INT:
		case IL_UNSIGNED_INT:
			IntPtr = (ILuint*)Data;
			if (iluCurImage->Format == IL_LUMINANCE) {
				for (i = 0; i < NumPix; i++, IntPtr++) {
					*(IntPtr) = ~*(IntPtr);
				}
			}
			else {
				Data += (Bpp - 1) * 4;
				for (i = Bpp - 1; i < NumPix; i += Bpp, IntPtr += Bpp) {
					*(IntPtr) = ~*(IntPtr);
				}
			}
			break;

		case IL_FLOAT:
			FltPtr = (ILfloat*)Data;
			if (iluCurImage->Format == IL_LUMINANCE) {
				for (i = 0; i < NumPix; i++, FltPtr++) {
					*(FltPtr) = 1.0f - *(FltPtr);
				}
			}
			else {
				Data += (Bpp - 1) * sizeof(ILfloat);
				for (i = Bpp - 1; i < NumPix; i += Bpp, FltPtr += Bpp) {
					*(FltPtr) = 1.0f - *(FltPtr);
				}
			}
			break;

		case IL_DOUBLE:
			DblPtr = (ILdouble*)Data;
			if (iluCurImage->Format == IL_LUMINANCE) {
				for (i = 0; i < NumPix; i++, DblPtr++) {
					*(DblPtr) = 1.0f - *(DblPtr);
				}
			}
			else {
				Data += (Bpp - 1) * sizeof(ILdouble);
				for (i = Bpp - 1; i < NumPix; i += Bpp, DblPtr += Bpp) {
					*(DblPtr) = 1.0f - *(DblPtr);
				}
			}
			break;
	}

	return IL_TRUE;
}


//! Inverts the colours in the image
ILboolean ILAPIENTRY iluNegative()
{
	ILuint		i, j, c, *IntPtr, NumPix, Bpp;
	ILubyte		*Data;
	ILushort	*ShortPtr;
	ILubyte		*RegionMask;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iluCurImage->Format == IL_COLOUR_INDEX) {
		if (!iluCurImage->Pal.Palette || !iluCurImage->Pal.PalSize || iluCurImage->Pal.PalType == IL_PAL_NONE) {
			ilSetError(ILU_ILLEGAL_OPERATION);
			return IL_FALSE;
		}
		Data = iluCurImage->Pal.Palette;
		i = iluCurImage->Pal.PalSize;
	}
	else {
		Data = iluCurImage->Data;
		i = iluCurImage->SizeOfData;
	}

	RegionMask = iScanFill();
	
	// @TODO:  Optimize this some.

	NumPix = i / iluCurImage->Bpc;
	Bpp = iluCurImage->Bpp;

	if (RegionMask) {
		switch (iluCurImage->Bpc)
		{
			case 1:
				for (j = 0, i = 0; j < NumPix; j += Bpp, i++, Data += Bpp) {
					for (c = 0; c < Bpp; c++) {
						if (RegionMask[i])
							*(Data+c) = ~*(Data+c);
					}
				}
				break;

			case 2:
				ShortPtr = (ILushort*)Data;
				for (j = 0, i = 0; j < NumPix; j += Bpp, i++, ShortPtr += Bpp) {
					for (c = 0; c < Bpp; c++) {
						if (RegionMask[i])
							*(ShortPtr+c) = ~*(ShortPtr+c);
					}
				}
				break;

			case 4:
				IntPtr = (ILuint*)Data;
				for (j = 0, i = 0; j < NumPix; j += Bpp, i++, IntPtr += Bpp) {
					for (c = 0; c < Bpp; c++) {
						if (RegionMask[i])
							*(IntPtr+c) = ~*(IntPtr+c);
					}
				}
				break;
		}
	}
	else {
		switch (iluCurImage->Bpc)
		{
			case 1:
				for (j = 0; j < NumPix; j++, Data++) {
					*(Data) = ~*(Data);
				}
				break;

			case 2:
				ShortPtr = (ILushort*)Data;
				for (j = 0; j < NumPix; j++, ShortPtr++) {
					*(ShortPtr) = ~*(ShortPtr);
				}
				break;

			case 4:
				IntPtr = (ILuint*)Data;
				for (j = 0; j < NumPix; j++, IntPtr++) {
					*(IntPtr) = ~*(IntPtr);
				}
				break;
		}
	}

	ifree(RegionMask);

	return IL_TRUE;
}


// Taken from
//	http://www-classic.be.com/aboutbe/benewsletter/volume_III/Issue2.html#Insight
//	Hope they don't mind too much. =]
ILboolean ILAPIENTRY iluWave(ILfloat Angle)
{
	ILint	Delta;
	ILuint	y;
	ILubyte	*DataPtr, *TempBuff;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	TempBuff = (ILubyte*)ialloc(iluCurImage->SizeOfData);
	if (TempBuff == NULL) {
		return IL_FALSE;
	}

	for (y = 0; y < iluCurImage->Height; y++) {
		Delta = (ILint)
			(30 * sin((10 * Angle + y) * IL_DEGCONV) +
			 15 * sin(( 7 * Angle + 3 * y) * IL_DEGCONV));

		DataPtr = iluCurImage->Data + y * iluCurImage->Bps;

		if (Delta < 0) {
			Delta = -Delta;
			memcpy(TempBuff, DataPtr, iluCurImage->Bpp * Delta);
			memcpy(DataPtr, DataPtr + iluCurImage->Bpp * Delta, iluCurImage->Bpp * (iluCurImage->Width - Delta));
			memcpy(DataPtr + iluCurImage->Bpp * (iluCurImage->Width - Delta), TempBuff, iluCurImage->Bpp * Delta);
		}
		else if (Delta > 0) {
			memcpy(TempBuff, DataPtr, iluCurImage->Bpp * (iluCurImage->Width - Delta));
			memcpy(DataPtr, DataPtr + iluCurImage->Bpp * (iluCurImage->Width - Delta), iluCurImage->Bpp * Delta);
			memcpy(DataPtr + iluCurImage->Bpp * Delta, TempBuff, iluCurImage->Bpp * (iluCurImage->Width - Delta));
		}
	}

	ifree(TempBuff);

	return IL_TRUE;
}


// Swaps the colour order of the current image (rgb(a)->bgr(a) or vice-versa).
//	Must be either an 8, 24 or 32-bit (coloured) image (or palette).
ILboolean ILAPIENTRY iluSwapColours()
{
	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iluCurImage->Bpp == 1) {
		if (ilGetBppPal(iluCurImage->Pal.PalType) == 0 || iluCurImage->Format != IL_COLOUR_INDEX) {
			ilSetError(ILU_ILLEGAL_OPERATION);  // Can be luminance.
			return IL_FALSE;
		}

		switch (iluCurImage->Pal.PalType)
		{
			case IL_PAL_RGB24:
				return ilConvertPal(IL_PAL_BGR24);
			case IL_PAL_RGB32:
				return ilConvertPal(IL_PAL_BGR32);
			case IL_PAL_RGBA32:
				return ilConvertPal(IL_PAL_BGRA32);
			case IL_PAL_BGR24:
				return ilConvertPal(IL_PAL_RGB24);
			case IL_PAL_BGR32:
				return ilConvertPal(IL_PAL_RGB32);
			case IL_PAL_BGRA32:
				return ilConvertPal(IL_PAL_RGBA32);
			default:
				ilSetError(ILU_INTERNAL_ERROR);
				return IL_FALSE;
		}
	}

	switch (iluCurImage->Format)
	{
		case IL_RGB:
			return ilConvertImage(IL_BGR, iluCurImage->Type);
		case IL_RGBA:
			return ilConvertImage(IL_BGRA, iluCurImage->Type);
		case IL_BGR:
			return ilConvertImage(IL_RGB, iluCurImage->Type);
		case IL_BGRA:
			return ilConvertImage(IL_RGBA, iluCurImage->Type);
	}

	ilSetError(ILU_INTERNAL_ERROR);
	return IL_FALSE;
}


typedef struct BUCKET { ILubyte Colours[4];  struct BUCKET *Next; } BUCKET;

ILuint ILAPIENTRY iluColoursUsed()
{
	ILuint i, c, Bpp, ColVal, SizeData, BucketPos = 0, NumCols = 0;
	BUCKET Buckets[8192], *Temp;
	ILubyte ColTemp[4];
	ILboolean Matched;
	BUCKET *Heap[9];
	ILuint HeapPos = 0, HeapPtr = 0, HeapSize;

	imemclear(Buckets, sizeof(BUCKET) * 8192);
	for (c = 0; c < 9; c++) {
		Heap[c] = 0;
	}

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return 0;
	}

	Bpp = iluCurImage->Bpp;
	SizeData = iluCurImage->SizeOfData;

	// Create our miniature memory heap.
	// I have determined that the average number of colours versus
	//	the number of pixels is about a 1:8 ratio, so divide by 8.
	HeapSize = IL_MAX(1, iluCurImage->SizeOfData / iluCurImage->Bpp / 8);
	Heap[0] = (BUCKET*)ialloc(HeapSize * sizeof(BUCKET));
	if (Heap[0] == NULL)
		return IL_FALSE;

	for (i = 0; i < SizeData; i += Bpp) {
		*(ILuint*)ColTemp = 0;
		/*for (c = 0; c < Bpp; c++) {
			ColTemp[c] = iluCurImage->Data[i + c];
		}*/
		ColTemp[0] = iluCurImage->Data[i];
		if (Bpp > 1) {
			ColTemp[1] = iluCurImage->Data[i + 1];
			ColTemp[2] = iluCurImage->Data[i + 2];
		}
		if (Bpp > 3)
			ColTemp[3] = iluCurImage->Data[i + 3];

		BucketPos = *(ILuint*)ColTemp % 8192;

		// Add to hash table
		if (Buckets[BucketPos].Next == NULL) {
			NumCols++;
			//Buckets[BucketPos].Next = (BUCKET*)ialloc(sizeof(BUCKET));
			Buckets[BucketPos].Next = Heap[HeapPos] + HeapPtr++;
			if (HeapPtr >= HeapSize) {
				Heap[++HeapPos] = (BUCKET*)ialloc(HeapSize * sizeof(BUCKET));
				if (Heap[HeapPos] == NULL)
					goto alloc_error;
				HeapPtr = 0;
			}
			*(ILuint*)Buckets[BucketPos].Next->Colours = *(ILuint*)ColTemp;
			Buckets[BucketPos].Next->Next = NULL;
		}
		else {
			Matched = IL_FALSE;
			Temp = Buckets[BucketPos].Next;

			ColVal = *(ILuint*)ColTemp;
			while (Temp->Next != NULL) {
				if (ColVal == *(ILuint*)Temp->Colours) {
					Matched = IL_TRUE;
					break;
				}
				Temp = Temp->Next;
			}
			if (!Matched) {
				if (ColVal != *(ILuint*)Temp->Colours) {  // Check against last entry
					NumCols++;
					Temp = Buckets[BucketPos].Next;
					//Buckets[BucketPos].Next = (BUCKET*)ialloc(sizeof(BUCKET));
					Buckets[BucketPos].Next = Heap[HeapPos] + HeapPtr++;
					if (HeapPtr >= HeapSize) {
						Heap[++HeapPos] = (BUCKET*)ialloc(HeapSize * sizeof(BUCKET));
						if (Heap[HeapPos] == NULL)
							goto alloc_error;
						HeapPtr = 0;
					}
					Buckets[BucketPos].Next->Next = Temp;
					*(ILuint*)Buckets[BucketPos].Next->Colours = *(ILuint*)ColTemp;
				}
			}
		}
	}

	// Delete our mini heap.
	for (i = 0; i < 9; i++) {
		if (Heap[i] == NULL)
			break;
		ifree(Heap[i]);
	}

	return NumCols;

alloc_error:
	for (i = 0; i < 9; i++) {
		ifree(Heap[i]);
	}

	return 0;
}


ILboolean ILAPIENTRY iluCompareImage(ILuint Comp)
{
	ILimage		*Original;
	ILuint		OrigName, i;
	ILboolean	Same = IL_TRUE;

	Original = iluCurImage = ilGetCurImage();
	OrigName = ilGetCurName();

	// Same image, so return true.
	if (ilGetCurName() == Comp)
		return IL_TRUE;

	if (iluCurImage == NULL || ilIsImage(Comp) == IL_FALSE) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return 0;
	}

	ilBindImage(Comp);


	// @TODO:  Should we check palettes, too?
	if (Original->Bpp != iluCurImage->Bpp ||
		Original->Depth != iluCurImage->Bpp ||
		Original->Format != iluCurImage->Format ||
		Original->Height != iluCurImage->Height ||
		Original->Origin != iluCurImage->Origin ||
		Original->Type != iluCurImage->Type ||
		Original->Width != iluCurImage->Width) {
			ilBindImage(OrigName);
			return IL_FALSE;
	}

	for (i = 0; i < iluCurImage->SizeOfData; i++) {
		if (Original->Data[i] != iluCurImage->Data[i]) {
			Same = IL_FALSE;
			break;
		}
	}

	ilBindImage(OrigName);
	return Same;
}


// @TODO:  FIX ILGETCLEARCALL!
ILboolean ILAPIENTRY iluReplaceColour(ILubyte Red, ILubyte Green, ILubyte Blue, ILfloat Tolerance)
{
	ILubyte	ClearCol[4];
	ILint	TolVal, Distance, Dist1, Dist2, Dist3;
	ILuint	i, NumPix;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return 0;
	}

	ilGetClear(ClearCol, IL_RGBA, IL_UNSIGNED_BYTE);
	if (Tolerance > 1.0f || Tolerance < -1.0f)
		Tolerance = 1.0f;  // Clamp it.
	TolVal = (ILuint)(fabs(Tolerance) * UCHAR_MAX);  // To be changed.
	NumPix = iluCurImage->Width * iluCurImage->Height * iluCurImage->Depth;

	if (Tolerance <= FLT_EPSILON && Tolerance >= 0) {


	}
	else {
		switch (iluCurImage->Format)
		{
			case IL_RGB:
			case IL_RGBA:
				for (i = 0; i < iluCurImage->SizeOfData; i += iluCurImage->Bpp) {
					Dist1 = (ILint)iluCurImage->Data[i] - (ILint)ClearCol[0];
					Dist2 = (ILint)iluCurImage->Data[i+1] - (ILint)ClearCol[1];
					Dist3 = (ILint)iluCurImage->Data[i+2] - (ILint)ClearCol[2];
					Distance = (ILint)sqrt(Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3);
					if (Distance >= -TolVal && Distance <= TolVal) {
						iluCurImage->Data[i] = Red;
						iluCurImage->Data[i+1] = Green;
						iluCurImage->Data[i+2] = Blue;
					}
				}
				break;
			case IL_BGR:
			case IL_BGRA:
				for (i = 0; i < iluCurImage->SizeOfData; i += iluCurImage->Bpp) {
					Dist1 = (ILint)iluCurImage->Data[i] - (ILint)ClearCol[0];
					Dist2 = (ILint)iluCurImage->Data[i+1] - (ILint)ClearCol[1];
					Dist3 = (ILint)iluCurImage->Data[i+2] - (ILint)ClearCol[2];
					Distance = (ILint)sqrt(Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3);
					if (Distance >= -TolVal && Distance <= TolVal) {
						iluCurImage->Data[i+2] = Red;
						iluCurImage->Data[i+1] = Green;
						iluCurImage->Data[i] = Blue;
					}
				}
				break;
			//case IL_LUMINANCE:  // @TODO:  Finish!
			//case IL_COLOUR_INDEX:  // @TODO:  Finish!
		}
	}

	return IL_TRUE;
}


// Credit goes to Lionel Brits for this (refer to credits.txt)
ILboolean ILAPIENTRY iluEqualize()
{
	ILuint	Histogram[256]; // image Histogram
	ILuint	SumHistm[256]; // normalized Histogram and LUT
    ILuint	i = 0; // index variable
	ILuint	j = 0; // index variable
	ILuint	Sum=0;
	ILuint	NumPixels, Bpp;
	ILint	Intensity;
	ILfloat	Scale;
	ILint	IntensityNew;
	ILimage	*LumImage;
	ILuint	NewColour[4];

	ILubyte		*BytePtr;
	ILushort	*ShortPtr;
	ILuint		*IntPtr;

	iluCurImage = ilGetCurImage();
	if (iluCurImage == NULL) {
		ilSetError(ILU_ILLEGAL_OPERATION);
		return 0;
	}

	// @TODO:  Change to work with other types!
	if (iluCurImage->Bpc > 1) {
		ilSetError(ILU_INTERNAL_ERROR);
		return IL_FALSE;
	}

	if (iluCurImage->Format == IL_COLOUR_INDEX) {
		NumPixels = iluCurImage->Pal.PalSize / ilGetBppPal(iluCurImage->Pal.PalType);
		Bpp = ilGetBppPal(iluCurImage->Pal.PalType);
	}
	else {
		NumPixels = iluCurImage->Width * iluCurImage->Height * iluCurImage->Depth;
		Bpp = iluCurImage->Bpp;
	}

	// Clear the tables.
	imemclear(Histogram, 256 * sizeof(ILuint));
	imemclear(SumHistm, 256 * sizeof(ILuint));

	LumImage = iConvertImage(iluCurImage, IL_LUMINANCE, IL_UNSIGNED_BYTE);
	if (LumImage == NULL)
		return IL_FALSE;
	for (i = 0; i < NumPixels; i++) {
		Histogram[LumImage->Data[i]]++;
	}

	// Calculate normalized Sum of Histogram.
	for (i = 0; i < 256; i++) {
		for (j = 0; j < i; j++)
			Sum += Histogram[j];

		SumHistm[i] = (Sum << 8) / NumPixels;
		Sum = 0;
	}


	BytePtr = (iluCurImage->Format == IL_COLOUR_INDEX) ? iluCurImage->Pal.Palette : iluCurImage->Data;
	ShortPtr = (ILushort*)iluCurImage->Data;
	IntPtr = (ILuint*)iluCurImage->Data;

	// Transform image using new SumHistm as a LUT
    for (i = 0; i < NumPixels; i++) {
		Intensity = LumImage->Data[i];

		// Look up the normalized intensity
		IntensityNew = (ILint)SumHistm[Intensity];

		// Find out by how much the intensity has been Scaled
		Scale = (ILfloat)IntensityNew / (ILfloat)Intensity;

		switch (iluCurImage->Bpc)
		{
			case 1:
				// Calculate new pixel(s)
				NewColour[0] = (ILuint)(BytePtr[i * iluCurImage->Bpp] * Scale);
				if (Bpp >= 3) {
					NewColour[1] = (ILuint)(BytePtr[i * iluCurImage->Bpp + 1] * Scale);
					NewColour[2] = (ILuint)(BytePtr[i * iluCurImage->Bpp + 2] * Scale);
				}

				// Clamp values
				if (NewColour[0] > UCHAR_MAX)
					NewColour[0] = UCHAR_MAX;
				if (Bpp >= 3) {
					if (NewColour[1] > UCHAR_MAX)
						NewColour[1] = UCHAR_MAX;
					if (NewColour[2] > UCHAR_MAX)
						NewColour[2] = UCHAR_MAX;
				}

				// Store pixel(s)
				BytePtr[i * iluCurImage->Bpp] = (ILubyte)NewColour[0];
				if (Bpp >= 3) {
					BytePtr[i * iluCurImage->Bpp + 1]	= (ILubyte)NewColour[1];
					BytePtr[i * iluCurImage->Bpp + 2]	= (ILubyte)NewColour[2];
				}
				break;

			/*case 2:
				// Calculate new pixel
				NewColour[0] = (ILuint)(ShortPtr[i * iluCurImage->Bpp] * Scale);
				NewColour[1] = (ILuint)(ShortPtr[i * iluCurImage->Bpp + 1] * Scale);
				NewColour[2] = (ILuint)(ShortPtr[i * iluCurImage->Bpp + 2] * Scale);

				// Clamp values
				if (NewColour[0] > USHRT_MAX)
					NewColour[0] = USHRT_MAX;
				if (NewColour[1] > USHRT_MAX)
					NewColour[1] = USHRT_MAX;
				if (NewColour[2] > USHRT_MAX)
					NewColour[2] = USHRT_MAX;

				// Store pixel
				ShortPtr[i * iluCurImage->Bpp]		= (ILushort)NewColour[0];
				ShortPtr[i * iluCurImage->Bpp + 1]	= (ILushort)NewColour[1];
				ShortPtr[i * iluCurImage->Bpp + 2]	= (ILushort)NewColour[2];
				break;

			case 4:
				// Calculate new pixel
				NewColour[0] = (ILuint)(IntPtr[i * iluCurImage->Bpp] * Scale);
				NewColour[1] = (ILuint)(IntPtr[i * iluCurImage->Bpp + 1] * Scale);
				NewColour[2] = (ILuint)(IntPtr[i * iluCurImage->Bpp + 2] * Scale);

				// Clamp values
				if (NewColour[0] > UINT_MAX)
					NewColour[0] = UINT_MAX;
				if (NewColour[1] > UINT_MAX)
					NewColour[1] = UINT_MAX;
				if (NewColour[2] > UINT_MAX)
					NewColour[2] = UINT_MAX;

				// Store pixel
				IntPtr[i * 4 * iluCurImage->Bpp]		= NewColour[0];
				IntPtr[i * 4 * iluCurImage->Bpp + 1]	= NewColour[1];
				IntPtr[i * 4 * iluCurImage->Bpp + 2]	= NewColour[2];
				break;*/
		}
	}

	ilCloseImage(LumImage);

	return IL_TRUE;
}
