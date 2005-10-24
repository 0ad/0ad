//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 06/20/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_dds-save.c
//
// Description: Saves a DirectDraw Surface (.dds) file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_dds.h"
#include "il_manip.h"
#include <limits.h>
#include <assert.h>

#ifndef IL_NO_DDS


#define DXTC_DEBUG
//#define FAST_COMPRESS

#ifdef DXTC_DEBUG
void debug_out(const wchar_t* fmt, ...)
{
	wchar_t buf[512];
	va_list ap;
	buf[511] = '\0';
	va_start(ap, fmt);
	_vsnwprintf(buf, 512-1, fmt, ap);
	va_end(ap);
	OutputDebugString(buf);
}
#endif


//! Writes a Dds file
ILboolean ilSaveDds(const ILstring FileName)
{
	ILHANDLE	DdsFile;
	ILboolean	bDds = IL_FALSE;

	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	DdsFile = iopenw(FileName);
	if (DdsFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bDds;
	}

	bDds = ilSaveDdsF(DdsFile);
	iclosew(DdsFile);

	return bDds;
}


//! Writes a Dds to an already-opened file
ILboolean ilSaveDdsF(ILHANDLE File)
{
	iSetOutputFile(File);
	return iSaveDdsInternal();
}


//! Writes a Dds to a memory "lump"
ILboolean ilSaveDdsL(ILvoid *Lump, ILuint Size)
{
	iSetOutputLump(Lump, Size);
	return iSaveDdsInternal();
}


// Internal function used to save the Dds.
ILboolean iSaveDdsInternal()
{
	ILenum	DXTCFormat;
	ILuint	counter, numMipMaps;
	ILubyte	*CurData = NULL;

	if (ilNextPower2(iCurImage->Width) != iCurImage->Width ||
		ilNextPower2(iCurImage->Height) != iCurImage->Height ||
		ilNextPower2(iCurImage->Depth) != iCurImage->Depth) {
			ilSetError(IL_BAD_DIMENSIONS);
			return IL_FALSE;
	}

	DXTCFormat = iGetInt(IL_DXTC_FORMAT);
	WriteHeader(iCurImage, DXTCFormat);
	
	numMipMaps = ilGetInteger(IL_NUM_MIPMAPS);
	for (counter = 0; counter <= numMipMaps; counter++) {
		ilActiveMipmap(counter);

		if (iCurImage->Origin != IL_ORIGIN_UPPER_LEFT) {
			CurData = iCurImage->Data;
			iCurImage->Data = iGetFlipped(iCurImage);
			if (iCurImage->Data == NULL) {
				iCurImage->Data = CurData;
				return IL_FALSE;
			}
		}

		if (!Compress(iCurImage, DXTCFormat))
			return IL_FALSE;

		if (iCurImage->Origin != IL_ORIGIN_UPPER_LEFT) {
			ifree(iCurImage->Data);
			iCurImage->Data = CurData;
		}
		
		ilActiveMipmap(0);
	}

	return IL_TRUE;
}


// @TODO:  Finish this, as it is incomplete.
ILboolean WriteHeader(ILimage *Image, ILenum DXTCFormat)
{
	ILuint i, FourCC, Flags1 = 0, Flags2 = 0, ddsCaps1 = 0, LinearSize;

	Flags1 |= DDS_LINEARSIZE
			| DDS_WIDTH | DDS_HEIGHT | DDS_CAPS | DDS_PIXELFORMAT;
	Flags2 |= DDS_FOURCC;

	if (ilGetInteger(IL_NUM_MIPMAPS) > 0)
		Flags1 |= DDS_MIPMAPCOUNT;


	// @TODO:  Fix the pre-multiplied alpha problem.
	if (DXTCFormat == IL_DXT2)
		DXTCFormat = IL_DXT3;
	else if (DXTCFormat == IL_DXT4)
		DXTCFormat = IL_DXT5;

	switch (DXTCFormat)
	{
		case IL_DXT1:
			FourCC = IL_MAKEFOURCC('D','X','T','1');
			break;
		case IL_DXT2:
			FourCC = IL_MAKEFOURCC('D','X','T','2');
			break;
		case IL_DXT3:
			FourCC = IL_MAKEFOURCC('D','X','T','3');
			break;
		case IL_DXT4:
			FourCC = IL_MAKEFOURCC('D','X','T','4');
			break;
		case IL_DXT5:
			FourCC = IL_MAKEFOURCC('D','X','T','5');
			break;
		default:
			// Error!
			ilSetError(IL_INTERNAL_ERROR);  // Should never happen, though.
			return IL_FALSE;
	}

	iwrite("DDS ", 1, 4);
	SaveLittleUInt(124);		// Size1
	SaveLittleUInt(Flags1);		// Flags1
	SaveLittleUInt(Image->Height);
	SaveLittleUInt(Image->Width);

	if (DXTCFormat == IL_DXT1) {
		LinearSize = Image->Width * Image->Height / 16 * 8;
	}
	else {
		LinearSize = Image->Width * Image->Height / 16 * 16;
	}
	SaveLittleUInt(LinearSize);	// LinearSize
	SaveLittleUInt(0);			// Depth

	if (ilGetInteger(IL_NUM_MIPMAPS) > 0)	// MipMapCount
		SaveLittleUInt(ilGetInteger(IL_NUM_MIPMAPS) + 1);
	else
		SaveLittleUInt(0);

	SaveLittleUInt(0);			// AlphaBitDepth

	for (i = 0; i < 10; i++)
		SaveLittleUInt(0);		// Not used

	SaveLittleUInt(32);			// Size2
	SaveLittleUInt(Flags2);		// Flags2
	SaveLittleUInt(FourCC);		// FourCC
	SaveLittleUInt(0);			// RGBBitCount
	SaveLittleUInt(0);			// RBitMask
	SaveLittleUInt(0);			// GBitMask
	SaveLittleUInt(0);			// BBitMask
	SaveLittleUInt(0);			// RGBAlphaBitMask
	ddsCaps1 |= DDS_TEXTURE;
	//changed 20040516: set mipmap flag on mipmap images
	//(cubemaps and non-compressed .dds files still not supported,
	//though)
	if (ilGetInteger(IL_NUM_MIPMAPS) > 0)
		ddsCaps1 |= DDS_MIPMAP | DDS_COMPLEX;
	SaveLittleUInt(ddsCaps1);	// ddsCaps1
	SaveLittleUInt(0);			// ddsCaps2
	SaveLittleUInt(0);			// ddsCaps3
	SaveLittleUInt(0);			// ddsCaps4
	SaveLittleUInt(0);			// TextureStage

	return IL_TRUE;
}

