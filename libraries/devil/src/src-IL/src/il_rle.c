//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 08/26/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_rle.c
//
// Description: Functions for run-length encoding
//
//-----------------------------------------------------------------------------

// RLE code from TrueVision's TGA sample code available as Tgautils.zip at
//	ftp://ftp.truevision.com/pub/TGA.File.Format.Spec/PC.Version


#include "il_internal.h"
#include "il_rle.h"


ILuint GetPix(ILubyte *p, ILuint bpp)
{
	ILuint Pixel;

	Pixel = (ILuint)*p++;
	while (bpp-- > 1) {
		Pixel <<= 8;
		Pixel |= (ILuint)*p++;
	}
	return Pixel;
}


ILint CountDiffPixels(ILubyte *p, ILuint bpp, ILuint pixCnt)
{
	ILuint	pixel;
	ILuint	nextPixel = 0;
	ILint	n;

	n = 0;
	if (pixCnt == 1)
		return pixCnt;
	pixel = GetPix(p, bpp);

	while (pixCnt > 1) {
		p += bpp;
		nextPixel = GetPix(p, bpp);
		if (nextPixel == pixel)
			break;
		pixel = nextPixel;
		++n;
		--pixCnt;
	}

	if (nextPixel == pixel)
		return n;
	return n + 1;
}


ILint CountSamePixels(ILubyte *p, ILuint bpp, ILuint pixCnt)
{
	ILuint	pixel;
	ILuint	nextPixel;
	ILint	n;

	n = 1;
	pixel = GetPix(p, bpp);
	pixCnt--;

	while (pixCnt > 0) {
		p += bpp;
		nextPixel = GetPix(p, bpp);
		if (nextPixel != pixel)
			break;
		++n;
		--pixCnt;
	}

	return n;
}


ILboolean ilRleCompressLine(ILubyte *p, ILuint n, ILubyte bpp, ILubyte *q, ILuint *DestWidth, ILenum CompressMode)
{
	ILint	DiffCount;		// pixel count until two identical
	ILint	SameCount;		// number of identical adjacent pixels
	ILint	RLEBufSize;		// count of number of bytes encoded
	ILint	MaxRun = SGI_MAX_RUN;

	switch (CompressMode)
	{
		case IL_TGACOMP:
			MaxRun = TGA_MAX_RUN;
			break;
		case IL_SGICOMP:
			MaxRun = SGI_MAX_RUN;
			break;
	}

	RLEBufSize = 0;

	while (n > 0) {
		DiffCount = CountDiffPixels(p, bpp, n);
		SameCount = CountSamePixels(p, bpp, n);
		if (DiffCount > MaxRun)
			DiffCount = MaxRun;
		if (SameCount > MaxRun)
			SameCount = MaxRun;

		if (DiffCount > 0) {
			// create a raw packet
			if (CompressMode == IL_TGACOMP)
				*q++ = (ILbyte)(DiffCount - 1);
			else
				*q++ = (ILbyte)(DiffCount | 0x80);
			n -= DiffCount;
			RLEBufSize += (DiffCount * bpp) + 1;

			while (DiffCount > 0) {
				*q++ = *p++;
				if (bpp > 1) *q++ = *p++;
				if (bpp > 2) *q++ = *p++;
				if (bpp > 3) *q++ = *p++;
				DiffCount--;
			}
		}

		if (SameCount > 1) {
			// create a RLE packet
			if (CompressMode == IL_TGACOMP)
				*q++ = (ILbyte)((SameCount - 1) | 0x80);
			else
				*q++ = (ILbyte)(SameCount);
			n -= SameCount;
			RLEBufSize += bpp + 1;
			p += (SameCount - 1) * bpp;
			*q++ = *p++;
			if (bpp > 1) *q++ = *p++;
			if (bpp > 2) *q++ = *p++;
			if (bpp > 3) *q++ = *p++;
		}
	}

	if (CompressMode == IL_SGICOMP)
		*q++ = 0;

	*DestWidth = RLEBufSize;

	return IL_TRUE;
}


