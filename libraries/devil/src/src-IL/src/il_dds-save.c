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


#ifndef IL_NO_DDS

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


/*
ILushort *CompressTo565(ILimage *Image)
{
	ILimage		*TempImage;
	ILushort	*Data;
	ILuint		i, j;

	if ((Image->Type != IL_UNSIGNED_BYTE && Image->Type != IL_BYTE) || Image->Format == IL_COLOUR_INDEX) {
		TempImage = iConvertImage(iCurImage, IL_BGR, IL_UNSIGNED_BYTE);  // @TODO: Needs to be BGRA.
		if (TempImage == NULL)
			return NULL;
	}
	else {
		TempImage = Image;
	}

	Data = (ILushort*)ialloc(iCurImage->Width * iCurImage->Height * 2);
	if (Data == NULL) {
		if (TempImage != Image)
			ilCloseImage(TempImage);
		return NULL;
	}

	//changed 20040623: Use TempImages format :)
	switch (TempImage->Format)
	{
		case IL_RGB:
			for (i = 0, j = 0; i < TempImage->SizeOfPlane; i += 3, j++) {
				Data[j]  = (TempImage->Data[i  ] >> 3) << 11;
				Data[j] |= (TempImage->Data[i+1] >> 2) << 5;
				Data[j] |=  TempImage->Data[i+2] >> 3;
			}
			break;

		case IL_RGBA:
			for (i = 0, j = 0; i < TempImage->SizeOfPlane; i += 4, j++) {
				Data[j]  = (TempImage->Data[i  ] >> 3) << 11;
				Data[j] |= (TempImage->Data[i+1] >> 2) << 5;
				Data[j] |=  TempImage->Data[i+2] >> 3;
			}
			break;

		case IL_BGR:
			for (i = 0, j = 0; i < TempImage->SizeOfPlane; i += 3, j++) {
				Data[j]  = (TempImage->Data[i+2] >> 3) << 11;
				Data[j] |= (TempImage->Data[i+1] >> 2) << 5;
				Data[j] |=  TempImage->Data[i  ] >> 3;
			}
			break;

		case IL_BGRA:
			for (i = 0, j = 0; i < TempImage->SizeOfPlane; i += 4, j++) {
				Data[j]  = (TempImage->Data[i+2] >> 3) << 11;
				Data[j] |= (TempImage->Data[i+1] >> 2) << 5;
				Data[j] |=  TempImage->Data[i  ] >> 3;
			}
			break;

		case IL_LUMINANCE:
			for (i = 0, j = 0; i < TempImage->SizeOfPlane; i++, j++) {
				Data[j]  = (TempImage->Data[i] >> 3) << 11;
				Data[j] |= (TempImage->Data[i] >> 2) << 5;
				Data[j] |=  TempImage->Data[i] >> 3;
			}
			break;

		case IL_LUMINANCE_ALPHA:
			for (i = 0, j = 0; i < TempImage->SizeOfPlane; i += 2, j++) {
				Data[j]  = (TempImage->Data[i] >> 3) << 11;
				Data[j] |= (TempImage->Data[i] >> 2) << 5;
				Data[j] |=  TempImage->Data[i] >> 3;
			}
			break;
	}

	if (TempImage != Image)
		ilCloseImage(TempImage);

	return Data;
}
*/


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