#endif//IL_NO_DDS


ILuint ILAPIENTRY ilGetDXTCData(ILvoid *Buffer, ILuint BufferSize, ILenum DXTCFormat)
{
	ILubyte	*CurData = NULL;
	ILuint	retVal;

	if (Buffer == NULL) {  // Return the number that will be written with a subsequent call.
		if (ilNextPower2(iCurImage->Width) != iCurImage->Width ||
			ilNextPower2(iCurImage->Height) != iCurImage->Height ||
			ilNextPower2(iCurImage->Depth) != iCurImage->Depth) {
				return 0;
		}

		switch (DXTCFormat)
		{
			case IL_DXT1:
				return iCurImage->Width * iCurImage->Height / 16 * 8;
			case IL_DXT3:
			case IL_DXT5:
				return iCurImage->Width * iCurImage->Height / 16 * 16;
			default:
				ilSetError(IL_FORMAT_NOT_SUPPORTED);
				return 0;
		}
	}

	if (DXTCFormat == iCurImage->DxtcFormat && iCurImage->DxtcSize && iCurImage->DxtcData) {
		memcpy(Buffer, iCurImage->DxtcData, IL_MIN(BufferSize, iCurImage->DxtcSize));
		return IL_MIN(BufferSize, iCurImage->DxtcSize);
	}

	if (iCurImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		CurData = iCurImage->Data;
		iCurImage->Data = iGetFlipped(iCurImage);
		if (iCurImage->Data == NULL) {
			iCurImage->Data = CurData;
			return 0;
		}
		ifree(CurData);
	}

	iSetOutputLump(Buffer, BufferSize);
	retVal = Compress(iCurImage, DXTCFormat);

	if (iCurImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		ifree(iCurImage->Data);
		iCurImage->Data = CurData;
	}

	return retVal;
}



ILubyte *CompressTo888(ILimage *Image)
{
	ILimage		*TempImage;
	ILubyte		*Data;

	if ((Image->Type != IL_UNSIGNED_BYTE && Image->Type != IL_BYTE) || Image->Format != IL_RGB) {

		TempImage = iConvertImage(iCurImage, IL_RGB, IL_UNSIGNED_BYTE);
		if (TempImage == NULL)
			return NULL;
	}
	else {
		TempImage = Image;
	}

	Data = (ILubyte*)ialloc(iCurImage->Width * iCurImage->Height * 3);
	if (Data == NULL) {
		if (TempImage != Image)
			ilCloseImage(TempImage);
		return NULL;
	}

	memcpy(Data, TempImage->Data, iCurImage->Width * iCurImage->Height * 3);

	if (TempImage != Image)
		ilCloseImage(TempImage);

	return Data;
}

#ifdef DXTC_DEBUG
ILuint g_x, g_y, g_score;
ILuint CalculateAlphaDist(ILubyte Alpha0, ILubyte Alpha1, ILuint Num, ILubyte *Block, ILuint DistLimit);
#endif