// Compresses an entire image using run-length encoding
ILuint ilRleCompress(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte *Dest, ILenum CompressMode, ILuint *ScanTable)
{
	ILuint DestW = 0, i, j, LineLen, Bps = Width * Bpp, SizeOfPlane = Width * Height * Bpp;

	for (j = 0; j < Depth; j++) {
		for (i = 0; i < Height; i++) {
			if (ScanTable)
				*ScanTable++ = DestW;

			ilRleCompressLine(Data + j * SizeOfPlane + i * Bps, Width, Bpp, Dest + DestW, &LineLen, CompressMode);
			//ilRleCompressLine(Data + i * Bps, Width, Bpp, Dest + DestW, &LineLen, CompressMode);
			
			DestW += LineLen;
		}
	}

	return DestW;
}


// Compresses a scanline using run-length encoding
/*ILboolean ilRleCompressLine(ILubyte *ScanLine, ILuint Width, ILubyte Bpp, ILubyte *Dest, ILuint *DestWidth, ILenum CompressMode)
{
	ILuint		MaxRun;  // Temporary
	ILuint		Count = 0, RunLen, DestPos = 0, i, c;
	ILboolean	BreakOut;

	switch (CompressMode)
	{
		case IL_TGACOMP:
			MaxRun = TGA_MAX_RUN;
			break;
		case IL_SGICOMP:
			MaxRun = SGI_MAX_RUN;
			break;
		default:
			ilSetError(IL_INVALID_PARAM);
			break;
	}

	// Should we check if Bpp is too high?
	if (ScanLine == NULL || Width == 0 || Bpp == 0 || Dest == NULL || DestWidth == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	while (Count < Width) {
		RunLen = 0;

		if (Count == Width - 1) {
			Dest[DestPos++] = 0;
			for (c = 0; c < Bpp; c++) {
				Dest[DestPos++] = ScanLine[(Width - 1) * Bpp + c];
			}
			break;
		}

		while (Count + RunLen < Width && RunLen < MaxRun) {
			if (Count + RunLen == Width - 1) {
				RunLen++;
				break;
			}

			BreakOut = IL_FALSE;
			for (c = 0; c < Bpp; c++) {
				if (ScanLine[(Count+RunLen) * Bpp + c]
					!= ScanLine[(Count+RunLen+1) * Bpp + c]) {
					if (!BreakOut)
						RunLen++;
					BreakOut = IL_TRUE;
				}
			}
			if (BreakOut)
				break;

			RunLen++;
		}

		if (RunLen > 1) {
			if (CompressMode == IL_TGACOMP)
				Dest[DestPos++] = 128 + RunLen - 1;
			else if (CompressMode == IL_SGICOMP)
				Dest[DestPos++] = 128 + RunLen;

			for (c = 0; c < Bpp; c++) {
				Dest[DestPos++] = ScanLine[Count * Bpp + c];
			}
			Count += RunLen;
		}

		else {
			RunLen = 1;
			while (Count + RunLen < Width && RunLen < MaxRun) {
				if (Count + RunLen == Width - 1) {
					RunLen++;
					break;
				}

				BreakOut = IL_FALSE;
				for (c = 0; c < Bpp; c++) {
					if (ScanLine[(Count+RunLen) * Bpp + c]
						== ScanLine[(Count+RunLen+1) * Bpp + c]) {
							BreakOut = IL_TRUE;
					}
				}
				if (BreakOut)
					break;

				RunLen++;
			}

			if (CompressMode == IL_TGACOMP)
				Dest[DestPos++] = RunLen - 1;
			else if (CompressMode == IL_SGICOMP)
				Dest[DestPos++] = RunLen;

			for (i = 0; i < RunLen; i++) {
				for (c = 0; c < Bpp; c++) {
					Dest[DestPos++] = ScanLine[(Count + i) * Bpp + c];
				}
			}
			Count += RunLen;
		}
	}

	// Sgi scanlines end with a 0.
	if (CompressMode == IL_SGICOMP)
		Dest[DestPos++] = 0;

	*DestWidth = DestPos;

	return IL_TRUE;
}*/