ILuint Compress(ILimage *Image, ILenum DXTCFormat)
{
	ILubyte		*Data;
	ILubyte		Block[16*3];
	ILushort	ex0, ex1;
	ILuint		x, y, i, BitMask;//, Rms1, Rms2;
	ILubyte		*Alpha, AlphaBlock[16], AlphaBitMask[6], AlphaOut[16], a0, a1;
	ILboolean	HasAlpha;
	ILuint		Count = 0;

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
					GetAlphaBlock(AlphaBlock, Alpha, Image, x, y);
					HasAlpha = IL_FALSE;
					for (i = 0 ; i < 16; i++) {
						if (AlphaBlock[i] < 128) {
							HasAlpha = IL_TRUE;
							break;
						}
					}

					GetBlock(Block, Data, Image, x, y);
					if (HasAlpha)
						ChooseEndpoints(&ex0, &ex1, 3, Block, AlphaBlock);
					else
						ChooseEndpoints(&ex0, &ex1, 4, Block, NULL);
					CorrectEndDXT1(&ex0, &ex1, HasAlpha);
					SaveLittleUShort(ex0);
					SaveLittleUShort(ex1);
					if (HasAlpha)
						BitMask = GenBitMask(ex0, ex1, 3, Block, AlphaBlock, NULL);
					else
						BitMask = GenBitMask(ex0, ex1, 4, Block, NULL, NULL);
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
					GetAlphaBlock(AlphaBlock, Alpha, Image, x, y);
					for (i = 0; i < 16; i += 2) {
						iputc((ILubyte)(((AlphaBlock[i+1] >> 4) << 4) | (AlphaBlock[i] >> 4)));
					}

					GetBlock(Block, Data, Image, x, y);
					ChooseEndpoints(&ex0, &ex1, 4, Block, NULL);
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
					GetAlphaBlock(AlphaBlock, Alpha, Image, x, y);
					ChooseAlphaEndpoints(AlphaBlock, &a0, &a1);
					GenAlphaBitMask(a0, a1, 6, AlphaBlock, AlphaBitMask, AlphaOut);
					/*Rms2 = RMSAlpha(AlphaBlock, AlphaOut);
					GenAlphaBitMask(a0, a1, 8, AlphaBlock, AlphaBitMask, AlphaOut);
					Rms1 = RMSAlpha(AlphaBlock, AlphaOut);
					if (Rms2 <= Rms1) {  // Yeah, we have to regenerate...
						GenAlphaBitMask(a0, a1, 6, AlphaBlock, AlphaBitMask, AlphaOut);
						Rms2 = a1;  // Just reuse Rms2 as a temporary variable...
						a1 = a0;
						a0 = Rms2;
					}*/
					iputc(a0);
					iputc(a1);
					iwrite(AlphaBitMask, 1, 6);

					GetBlock(Block, Data, Image, x, y);
					ChooseEndpoints(&ex0, &ex1, 4, Block, NULL);
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

	return Count;
}


// Assumed to be 24-bit (8:8:8).
ILboolean GetBlock(ILubyte *Block, ILubyte *Data, ILimage *Image, ILuint XPos, ILuint YPos)
{
	ILuint x, y, i = 0, Offset;

	for (y = 0; y < 4; y++) {
		Offset = ((YPos + y) * Image->Width + XPos) * 3;
		for (x = 0; x < 4*3; x++)
			Block[i++] = Data[Offset++];
	}

	return IL_TRUE;
}


ILboolean GetAlphaBlock(ILubyte *Block, ILubyte *Data, ILimage *Image, ILuint XPos, ILuint YPos)
{
	ILuint x, y, i = 0, Offset;

	for (y = 0; y < 4; y++) {
		for (x = 0; x < 4; x++) {
			Offset = (YPos + y) * Image->Width + (XPos + x);
			Block[i++] = Data[Offset];
		}
	}

	return IL_TRUE;
}


ILvoid ShortToColor565(ILushort Pixel, Color565 *Colour)
{
	Colour->nRed   = (Pixel & 0xF800) >> 11;
	Colour->nGreen = (Pixel & 0x07E0) >> 5;
	Colour->nBlue  = (Pixel & 0x001F);
	return;
}


ILvoid ShortToColor888(ILushort Pixel, Color888 *Colour)
{
	// Use 255/31 rather than 256/32, in order to convert 0->31 to
	// 0->255 (rather than to 0->248)
	Colour->r = (((Pixel & 0xF800) >> 11) * 255) / 31;
	Colour->g = (((Pixel & 0x07E0) >>  5) * 255) / 63;
	Colour->b = (((Pixel & 0x001F)      ) * 255) / 31;
	return;
}