ILuint Compress(ILimage *Image, ILenum DXTCFormat)
{
	ILubyte		*Data;
	ILubyte		Block[16*3];
	ILushort	ex0, ex1;
	ILuint		x, y, i, BitMask;//, Rms1, Rms2;
	ILubyte		*Alpha, AlphaBlock[16], AlphaBitMask[6], AlphaOut[16], a0, a1;
	ILboolean	HasAlpha;
	ILuint		Count = 0;
	ILuint		FinalScore = 0, FinalScoreA = 0; // mainly used when attempting to improve the algorithm, to measure how good it is

	if (ilNextPower2(iCurImage->Width) != iCurImage->Width ||
		ilNextPower2(iCurImage->Height) != iCurImage->Height ||
		ilNextPower2(iCurImage->Depth) != iCurImage->Depth) {
			ilSetError(IL_BAD_DIMENSIONS);
			return 0;
	}

	Data = CompressTo888(Image);
	if (Data == NULL)
		return 0;

	Alpha = ilGetAlpha(IL_UNSIGNED_BYTE);
	if (Alpha == NULL) {
		ifree(Data);
		return 0;
	}

	switch (DXTCFormat)
	{
		case IL_DXT1:
			for (y = 0; y < Image->Height; y += 4) {
				for (x = 0; x < Image->Width; x += 4) {
#ifdef DXTC_DEBUG
					g_x=x;g_y=y;
#endif
					GetAlphaBlock(AlphaBlock, Alpha, Image, x, y);
					HasAlpha = IL_FALSE;
					for (i = 0 ; i < 16; i++) {
						if (AlphaBlock[i] < 128) {
							HasAlpha = IL_TRUE;
							break;
						}
					}

					GetBlock(Block, Data, Image, x, y);
					if (HasAlpha) {
						FinalScore += ChooseEndpoints(&ex0, &ex1, 3, Block, AlphaBlock, UINT_MAX);
						CorrectEndDXT1(&ex0, &ex1, IL_TRUE);
						BitMask = GenBitMask(ex0, ex1, 3, Block, AlphaBlock, NULL);
					}
					else {
						// When compressing DXT1 with no alpha, we are able
						// to use a 3 colour palette rather than 4, which might
						// give better results. Try both, and see which gives
						// the best answer.
						ILuint ScoreA = 0, ScoreB = 0;

						ScoreB = ChooseEndpoints(&ex0, &ex1, 4, Block, NULL, UINT_MAX);
						if (ScoreB != 0)
							ScoreA = ChooseEndpoints(&ex0, &ex1, 3, Block, NULL, ScoreB);

						if (ScoreA < ScoreB) {
							// Use the three-colour version
							CorrectEndDXT1(&ex0, &ex1, IL_TRUE);
							BitMask = GenBitMask(ex0, ex1, 3, Block, NULL, NULL);
							FinalScore += ScoreA;
						} else {
							// Use the four-colour version
							CorrectEndDXT1(&ex0, &ex1, IL_FALSE);
							BitMask = GenBitMask(ex0, ex1, 4, Block, NULL, NULL);
							FinalScore += ScoreB;
						}

/*
#ifdef DXTC_DEBUG
						if (ScoreA&&ScoreB) {
							ILuint x,y;
							Color888 ClosestC0, ClosestC1;
							ShortToColor888(ex0, &ClosestC0);
							ShortToColor888(ex1, &ClosestC1);
							debug_out(L"END (%d,%d): dist=%d, %02x%02x%02x %02x%02x%02x\n", g_x, g_y,
								min(ScoreA, ScoreB), 
								ClosestC0.r, ClosestC0.g, ClosestC0.b,
								ClosestC1.r, ClosestC1.g, ClosestC1.b);
							for (y=0; y<4; ++y) {
								for (x=0; x<4; ++x) {
									Color888 c;
									BytesToColor888(&Block[(x+y*4)*3], &c);
									debug_out(L" %02x%02x%02x", c.r, c.g, c.b);
								}
								debug_out(L"\n");
							}
						}
#endif
//*/

					}
					SaveLittleUShort(ex0);
					SaveLittleUShort(ex1);
					SaveLittleUInt(BitMask);
					Count += 8;
				}
			}
			break;

		/*case IL_DXT2:
			for (y = 0; y < Image->Height; y += 4) {
				for (x = 0; x < Image->Width; x += 4) {
					GetAlphaBlock(AlphaBlock, Alpha, Image, x, y);
					for (i = 0; i < 16; i += 2) {
						iputc((ILubyte)(((AlphaBlock[i] >> 4) << 4) | (AlphaBlock[i+1] >> 4)));
					}

					GetBlock(Block, Data, Image, x, y);
					PreMult(Block, AlphaBlock);
					ChooseEndpoints(Block, &ex0, &ex1);
					SaveLittleUShort(ex0);
					SaveLittleUShort(ex1);
					BitMask = GenBitMask(ex0, ex1, 4, Block, NULL, NULL);
					SaveLittleUInt(BitMask);
				}		
			}
			break;*/

		case IL_DXT3:
			for (y = 0; y < Image->Height; y += 4) {
				for (x = 0; x < Image->Width; x += 4) {
#ifdef DXTC_DEBUG
					g_x=x;g_y=y;
#endif
					GetAlphaBlock(AlphaBlock, Alpha, Image, x, y);
					for (i = 0; i < 16; i += 2) {
						iputc((ILubyte)(((AlphaBlock[i+1] >> 4) << 4) | (AlphaBlock[i] >> 4)));
					}

					GetBlock(Block, Data, Image, x, y);
					FinalScore += ChooseEndpoints(&ex0, &ex1, 4, Block, NULL, UINT_MAX);
					CorrectEndDXT1(&ex0, &ex1, IL_FALSE);
					SaveLittleUShort(ex0);
					SaveLittleUShort(ex1);
					BitMask = GenBitMask(ex0, ex1, 4, Block, NULL, NULL);
					SaveLittleUInt(BitMask);
					Count += 16;
				}
			}
			break;

		case IL_DXT5:
			for (y = 0; y < Image->Height; y += 4) {
				for (x = 0; x < Image->Width; x += 4) {
#ifdef DXTC_DEBUG
					g_x=x;g_y=y;
#endif
					GetAlphaBlock(AlphaBlock, Alpha, Image, x, y);
					ChooseAlphaEndpoints(AlphaBlock, &a0, &a1);
					iputc(a0);
					iputc(a1);
					if (a0 <= a1)
					{
						FinalScoreA += CalculateAlphaDist(a0, a1, 6, AlphaBlock, UINT_MAX);
						GenAlphaBitMask(a0, a1, 6, AlphaBlock, AlphaBitMask, AlphaOut);
					}
					else
					{
						FinalScoreA += CalculateAlphaDist(a0, a1, 8, AlphaBlock, UINT_MAX);
						GenAlphaBitMask(a0, a1, 8, AlphaBlock, AlphaBitMask, AlphaOut);
					}

					iwrite(AlphaBitMask, 1, 6);

					GetBlock(Block, Data, Image, x, y);
					FinalScore += ChooseEndpoints(&ex0, &ex1, 4, Block, NULL, UINT_MAX);
					CorrectEndDXT1(&ex0, &ex1, IL_FALSE);
					SaveLittleUShort(ex0);
					SaveLittleUShort(ex1);
					BitMask = GenBitMask(ex0, ex1, 4, Block, NULL, NULL);
					SaveLittleUInt(BitMask);
					Count += 16;
				}
			}
			break;
	}


	ifree(Alpha);
	ifree(Data);

#ifdef DXTC_DEBUG
	debug_out(L"FINAL SCORE: %d (%d)  (%dx%d)\n", FinalScore, FinalScoreA, Image->Width, Image->Height);
#endif

	return Count;
}


// Assumed to be 24-bit (8:8:8).
ILboolean GetBlock(ILubyte *Block, ILubyte *Data, ILimage *Image, ILuint XPos, ILuint YPos)
{
	ILuint x, y, i = 0, Offset;

	if (Image->Height < 4 || Image->Width < 4) {
		// Slower version, which handles small images correctly (where
		// 'correctly' is assumed to mean wrapping/tiling)
		for (y = 0; y < 4; y++) {
			for (x = 0; x < 4; x++) {
				Offset = ( ((YPos + y) % Image->Height) * Image->Width + ((XPos + x) % Image->Width) ) * 3;
				Block[i++] = Data[Offset++];
				Block[i++] = Data[Offset++];
				Block[i++] = Data[Offset++];
			}
		}
	} else {
		for (y = 0; y < 4; y++) {
			Offset = ((YPos + y) * Image->Width + XPos) * 3;
			for (x = 0; x < 4*3; x++)
				Block[i++] = Data[Offset++];
		}
	}

	return IL_TRUE;
}


ILboolean GetAlphaBlock(ILubyte *Block, ILubyte *Data, ILimage *Image, ILuint XPos, ILuint YPos)
{
	ILuint x, y, i = 0, Offset;

	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			// offset = y*w + x, but with % to avoid overflowing the image bounds
			// (primarily for small (2x2, 1x1) images which don't have a 4x4 block)
			Offset = ((YPos + y) % Image->Height) * Image->Width + ((XPos + x) % Image->Width);
			Block[i++] = Data[Offset];
		}
	}

	return IL_TRUE;
}


ILvoid ShortToColor565(ILushort Pixel, Color565 *Colour)
{
	// Unpack a 565 colour from a short
	Colour->nRed   = (Pixel & 0xF800) >> 11;
	Colour->nGreen = (Pixel & 0x07E0) >> 5;
	Colour->nBlue  = (Pixel & 0x001F);
	return;
}


ILvoid ShortToColor888(ILushort Pixel, Color888 *Colour)
{
	// Unpack a 565 short into an 888 colour.
	// See il_dds.c for the reason behind expanding this way
	Colour->r = ((Pixel & 0xF800) >> 8) | ((Pixel & 0xF800) >> 13);
	Colour->g = ((Pixel & 0x07E0) >> 3) | ((Pixel & 0x07E0) >> 9);
	Colour->b = ((Pixel & 0x001F) << 3) | ((Pixel & 0x001F) >> 2);
}