ILushort Color565ToShort(Color565 *Colour)
{
	return (Colour->nRed << 11) | (Colour->nGreen << 5) | (Colour->nBlue);
}


ILushort Color888ToShort(Color888 *Colour)
{
	return ((Colour->r >> 3) << 11) | ((Colour->g >> 2) << 5) | (Colour->b >> 3);
}

ILvoid BytesToColor888(ILubyte *Data, Color888 *Colour)
{
	Colour->r = Data[0];
	Colour->g = Data[1];
	Colour->b = Data[2];
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

ILuint ApplyBitMask(Color888 ex0, Color888 ex1, ILuint NumCols, ILubyte *In, ILubyte *Alpha, Color888 *OutCol, ILuint *DistLimit)
{
	Color888	Colours[4];
	ILuint		i, j, ClosestDist, Dist, TotalDist = 0, ClosestIndex, BitMask = 0;
	ILubyte		Mask[16];

	Colours[0] = ex0;
	Colours[1] = ex1;

	if (NumCols == 3) {
		Colours[2].r = (Colours[0].r + Colours[1].r) / 2;
		Colours[2].g = (Colours[0].g + Colours[1].g) / 2;
		Colours[2].b = (Colours[0].b + Colours[1].b) / 2;
		Colours[3].r = (Colours[0].r + Colours[1].r) / 2;
		Colours[3].g = (Colours[0].g + Colours[1].g) / 2;
		Colours[3].b = (Colours[0].b + Colours[1].b) / 2;
	}
	else {  // NumCols == 4
		Colours[2].r = (2 * Colours[0].r + Colours[1].r + 1) / 3;
		Colours[2].g = (2 * Colours[0].g + Colours[1].g + 1) / 3;
		Colours[2].b = (2 * Colours[0].b + Colours[1].b + 1) / 3;
		Colours[3].r = (Colours[0].r + 2 * Colours[1].r + 1) / 3;
		Colours[3].g = (Colours[0].g + 2 * Colours[1].g + 1) / 3;
		Colours[3].b = (Colours[0].b + 2 * Colours[1].b + 1) / 3;
	}

	for (i = 0; i < 16; i++) {
		if (Alpha) {  // Test to see if we have 1-bit transparency
			if (Alpha[i] < 128) {
				Mask[i] = 3;  // Transparent
				if (OutCol) {
					OutCol[i].r = Colours[3].r;
					OutCol[i].g = Colours[3].g;
					OutCol[i].b = Colours[3].b;
				}
				continue;
			}
		}

		// If no transparency, try to find which colour is the closest.

		ClosestDist = UINT_MAX;
		for (j = 0; j < NumCols; j++) {

			// Manually inlined for speed (~25%)
			/* Dist = Distance((Color888*) &In[i*3], &Colours[j]); */
			Dist = (In[i*3  ]-Colours[j].r) * (In[i*3  ]-Colours[j].r)
				 + (In[i*3+1]-Colours[j].g) * (In[i*3+1]-Colours[j].g)
				 + (In[i*3+2]-Colours[j].b) * (In[i*3+2]-Colours[j].b)
				 ;

			if (Dist < ClosestDist) {
				ClosestDist = Dist;
				ClosestIndex = j;
			}
		}

		if (DistLimit) {
			TotalDist += ClosestDist; // max is (255^2 * 3) * 16, which is < 2^30, so no chance of an integer overflow
			if (TotalDist >= *DistLimit)
				return UINT_MAX; // give up, since this is already worse than the previous best
		}
		else {
			Mask[i] = ClosestIndex;
			if (OutCol) {
				OutCol[i].r = Colours[ClosestIndex].r;
				OutCol[i].g = Colours[ClosestIndex].g;
				OutCol[i].b = Colours[ClosestIndex].b;
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


ILvoid ChooseEndpoints(ILushort *ex0, ILushort *ex1, ILuint NumCols, ILubyte *Block, ILubyte *Alpha)
{
	ILuint		i, j;
	ILint		a,b,c, k;
	Color888	ClosestC0, ClosestC1, NewC0, NewC1;
	ILuint		ClosestDist = UINT_MAX, dist;
	Color888	Col_i, Col_j;

	// First, try every combination of 2 colours from the image:

	for (i = 0; i < 16; i++) {

		BytesToColor888(&Block[i*3], &Col_i);
		QuantizeColor888To565(&Col_i);

		for (j = i+1; j < 16; j++) {

			BytesToColor888(&Block[j*3], &Col_j);
			QuantizeColor888To565(&Col_j);

			dist = ApplyBitMask(Col_i, Col_j, NumCols, Block, Alpha, NULL, &ClosestDist);

			if (dist < ClosestDist) {
				ClosestC0 = Col_i;
				ClosestC1 = Col_j;
				ClosestDist = dist;

				if (dist == 0)
					i = j = 16; // break out of both 'for' loops if there's a perfect match
			}
		}
	}

	// If we've found a perfect match (which often happens when re-compressing
	// a DDS image), stop now.
	if (ClosestDist == 0)
	{
		*ex0 = Color888ToShort(&ClosestC0);
		*ex1 = Color888ToShort(&ClosestC1);

		return;
	}


	// Usually the match is good enough at this point, so the code could just
	// stop here. But it can be improved, so try some brute-force (and fairly
	// slow) methods of improving the match:


//*
	// First, move one endpoint around every point in a cube to find a better
	// match. Then do the same for the second endpoint, and then repeat with
	// smaller higher-resolution cubes.

	for (i = 0; i < 3; ++i) {
		int range, step;
		if (i == 0)
			range = 24, step = 8; // loops 686 times
		else if (i == 1)
			range = 8, step = 4; // loops 250 times
		else
			range = 2, step = 1; // loops 250 times

		for (a = -range; a <= range; a += step)
			for (b = -range*2; b <= range*2; b += step) // green has twice the resolution, so use twice the range
				for (c = -range; c <= range; c += step) {

					Color888 TempC;

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

//*/

//*

	// Maybe we could stop now, but try a few more adjustments to get even
	// better results (at the expense of speed):

	// Do a similar blind-searching-over-lots-of-values thing to before, but
	// adjusting each component individually - this allows each to be tested
	// over a greater range/resolution, since the number of tests is O(n) rather
	// than O(n^3) (where n = number of values tested for each component)

	// TODO: Does this actually help the quality noticeably?

	NewC0 = ClosestC0;
	NewC1 = ClosestC1;

	// Repeat it lots, since it often gets better each time
	for (i = 0; i < 8; ++i) {

		ILint SearchLimit;

		// Do one wide search, then get narrower (to avoid wasting time)
		if (i == 0) SearchLimit = 16;
		else if (i <= 4) SearchLimit = 8;
		else SearchLimit = 2;

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
//*/

	*ex0 = Color888ToShort(&ClosestC0);
	*ex1 = Color888ToShort(&ClosestC1);

	return;
}


ILvoid ChooseAlphaEndpoints(ILubyte *Block, ILubyte *a0, ILubyte *a1)
{
	ILuint	i;
	ILuint	Lowest = 0xFF, Highest = 0;
	ILboolean flip = IL_FALSE;

	*a1 = Lowest;
	*a0 = Highest;

	for (i = 0; i < 16; i++) {
		if (Block[i] == 0) // use 0, 255 as endpoints
			flip = IL_TRUE;
		else if (Block[i] < Lowest) {
			*a1 = Block[i];  // a1 is the lower of the two.
			Lowest = Block[i];
		}

		if (Block[i] == 255) //use 0, 255 as endpoints
			flip = IL_TRUE;
		else if (Block[i] > Highest) {
			*a0 = Block[i];  // a0 is the higher of the two.
			Highest = Block[i];
		}
	}

	if (flip) {
		i = *a0;
		*a0 = *a1;
		*a1 = i;
	}


	return;
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