ILushort Color565ToShort(Color565 *Colour)
{
	// Pack a 565 colour into a short
	return (Colour->nRed << 11) | (Colour->nGreen << 5) | (Colour->nBlue);
}


ILushort Color888ToShort(Color888 *Colour)
{
	// Quantize an 888 colour to 565, then pack into a short
	return ((Colour->r >> 3) << 11) | ((Colour->g >> 2) << 5) | (Colour->b >> 3);
}

ILvoid BytesToColor888(ILubyte *Data, Color888 *Colour)
{
	*Colour = *(Color888*)Data;
}

ILvoid QuantizeColor888To565(Color888 *Colour)
{
	// Reproduce the effect of saving as 565 then reading as 888 - used
	// when calculating the effect of choosing a particular colour for the DXTC
	ShortToColor888(Color888ToShort(Colour), Colour);
}


ILuint GenBitMask(ILushort ex0, ILushort ex1, ILuint NumCols, ILubyte *In, ILubyte *Alpha, Color888 *OutCol)
{
	Color888 c0, c1;
	ShortToColor888(ex0, &c0);
	ShortToColor888(ex1, &c1);
	return ApplyBitMask(c0, c1, NumCols, In, Alpha, OutCol, NULL);
}

ILuint ApplyBitMask(Color888 Colour0, Color888 Colour1, ILuint NumCols, ILubyte *In, ILubyte *Alpha, Color888 *OutCol, ILuint *DistLimit)
{
	ILubyte		Mask[16];
	ILuint		i, TotalDist = 0, ClosestIndex, BitMask = 0;
	ILint		in_r, in_g, in_b;
	Color888	Colour2, Colour3;
	ILuint		ClosestDist, Dist;

	// Calculate intermediate colour(s). This assumes calculations are done
	// in full 888 precision, which isn't true on old hardware (e.g. NV20
	// (GeForce 3) uses 555 for DXT1), but apparently is on most other devices.
	if (NumCols == 3) {
		Colour2.r = (Colour0.r + Colour1.r) / 2;
		Colour2.g = (Colour0.g + Colour1.g) / 2;
		Colour2.b = (Colour0.b + Colour1.b) / 2;
		Colour3.r = 0;
		Colour3.g = 0;
		Colour3.b = 0;
	}
	else {  // NumCols == 4
		Colour2.r = (2 * Colour0.r + Colour1.r + 1) / 3;
		Colour2.g = (2 * Colour0.g + Colour1.g + 1) / 3;
		Colour2.b = (2 * Colour0.b + Colour1.b + 1) / 3;
		Colour3.r = (Colour0.r + 2 * Colour1.r + 1) / 3;
		Colour3.g = (Colour0.g + 2 * Colour1.g + 1) / 3;
		Colour3.b = (Colour0.b + 2 * Colour1.b + 1) / 3;
	}

	for (i = 0; i < 16; i++) {

		if (Alpha) {  // Test to see if we have 1-bit transparency
			if (Alpha[i] < 128) {
				Mask[i] = 3;  // Transparent
				if (OutCol) {
					OutCol[i] = Colour3;
				}
				continue;
			}
		}

		// If no transparency, try to find which colour is the closest.

		in_r = In[i*3+0];
		in_g = In[i*3+1];
		in_b = In[i*3+2];

		if (DistLimit) {

			#define TEST(n) \
				(in_r-Colour##n.r) * (in_r-Colour##n.r) \
			  + (in_g-Colour##n.g) * (in_g-Colour##n.g) \
			  + (in_b-Colour##n.b) * (in_b-Colour##n.b)
			if (NumCols == 4) {
				ClosestDist = TEST(0);
				Dist = TEST(1); if (Dist < ClosestDist) ClosestDist = Dist;
				Dist = TEST(2); if (Dist < ClosestDist) ClosestDist = Dist;
				Dist = TEST(3); if (Dist < ClosestDist) ClosestDist = Dist;
			} else {
				ClosestDist = TEST(0);
				Dist = TEST(1); if (Dist < ClosestDist) ClosestDist = Dist;
				Dist = TEST(2); if (Dist < ClosestDist) ClosestDist = Dist;
			}
			#undef TEST

			TotalDist += ClosestDist; // max is (255^2 * 3) * 16, which is < 2^30, so no chance of an integer overflow
			if (TotalDist >= *DistLimit)
				return UINT_MAX; // give up, since this is already worse than the previous best

		} else {

			#define TEST(n) \
				Dist = (in_r-Colour##n.r) * (in_r-Colour##n.r) \
				     + (in_g-Colour##n.g) * (in_g-Colour##n.g) \
				     + (in_b-Colour##n.b) * (in_b-Colour##n.b); \
				if (n == 0 || Dist < ClosestDist) { \
					ClosestDist = Dist; \
					ClosestIndex = n; \
				}
			if (NumCols == 4) {
				TEST(0)
				TEST(1)
				TEST(2)
				TEST(3)
			} else {
				TEST(0)
				TEST(1)
				TEST(2)
			}
			#undef TEST

			Mask[i] = ClosestIndex;
			if (OutCol) {
				switch (ClosestIndex) {
					case 0: OutCol[i] = Colour0; break;
					case 1: OutCol[i] = Colour1; break;
					case 2: OutCol[i] = Colour2; break;
					case 3: OutCol[i] = Colour3; break;
				}
			}
		}
	}

	if (DistLimit)
		return TotalDist;

	for (i = 0; i < 16; i++) {
		BitMask |= (Mask[i] << (i*2));
	}

	return BitMask;
}


ILvoid GenAlphaBitMask(ILubyte a0, ILubyte a1, ILuint Num, ILubyte *In, ILubyte *Mask, ILubyte *Out)
{
	ILubyte Alphas[8], M[16];
	ILuint	i, j, Closest, Dist;

	Alphas[0] = a0;
	Alphas[1] = a1;

	// 8-alpha or 6-alpha block?    
	if (Num == 8) {
		assert(Alphas[0] > Alphas[1]);
		// 8-alpha block:  derive the other six alphas.    
		// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		Alphas[2] = (6 * Alphas[0] + 1 * Alphas[1] + 3) / 7;	// bit code 010
		Alphas[3] = (5 * Alphas[0] + 2 * Alphas[1] + 3) / 7;	// bit code 011
		Alphas[4] = (4 * Alphas[0] + 3 * Alphas[1] + 3) / 7;	// bit code 100
		Alphas[5] = (3 * Alphas[0] + 4 * Alphas[1] + 3) / 7;	// bit code 101
		Alphas[6] = (2 * Alphas[0] + 5 * Alphas[1] + 3) / 7;	// bit code 110
		Alphas[7] = (1 * Alphas[0] + 6 * Alphas[1] + 3) / 7;	// bit code 111  
	}    
	else {  
		assert(Alphas[0] <= Alphas[1]);
		// 6-alpha block.    
		// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		Alphas[2] = (4 * Alphas[0] + 1 * Alphas[1] + 2) / 5;	// Bit code 010
		Alphas[3] = (3 * Alphas[0] + 2 * Alphas[1] + 2) / 5;	// Bit code 011
		Alphas[4] = (2 * Alphas[0] + 3 * Alphas[1] + 2) / 5;	// Bit code 100
		Alphas[5] = (1 * Alphas[0] + 4 * Alphas[1] + 2) / 5;	// Bit code 101
		Alphas[6] = 0x00;										// Bit code 110
		Alphas[7] = 0xFF;										// Bit code 111
	}

	for (i = 0; i < 16; i++) {
		Closest = UINT_MAX;
		for (j = 0; j < 8; j++) {
			Dist = abs((ILint)In[i] - (ILint)Alphas[j]);
			if (Dist < Closest) {
				Closest = Dist;
				M[i] = j;
			}
		}
	}

	if (Out) {
		for (i = 0; i < 16; i++) {
			Out[i] = Alphas[M[i]];
		}
	}

	// First three bytes.
	Mask[0] = (M[0]) | (M[1] << 3) | ((M[2] & 0x03) << 6);
	Mask[1] = ((M[2] & 0x04) >> 2) | (M[3] << 1) | (M[4] << 4) | ((M[5] & 0x01) << 7);
	Mask[2] = ((M[5] & 0x06) >> 1) | (M[6] << 2) | (M[7] << 5);

	// Second three bytes.
	Mask[3] = (M[8]) | (M[9] << 3) | ((M[10] & 0x03) << 6);
	Mask[4] = ((M[10] & 0x04) >> 2) | (M[11] << 1) | (M[12] << 4) | ((M[13] & 0x01) << 7);
	Mask[5] = ((M[13] & 0x06) >> 1) | (M[14] << 2) | (M[15] << 5);

	return;
}


ILuint RMSAlpha(ILubyte *Orig, ILubyte *Test)
{
	ILuint	RMS = 0, i;
	ILint	d;

	for (i = 0; i < 16; i++) {
		d = Orig[i] - Test[i];
		RMS += d*d;
	}

	//RMS /= 16;

	return RMS;
}


ILuint Distance(Color888 *c1, Color888 *c2)
{
	return (c1->r - c2->r) * (c1->r - c2->r)
		 + (c1->g - c2->g) * (c1->g - c2->g)
		 + (c1->b - c2->b) * (c1->b - c2->b);
}

ILboolean IsColourEqual(Color888 *c1, Color888 *c2)
{
	return (c1->r == c2->r && c1->g == c2->g && c1->b == c2->b);
}

ILuint ChooseEndpoints(ILushort *ex0, ILushort *ex1, ILuint NumCols, ILubyte *Block, ILubyte *Alpha, ILuint ClosestDist)
{
	ILuint		i, j;
	ILint		a,b,c, k;
	Color888	ClosestC0, ClosestC1, NewC0, NewC1; // always contain 565 quantized colours
	ILuint		dist;
	Color888	Col_i, Col_j;

	// Initialise ClosestC with the inputted closest values. (If there was
	// no previously known closest, these colours will be garbage, but that
	// doesn't matter because they'll get replaced soon.)
	ShortToColor888(*ex0, &ClosestC0);
	ShortToColor888(*ex1, &ClosestC1);

	// Wherever possible, this code aims to make a perfect choice. Recompressing
	// a DDS should therefore be entirely lossless.

	// First, try every combination of 2 colours from the image, because that
	// often results in a simple perfect match.

#define TRY(c0, c1) \
	dist = ApplyBitMask(c0, c1, NumCols, Block, Alpha, NULL, &ClosestDist); \
	if (dist < ClosestDist) { \
		ClosestC0 = c0; \
		ClosestC1 = c1; \
		ClosestDist = dist; \
	}


	for (i = 0; i < 16; i++) {

		BytesToColor888(&Block[i*3], &Col_i);
		QuantizeColor888To565(&Col_i);

		for (j = i+1; j < 16; j++) {

			BytesToColor888(&Block[j*3], &Col_j);
			QuantizeColor888To565(&Col_j);

			TRY(Col_i, Col_j)

			// If we've found a perfect match, stop now.
			if (ClosestDist == 0) {
				*ex0 = Color888ToShort(&ClosestC0);
				*ex1 = Color888ToShort(&ClosestC1);
				return ClosestDist;
			}
		}
	}

	{
		// If there are <= NumCols-1 unique colours, it's possible that there's
		// a perfect match where one of the endpoints isn't present in the block.
		// So, try to detect this:

		ILuint NumUniqueColours = 0;
		Color888 UniqueColours[4];

		for (i = 0; i < 16; ++i) {

			// If we have 1-bit alpha (i.e. DXT1a), ignore all transparent pixels,
			// since we don't care about what colour they are.
			if (Alpha && Alpha[i] < 128)
				continue;

			BytesToColor888(&Block[i*3], &Col_i);

			for (j = 0; j < NumUniqueColours; ++j)
				if (IsColourEqual(&Col_i, &UniqueColours[j]))
					break;

			if (j == NumUniqueColours) { // ran off end of loop, no match found, so this colour is new
				// If we've seen too many, give up
				if (NumUniqueColours >= NumCols)
				{
					NumUniqueColours = UINT_MAX; // i.e. too many to count, since I can only count up to 3 or 4
					break;
				}
				// Otherwise, add this to the list of 
				UniqueColours[NumUniqueColours++] = Col_i;
			}
		}

		if (NumUniqueColours < NumCols) {
			Color888 c[4];
			ILuint ir,jr, ig,jg, ib,jb;
			ILint UniqueColourMatches_r[4];
			ILint UniqueColourMatches_g[4];
			ILint UniqueColourMatches_b[4];

			// If there's a perfect match, there's also a perfect match
			// for each component of the colour. So, consider each component
			// one at a time (so there's a manageable number of combinations to
			// test), and then repeat for the next component when the earlier
			// ones look good.

			// This following code is... um... probably quite dangerously nested.
			// It looks like it could be very slow, with six nested loops and
			// four billion potential iterations; but it should never go around
			// that many times, so there's no need to worry. At best, it will
			// only do 1024 iterations, before deciding that it can't match red
			// and so there's no hope of matching the whole colour.


			// First, pick values of r; we then know which of {c0..c3} are
			// used by each unique colour. Then, for b/g components, we only need
			// to test those specific ones.
			// TEST1 creates a bitmask indicating which palette entries could
			// possibly be used for each colour. (If e.g. c0.r==c1.r, any are
			// allowed.)
#define GENBIT(n, comp) \
	 (((UniqueColours[n].comp == c[0].comp) ? 1 : 0) \
	| ((UniqueColours[n].comp == c[1].comp) ? 2 : 0) \
	| ((UniqueColours[n].comp == c[2].comp) ? 4 : 0) \
	| ((NumCols == 4 && UniqueColours[n].comp == c[3].comp) ? 8 : 0))

#define TEST1(nplus1, n, comp) \
	case nplus1: \
		UniqueColourMatches_##comp[n] = GENBIT(n, comp); \
		if (UniqueColourMatches_##comp[n] == 0) break;

#define TEST2(nplus1, n, comp, compprev) \
	case nplus1: \
		UniqueColourMatches_##comp[n] = UniqueColourMatches_##compprev[n] & GENBIT(n, comp); \
		if (UniqueColourMatches_##comp[n] == 0) break;

			// Maybe we've only got one unique value for the R component,
			// and it's also a valid 5-bit number, in which case it's safe
			// to assume that c[0] and c[1] will always have this value.
			// That means we can save a few hundred iterations, rather than
			// checking all possible values of c[1], since we know c[0] is always
			// going to be valid.
			ILboolean unique_r = IL_FALSE;
			switch (NumUniqueColours) {
				case 4: if (UniqueColours[3].r != UniqueColours[2].r) break;
				case 3: if (UniqueColours[2].r != UniqueColours[1].r) break;
				case 2: if (UniqueColours[1].r != UniqueColours[0].r) break;
				default: // All are the same. See if it's 5 bits.
					if ((UniqueColours[0].r & 7) == (UniqueColours[0].r >> 5))
						unique_r = IL_TRUE;
			}

			for (ir = 0; ir < 32; ++ir) {
				if (unique_r && ir == 0) c[0].r = UniqueColours[0].r; // first iteration: use the known unique value
				else if (unique_r) break; // subsequent iterations: abort
				else
				// Try every possible c0.r
				c[0].r = (ir << 3) | (ir >> 2);

				for (jr = 0; jr < 32; ++jr) {
					if (unique_r && jr == 0) c[1].r = UniqueColours[0].r; // first iteration: use the known unique value
					else if (unique_r) break; // subsequent iterations: abort
					else
					// Try every possible c1.r
					c[1].r = (jr << 3) | (jr >> 2);

					// Calculate the other colours
					if (NumCols == 3) {
						c[2].r = (c[0].r + c[1].r)/2;
					} else {
						c[2].r = (2*c[0].r + c[1].r + 1)/3;
						c[3].r = (2*c[1].r + c[0].r + 1)/3;
					}

					// Check whether these colours are any good
					switch (NumUniqueColours) {
						TEST1(4, 3, r); TEST1(3, 2, r); TEST1(2, 1, r); TEST1(1, 0, r);
						default: /* all succeeded, at least for this component */

						for (ib = 0; ib < 32; ++ib) {
							c[0].b = (ib << 3) | (ib >> 2);
							for (jb = 0; jb < 32; ++jb) {
								c[1].b = (jb << 3) | (jb >> 2);
								if (NumCols == 3) {
									c[2].b = (c[0].b + c[1].b)/2;
								} else {
									c[2].b = (2*c[0].b + c[1].b + 1)/3;
									c[3].b = (2*c[1].b + c[0].b + 1)/3;
								}
								switch (NumUniqueColours) {
									TEST2(4, 3, b,r); TEST2(3, 2, b,r); TEST2(2, 1, b,r); TEST2(1, 0, b,r);
									default: /* all succeeded, at least for this component */

									for (ig = 0; ig < 64; ++ig) {
										c[0].g = (ig << 2) | (ig >> 4);
										for (jg = 0; jg < 64; ++jg) {
											c[1].g = (jg << 2) | (jg >> 4);
											if (NumCols == 3) {
												c[2].g = (c[0].g + c[1].g)/2;
											} else {
												c[2].g = (2*c[0].g + c[1].g + 1)/3;
												c[3].g = (2*c[1].g + c[0].g + 1)/3;
											}
											switch (NumUniqueColours) {
												TEST2(4, 3, g,b); TEST2(3, 2, g,b); TEST2(2, 1, g,b); TEST2(1, 0, g,b);
												default:
												/* all components succeeded, hurrah! */
												NewC0 = c[0];
												NewC1 = c[1];

												TRY(NewC0, NewC1)

												// Unless I've been stupid, this
												// will always succeed, since it
												// won't get this far unless the
												// match was perfect.
												if (ClosestDist == 0) {
													*ex0 = Color888ToShort(&ClosestC0);
													*ex1 = Color888ToShort(&ClosestC1);
													return ClosestDist;
												} else {
													debug_out(L"Argh!");
												}
											}
										}
									}
								}
							}
						}
					}
				}
			} // whee...
		}
	}
#undef TEST1
#undef TEST2

	// Usually the match is not bad at this point, so the code could just
	// stop here if it was lazy. But it can be improved, so try some brute-force
	// (and fairly slow) methods of improving the match:

#ifndef FAST_COMPRESS

	// Do a similar blind-searching-over-lots-of-values thing to the code further
	// down, but adjusting each component individually - this allows each to be tested
	// over a greater range/resolution, since the number of tests is O(n) rather
	// than O(n^3) (where n = number of values tested for each component)

	NewC0 = ClosestC0;
	NewC1 = ClosestC1;

	// Repeat it lots, since it often gets better each time
	for (i = 0; i < 6; ++i) {

		// Start with wide searches, then get narrower to avoid wasting time
		ILint SearchLimit;
		switch (i) {
			case 0: SearchLimit = 32; break;
			case 1: SearchLimit = 16; break;
			case 2: SearchLimit = 8; break;
			case 3: SearchLimit = 4; break;
			case 4: SearchLimit = 4; break;
			default: SearchLimit = 2; break;
		}

		// Try greyscale first

		for (k = -SearchLimit; k <= SearchLimit; ++k) {
			Color888 TempC;
			ILuint dist;

			if (k == 0) continue;

			TempC = ClosestC0;
			TempC.r += k*8;
			TempC.g += k*8;
			TempC.b += k*8;

			dist = ApplyBitMask(TempC, ClosestC1, NumCols, Block, Alpha, NULL, &ClosestDist);
			if (dist < ClosestDist) {
				NewC0 = TempC;
				ClosestDist = dist;
			}

			TempC = ClosestC1;
			TempC.r += k*8;
			TempC.g += k*8;
			TempC.b += k*8;

			dist = ApplyBitMask(ClosestC0, TempC, NumCols, Block, Alpha, NULL, &ClosestDist);
			if (dist < ClosestDist) {
				NewC1 = TempC;
				ClosestDist = dist;
			}
		}

		ClosestC0 = NewC0;
		ClosestC1 = NewC1;

		// Next, try adjusting the red component

		for (k = -SearchLimit; k <= SearchLimit; ++k) {
			Color888 TempC;
			ILuint dist;

			if (k == 0) continue;

			TempC = ClosestC0;
			TempC.r += k*8;

			dist = ApplyBitMask(TempC, ClosestC1, NumCols, Block, Alpha, NULL, &ClosestDist);
			if (dist < ClosestDist) {
				NewC0 = TempC;
				ClosestDist = dist;
			}

			TempC = ClosestC1;
			TempC.r += k*8;

			dist = ApplyBitMask(ClosestC0, TempC, NumCols, Block, Alpha, NULL, &ClosestDist);
			if (dist < ClosestDist) {
				NewC1 = TempC;
				ClosestDist = dist;
			}
		}

		ClosestC0 = NewC0;
		ClosestC1 = NewC1;

		// Then try adjusting the green component (at twice the resolution of red and blue)

		for (k = -SearchLimit*2; k <= SearchLimit*2; ++k) {
			Color888 TempC;
			ILuint dist;

			if (k == 0) continue;

			TempC = ClosestC0;
			TempC.g += k*4;

			dist = ApplyBitMask(TempC, ClosestC1, NumCols, Block, Alpha, NULL, &ClosestDist);
			if (dist < ClosestDist) {
				NewC0 = TempC;
				ClosestDist = dist;
			}

			TempC = ClosestC1;
			TempC.g += k*4;

			dist = ApplyBitMask(ClosestC0, TempC, NumCols, Block, Alpha, NULL, &ClosestDist);
			if (dist < ClosestDist) {
				NewC1 = TempC;
				ClosestDist = dist;
			}
		}

		ClosestC0 = NewC0;
		ClosestC1 = NewC1;

		// Finally try adjusting the blue component

		for (k = -SearchLimit; k <= SearchLimit; ++k) {
			Color888 TempC;
			ILuint dist;

			if (k == 0) continue;

			TempC = ClosestC0;
			TempC.b += k*8;

			dist = ApplyBitMask(TempC, ClosestC1, NumCols, Block, Alpha, NULL, &ClosestDist);
			if (dist < ClosestDist) {
				NewC0 = TempC;
				ClosestDist = dist;
			}

			TempC = ClosestC1;
			TempC.b += k*8;

			dist = ApplyBitMask(ClosestC0, TempC, NumCols, Block, Alpha, NULL, &ClosestDist);
			if (dist < ClosestDist) {
				NewC1 = TempC;
				ClosestDist = dist;
			}
		}

		ClosestC0 = NewC0;
		ClosestC1 = NewC1;

		// (TODO: Remove some of this code duplication)

		if (ClosestDist == 0)
			break;
	}


	if (ClosestDist == 0) {
		*ex0 = Color888ToShort(&ClosestC0);
		*ex1 = Color888ToShort(&ClosestC1);
		return ClosestDist;
	}



	// Move one endpoint around every point in a cube to find a better
	// match. Then do the same for the second endpoint, and then repeat with
	// smaller higher-resolution cubes.

	for (i = 0; i < 8; ++i) {
		int range, step;
		Color888 TempC;

		switch (i) {
			// Weird ranges/steps, but experimental evidence suggests that
			// these values are reasonable.
			case 1: range = 2; step = 1; break;
			case 3: range = 4; step = 2; break;
			case 5: range = 8; step = 4; break;
			default: range = 1; step = 1; break;
		}

		for (a = -range; a <= range; a += step) {
			for (b = -range*2; b <= range*2; b += step) { // green has twice the resolution, so use twice the range
				for (c = -range; c <= range; c += step) {

					TempC = ClosestC0;
					TempC.r += a*8;
					TempC.g += b*4;
					TempC.b += c*8;

					dist = ApplyBitMask(TempC, ClosestC1, NumCols, Block, Alpha, NULL, &ClosestDist);
					if (dist < ClosestDist) {
						ClosestC0 = TempC;
						ClosestDist = dist;
					}

					TempC = ClosestC1;
					TempC.r += a*8;
					TempC.g += b*4;
					TempC.b += c*8;

					dist = ApplyBitMask(ClosestC0, TempC, NumCols, Block, Alpha, NULL, &ClosestDist);
					if (dist < ClosestDist) {
						ClosestC1 = TempC;
						ClosestDist = dist;
					}

				}
			}
		}
	}

#endif // FAST_COMPRESS

/*
#ifdef DXTC_DEBUG
	if (ClosestDist) {
		ILuint x,y;
		debug_out(L"END (%d,%d): dist=%d, c0=%02x%02x%02x, c1=%02x%02x%02x\n", g_x, g_y,
			ClosestDist, 
			ClosestC0.r, ClosestC0.g, ClosestC0.b,
			ClosestC1.r, ClosestC1.g, ClosestC1.b);
		for (y=0; y<4; ++y) {
			for (x=0; x<4; ++x) {
				Color888 c;
				BytesToColor888(&Block[(x+y*4)*3], &c);
				debug_out(L" %02x%02x%02x", c.r, c.g, c.b);
			}
			debug_out(L"\n");
		}
	}
#endif
//*/

	*ex0 = Color888ToShort(&ClosestC0);
	*ex1 = Color888ToShort(&ClosestC1);
	return ClosestDist;
}


ILuint CalculateAlphaDist(ILubyte Alpha0, ILubyte Alpha1, ILuint Num, ILubyte *Block, ILuint DistLimit)
{
	ILubyte	Alphas[8];
	ILuint	i, j, TotalDist = 0;
	ILuint	ClosestDist, Dist;

	Alphas[0] = Alpha0;
	Alphas[1] = Alpha1;

	// 8-alpha or 6-alpha block?    
	if (Num == 8) {    
		// 8-alpha block:  derive the other six alphas.    
		// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		Alphas[2] = (6 * Alphas[0] + 1 * Alphas[1] + 3) / 7;	// bit code 010
		Alphas[3] = (5 * Alphas[0] + 2 * Alphas[1] + 3) / 7;	// bit code 011
		Alphas[4] = (4 * Alphas[0] + 3 * Alphas[1] + 3) / 7;	// bit code 100
		Alphas[5] = (3 * Alphas[0] + 4 * Alphas[1] + 3) / 7;	// bit code 101
		Alphas[6] = (2 * Alphas[0] + 5 * Alphas[1] + 3) / 7;	// bit code 110
		Alphas[7] = (1 * Alphas[0] + 6 * Alphas[1] + 3) / 7;	// bit code 111  
	}    
	else {  
		// 6-alpha block.    
		// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		Alphas[2] = (4 * Alphas[0] + 1 * Alphas[1] + 2) / 5;	// Bit code 010
		Alphas[3] = (3 * Alphas[0] + 2 * Alphas[1] + 2) / 5;	// Bit code 011
		Alphas[4] = (2 * Alphas[0] + 3 * Alphas[1] + 2) / 5;	// Bit code 100
		Alphas[5] = (1 * Alphas[0] + 4 * Alphas[1] + 2) / 5;	// Bit code 101
		Alphas[6] = 0x00;										// Bit code 110
		Alphas[7] = 0xFF;										// Bit code 111
	}

	if (Num == 6)
	{
		for (i = 0; i < 16; ++i)
		{
			if (Block[i] == 0 || Block[i] == 255) // handle the common case of max/min regions
				ClosestDist = 0;
			else
			{
				ClosestDist = UINT_MAX;
				for (j = 0; j < 6; ++j)
				{
					Dist = (Block[i] - Alphas[j]) * (Block[i] - Alphas[j]);
					if (Dist < ClosestDist)
						ClosestDist = Dist;
				}
				TotalDist += ClosestDist;
				if (TotalDist >= DistLimit)
					return UINT_MAX;
			}
		}
	}
	else
	{
		for (i = 0; i < 16; ++i)
		{
			ClosestDist = UINT_MAX;
			for (j = 0; j < 8; ++j)
			{
				Dist = (Block[i] - Alphas[j]) * (Block[i] - Alphas[j]);
				if (Dist < ClosestDist)
					ClosestDist = Dist;
			}
			TotalDist += ClosestDist;
			if (TotalDist >= DistLimit)
				return UINT_MAX;
		}
	}
	return TotalDist;
}

void ChooseAlphaEndpoints(ILubyte *Block, ILubyte *Alpha0, ILubyte *Alpha1)
{
	ILint	a0, a1;
	ILuint	Dist, ClosestDist = UINT_MAX;


	// First try every point in a coarse grid, and remember the best.
	// (This usually gives a pretty decent result by itself.)
	for (a1 = 0; a1 < 256; a1 += 8)
	{
		for (a0 = 0; a0 < a1; a0 += 8)
		{
			// Try 6-value (plus 0 and 255) version
			Dist = CalculateAlphaDist(a0, a1, 6, Block, ClosestDist);
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				*Alpha0 = a0;
				*Alpha1 = a1;
				if (Dist == 0) return;
			}

			// Try 8-value version
			Dist = CalculateAlphaDist(a0, a1, 8, Block, ClosestDist);
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				*Alpha0 = a1;
				*Alpha1 = a0;
				if (Dist == 0) return;
			}
		}
	}

	// Attempt to fine-tune the result, by testing a small grid around the
	// previously determined 'best' values.
	{
		ILint oldAlpha0 = *Alpha0;
		ILint oldAlpha1 = *Alpha1;
		const ILint range = 8;
		for (a1 = oldAlpha1-range; a1 <= oldAlpha1+range; ++a1)
		{
			for (a0 = oldAlpha0-range; a0 <= oldAlpha0+range; ++a0)
			{
				ILuint num = (ILubyte)a0 <= (ILubyte)a1 ? 6 : 8;
				Dist = CalculateAlphaDist(a0, a1, num, Block, ClosestDist);
				if (Dist < ClosestDist)
				{
					ClosestDist = Dist;
					*Alpha0 = a0;
					*Alpha1 = a1;
					if (Dist == 0) return;
				}
			}
		}
	}


	// Or we could do the very slow but optimal [at least by the specific
	// 'goodness' criteria used by this code] method:
#if 0
	// First try with the 6-value system:
	for (a1 = 0; a1 < 256; ++a1)
	{
		for (a0 = 0; a0 < a1; ++a0)
		{
			Dist = CalculateAlphaDist(a0, a1, 6, Block, ClosestDist);
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				*Alpha0 = a0;
				*Alpha1 = a1;
				if (Dist == 0) return; // early finish, e.g. if image is only 0/255
			}
		}
	}

	// Then try with the 8-value system:
	for (a1 = 0; a1 < 256; ++a1)
	{
		for (a0 = 0; a0 < a1; ++a0)
		{
			Dist = CalculateAlphaDist(a1, a0, 8, Block, ClosestDist);
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				*Alpha0 = a1;
				*Alpha1 = a0;
				if (Dist == 0) return;
			}
		}
	}
#endif
}


ILvoid CorrectEndDXT1(ILushort *ex0, ILushort *ex1, ILboolean HasAlpha)
{
	ILushort Temp;

	if (HasAlpha) {
		if (*ex0 > *ex1) {
			Temp = *ex0;
			*ex0 = *ex1;
			*ex1 = Temp;
		}
	}
	else {
		if (*ex0 < *ex1) {
			Temp = *ex0;
			*ex0 = *ex1;
			*ex1 = Temp;
		}
	}

	return;
}


ILvoid PreMult(ILushort *Data, ILubyte *Alpha)
{
	Color888	Colour;
	ILuint		i;

	for (i = 0; i < 16; i++) {
		ShortToColor888(Data[i], &Colour);
		Colour.r = (ILubyte)(((ILuint)Colour.r * Alpha[i]) >> 8);
		Colour.g = (ILubyte)(((ILuint)Colour.g * Alpha[i]) >> 8);
		Colour.b = (ILubyte)(((ILuint)Colour.b * Alpha[i]) >> 8);

		/*Colour.r = (ILubyte)(Colour.r * (Alpha[i] / 255.0));
		Colour.g = (ILubyte)(Colour.g * (Alpha[i] / 255.0));
		Colour.b = (ILubyte)(Colour.b * (Alpha[i] / 255.0));*/

		Data[i] = Color888ToShort(&Colour);
		ShortToColor888(Data[i], &Colour);
	}

	return;
}
