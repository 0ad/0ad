//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 09/01/2003 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_pcx.c
//
// Description: Reads and writes from/to a .pcx file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PCX
#include "il_pcx.h"
#include "il_manip.h"


//! Checks if the file specified in FileName is a valid .pcx file.
ILboolean ilIsValidPcx(const ILstring FileName)
{
	ILHANDLE	PcxFile;
	ILboolean	bPcx = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("pcx"))) {
		ilSetError(IL_INVALID_EXTENSION);
		return bPcx;
	}

	PcxFile = iopenr(FileName);
	if (PcxFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPcx;
	}

	bPcx = ilIsValidPcxF(PcxFile);
	icloser(PcxFile);

	return bPcx;
}


//! Checks if the ILHANDLE contains a valid .pcx file at the current position.
ILboolean ilIsValidPcxF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iIsValidPcx();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid .pcx lump.
ILboolean ilIsValidPcxL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iIsValidPcx();
}


// Internal function obtain the .pcx header from the current file.
ILboolean iGetPcxHead(PCXHEAD *Head)
{
	if (iread(Head, sizeof(PCXHEAD), 1) != 1)
		return IL_FALSE;

	UShort(&Head->Xmin);
	UShort(&Head->Ymin);
	UShort(&Head->Xmax);
	UShort(&Head->Ymax);
	UShort(&Head->HDpi);
	UShort(&Head->VDpi);
	UShort(&Head->Bps);
	UShort(&Head->PaletteInfo);
	UShort(&Head->HScreenSize);
	UShort(&Head->VScreenSize);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidPcx()
{
	PCXHEAD Head;

	if (!iGetPcxHead(&Head))
		return IL_FALSE;
	iseek(-(ILint)sizeof(PCXHEAD), IL_SEEK_CUR);

	return iCheckPcx(&Head);
}


// Internal function used to check if the HEADER is a valid .pcx header.
// Should we also do a check on Header->Bpp?
ILboolean iCheckPcx(PCXHEAD *Header)
{
	ILuint	Test, i;

	//	Got rid of the Reserved check, because I've seen some .pcx files with invalid values in it.
	if (Header->Manufacturer != 10 || Header->Encoding != 1/* || Header->Reserved != 0*/)
		return IL_FALSE;

	// Try to support all pcx versions, as they only differ in allowed formats...
	// Let's hope it works.
	if(Header->Version != 5 && Header->Version != 0 && Header->Version != 2 &&
		 Header->VDpi != 3 && Header->VDpi != 4)
		return IL_FALSE;

	// See if the padding size is correct
	Test = Header->Xmax - Header->Xmin + 1;
	if (Header->Bpp >= 8) {
		if (Test & 1) {
			if (Header->Bps != Test + 1)
				return IL_FALSE;
		}
		else {
			if (Header->Bps != Test)  // No padding
				return IL_FALSE;
		}
	}

	for (i = 0; i < 54; i++) {
		if (Header->Filler[i] != 0)
			return IL_FALSE;
	}

	return IL_TRUE;
}


//! Reads a .pcx file
ILboolean ilLoadPcx(const ILstring FileName)
{
	ILHANDLE	PcxFile;
	ILboolean	bPcx = IL_FALSE;

	PcxFile = iopenr(FileName);
	if (PcxFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPcx;
	}

	bPcx = ilLoadPcxF(PcxFile);
	icloser(PcxFile);

	return bPcx;
}


//! Reads an already-opened .pcx file
ILboolean ilLoadPcxF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadPcxInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .pcx
ILboolean ilLoadPcxL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadPcxInternal();
}


// Internal function used to load the .pcx.
ILboolean iLoadPcxInternal()
{
	PCXHEAD	Header;
	ILboolean bPcx = IL_FALSE;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return bPcx;
	}

	if (!iGetPcxHead(&Header))
		return IL_FALSE;
	if (!iCheckPcx(&Header)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	bPcx = iUncompressPcx(&Header);

	ilFixImage();

	return bPcx;
}


// Internal function to uncompress the .pcx (all .pcx files are rle compressed)
ILboolean iUncompressPcx(PCXHEAD *Header)
{
	//changed decompression loop 2003-09-01
	//From the pcx spec: "There should always
	//be a decoding break at the end of each scan line.
	//But there will not be a decoding break at the end of
	//each plane within each scan line."
	//This is now handled correctly (hopefully ;) )

	ILubyte	ByteHead, Colour, *ScanLine /* For all planes */;
	ILuint ScanLineSize;
	ILuint	c, i, x, y;

	if (Header->Bpp < 8) {
		/*ilSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;*/
		return iUncompressSmall(Header);
	}

	if (!ilTexImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 0, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;
	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (iCurImage->Bpp)
	{
		case 1:
			iCurImage->Format = IL_COLOUR_INDEX;
			iCurImage->Pal.PalType = IL_PAL_RGB24;
			iCurImage->Pal.PalSize = 256 * 3;  // Need to find out for sure...
			iCurImage->Pal.Palette = (ILubyte*)ialloc(iCurImage->Pal.PalSize);
			if (iCurImage->Pal.Palette == NULL) {
				return IL_FALSE;
			}
			break;
		//case 2:  // No 16-bit images in the pcx format!
		case 3:
			iCurImage->Format = IL_RGB;
			iCurImage->Pal.Palette = NULL;
			iCurImage->Pal.PalSize = 0;
			iCurImage->Pal.PalType = IL_PAL_NONE;
			break;
		case 4:
			iCurImage->Format = IL_RGBA;
			iCurImage->Pal.Palette = NULL;
			iCurImage->Pal.PalSize = 0;
			iCurImage->Pal.PalType = IL_PAL_NONE;
			break;

		default:
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	ScanLineSize = iCurImage->Bpp*Header->Bps;
	ScanLine = (ILubyte*)ialloc(ScanLineSize);
	if (ScanLine == NULL) {
		return IL_FALSE;
	}


	//changed 2003-09-01
	//having the decoding code twice is error-prone,
	//so I made iUnCache() smart enough to grasp
	//if iPreCache() wasn't called and call it
	//anyways.
	if (iGetHint(IL_MEM_SPEED_HINT) == IL_FASTEST)
		iPreCache(iCurImage->SizeOfData / 4);

	for (y = 0; y < iCurImage->Height; y++) {
		x = 0;
		//read scanline
		while (x < ScanLineSize) {
			if (iread(&ByteHead, 1, 1) != 1) {
				iUnCache();
				goto file_read_error;
			}
			if ((ByteHead & 0xC0) == 0xC0) {
				ByteHead &= 0x3F;
				if (iread(&Colour, 1, 1) != 1) {
					iUnCache();
					goto file_read_error;
				}
				if (x + ByteHead > ScanLineSize) {
					iUnCache();
					goto file_read_error;
				}
				for (i = 0; i < ByteHead; i++) {
					ScanLine[x++] = Colour;
				}
			}
			else {
				ScanLine[x++] = ByteHead;
			}
		}

		//convert plane-separated scanline into index, rgb or rgba pixels.
		//there might be a padding byte at the end of each scanline...
		for (x = 0; x < iCurImage->Width; x++) {
			for(c = 0; c < iCurImage->Bpp; c++) {
				iCurImage->Data[y * iCurImage->Bps + x * iCurImage->Bpp + c] =
						ScanLine[x + c * Header->Bps];
			}
		}
	}

	iUnCache();

	// Read in the palette
	if (Header->Version == 5 && iCurImage->Bpp == 1) {
		x = itell();
		if (iread(&ByteHead, 1, 1) == 0) {  // If true, assume that we have a luminance image.
			ilGetError();  // Get rid of the IL_FILE_READ_ERROR.
			iCurImage->Format = IL_LUMINANCE;
			if (iCurImage->Pal.Palette)
				ifree(iCurImage->Pal.Palette);
			iCurImage->Pal.PalSize = 0;
			iCurImage->Pal.PalType = IL_PAL_NONE;
		}
		else {
			if (ByteHead != 12)  // Some Quake2 .pcx files don't have this byte for some reason.
				iseek(-1, IL_SEEK_CUR);
			if (iread(iCurImage->Pal.Palette, 1, iCurImage->Pal.PalSize) != iCurImage->Pal.PalSize)
				goto file_read_error;
		}
	}

	ifree(ScanLine);

	return IL_TRUE;

file_read_error:
	ifree(ScanLine);

	//added 2003-09-01
	ilSetError(IL_FILE_READ_ERROR);
	return IL_FALSE;
}


ILboolean iUncompressSmall(PCXHEAD *Header)
{
	ILuint	i = 0, j, k, c, d, x, y, Bps;
	ILubyte	HeadByte, Colour, Data = 0, *ScanLine;

	if (!ilTexImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, 1, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (Header->NumPlanes)
	{
		case 1:
			iCurImage->Format = IL_LUMINANCE;
			break;
		case 4:
			iCurImage->Format = IL_COLOUR_INDEX;
			break;
		default:
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	if (Header->NumPlanes == 1 && Header->Bpp == 1) {
		for (j = 0; j < iCurImage->Height; j++) {
			i = 0; //number of written pixels
			while (i < iCurImage->Width) {
				if (iread(&HeadByte, 1, 1) != 1)
					return IL_FALSE;
				if (HeadByte >= 192) {
					HeadByte -= 192;
					if (iread(&Data, 1, 1) != 1)
						return IL_FALSE;

					for (c = 0; c < HeadByte; c++) {
						k = 128;
						for (d = 0; d < 8 && i < iCurImage->Width; d++) {
							iCurImage->Data[j * iCurImage->Width + i++] = ((Data & k) != 0 ? 255 : 0);
							k >>= 1;
						}
					}
				}
				else {
					k = 128;
					for (c = 0; c < 8 && i < iCurImage->Width; c++) {
						iCurImage->Data[j * iCurImage->Width + i++] = ((HeadByte & k) != 0 ? 255 : 0);
						k >>= 1;
					}
				}
			}

			//if(Data != 0)
			//changed 2003-09-01:
			//There has to be an even number of bytes per line in a pcx.
			//One byte can hold up to 8 bits, so Width/8 bytes
			//are needed to hold a 1 bit per pixel image line.
			//If Width/8 is even no padding is needed,
			//one pad byte has to be read otherwise.
			//(let's hope the above is true ;-))
			if(!((iCurImage->Width >> 3) & 0x1))
				igetc();	// Skip pad byte
		}
	}
	else if (Header->NumPlanes == 4 && Header->Bpp == 1){   // 4-bit images
		//changed decoding 2003-09-10 (was buggy)...could need a speedup

		Bps = Header->Bps * Header->NumPlanes * 8;
		iCurImage->Pal.Palette = (ILubyte*)ialloc(16 * 3);  // Size of palette always (48 bytes).
		ScanLine = (ILubyte*)ialloc(Bps);
		if (iCurImage->Pal.Palette == NULL || ScanLine == NULL) {
			ifree(ScanLine);
			ifree(iCurImage->Pal.Palette);
			return IL_FALSE;
		}
		memcpy(iCurImage->Pal.Palette, Header->ColMap, 16 * 3);
		iCurImage->Pal.PalSize = 16 * 3;
		iCurImage->Pal.PalType = IL_PAL_RGB24;

		memset(iCurImage->Data, 0, iCurImage->SizeOfData);

		if (iGetHint(IL_MEM_SPEED_HINT) == IL_FASTEST)
			iPreCache(iCurImage->SizeOfData / 4);
		for (y = 0; y < iCurImage->Height; y++) {
			x = 0;
			while (x < Bps) {
				if (iread(&HeadByte, 1, 1) != 1) {
					iUnCache();
					ifree(ScanLine);
					return IL_FALSE;
				}
				if ((HeadByte & 0xC0) == 0xC0) {
					HeadByte &= 0x3F;
					if (iread(&Colour, 1, 1) != 1) {
						iUnCache();
						ifree(ScanLine);
						return IL_FALSE;
					}
					for (i = 0; i < HeadByte; i++) {
						k = 128;
						for (j = 0; j < 8 && x < Bps; j++) {
							ScanLine[x++] = (Colour & k)?1:0;
							k >>= 1;
						}
					}
				}
				else {
					k = 128;
					for (j = 0; j < 8 && x < Bps; j++) {
						ScanLine[x++] = (HeadByte & k)?1:0;
						k >>= 1;
					}
				}
			}

			for (x = 0; x < iCurImage->Width; x++) {  // 'Cleverly' ignores the pad bytes. ;)
				for(c = 0; c < Header->NumPlanes; c++)
					iCurImage->Data[y * iCurImage->Width + x] |= ScanLine[x + c*Header->Bps*8] << c;
			}
		}
		iUnCache();
		ifree(ScanLine);
	}
	else {
		ilSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}

	return IL_TRUE;
}


//! Writes a .pcx file
ILboolean ilSavePcx(const ILstring FileName)
{
	ILHANDLE	PcxFile;
	ILboolean	bPcx = IL_FALSE;

	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	PcxFile = iopenw(FileName);
	if (PcxFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPcx;
	}

	bPcx = ilSavePcxF(PcxFile);
	iclosew(PcxFile);

	return bPcx;
}


//! Writes a .pcx to an already-opened file
ILboolean ilSavePcxF(ILHANDLE File)
{
	iSetOutputFile(File);
	return iSavePcxInternal();
}


//! Writes a .pcx to a memory "lump"
ILboolean ilSavePcxL(ILvoid *Lump, ILuint Size)
{
	iSetOutputLump(Lump, Size);
	return iSavePcxInternal();
}


// Internal function used to save the .pcx.
ILboolean iSavePcxInternal()
{
	ILuint	i, c, PalSize;
	ILpal	*TempPal;
	ILimage	*TempImage = iCurImage;
	ILubyte	*TempData;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	switch (iCurImage->Format)
	{
		case IL_LUMINANCE:
			TempImage = iConvertImage(iCurImage, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
			break;

		case IL_BGR:
			TempImage = iConvertImage(iCurImage, IL_RGB, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
			break;

		case IL_BGRA:
			TempImage = iConvertImage(iCurImage, IL_RGBA, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
			break;

		default:
			if (iCurImage->Bpc > 1) {
				TempImage = iConvertImage(iCurImage, iCurImage->Format, IL_UNSIGNED_BYTE);
				if (TempImage == NULL)
					return IL_FALSE;
			}
	}

	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			if (TempImage != iCurImage) {
				ilCloseImage(TempImage);
			}
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}


	iputc(0xA);  // Manufacturer - always 10
	iputc(0x5);  // Version Number - always 5
	iputc(0x1);  // Encoding - always 1
	iputc(0x8);  // Bits per channel
	SaveLittleUShort(0);  // X Minimum
	SaveLittleUShort(0);  // Y Minimum
	SaveLittleUShort((ILushort)(iCurImage->Width - 1));
	SaveLittleUShort((ILushort)(iCurImage->Height - 1));
	SaveLittleUShort(0);
	SaveLittleUShort(0);

	// Useless palette info?
	for (i = 0; i < 48; i++) {
		iputc(0);
	}
	iputc(0x0);  // Reserved - always 0

	iputc(iCurImage->Bpp);  // Number of planes - only 1 is supported right now

	SaveLittleUShort((ILushort)(iCurImage->Width & 1 ? iCurImage->Width + 1 : iCurImage->Width));  // Bps
	SaveLittleUShort(0x1);  // Palette type - ignored?

	// Mainly filler info
	for (i = 0; i < 58; i++) {
		iputc(0x0);
	}

	// Output data
	for (i = 0; i < TempImage->Height; i++) {
		for (c = 0; c < TempImage->Bpp; c++) {
			encLine(TempData + TempImage->Bps * i + c, TempImage->Width, (ILubyte)(TempImage->Bpp - 1));
		}
	}

	// Automatically assuming we have a palette...dangerous!
	//	Also assuming 3 bpp palette
	iputc(0xC);  // Pad byte must have this value

	// If the current image has a palette, take care of it
	if (TempImage->Format == IL_COLOUR_INDEX) {
		// If the palette in .pcx format, write it directly
		if (TempImage->Pal.PalType == IL_PAL_RGB24) {
			iwrite(TempImage->Pal.Palette, 1, TempImage->Pal.PalSize);
		}
		else {
			TempPal = iConvertPal(&TempImage->Pal, IL_PAL_RGB24);
			if (TempPal == NULL) {
				if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
					ifree(TempData);
				if (TempImage != iCurImage)
					ilCloseImage(TempImage);
				return IL_FALSE;
			}

			iwrite(TempPal->Palette, 1, TempPal->PalSize);
			ifree(TempPal->Palette);
			ifree(TempPal);
		}
	}

	// If the palette is not all 256 colours, we have to pad it.
	PalSize = 768 - iCurImage->Pal.PalSize;
	for (i = 0; i < PalSize; i++) {
		iputc(0x0);
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (TempImage != iCurImage)
		ilCloseImage(TempImage);

	return IL_TRUE;
}


// Routine used from ZSoft's pcx documentation
ILuint encput(ILubyte byt, ILubyte cnt)
{
	if (cnt) {
		if ((cnt == 1) && (0xC0 != (0xC0 & byt))) {
			if (IL_EOF == iputc(byt))
				return(0);     /* disk write error (probably full) */
			return(1);
		}
		else {
			if (IL_EOF == iputc((ILubyte)((ILuint)0xC0 | cnt)))
				return (0);      /* disk write error */
			if (IL_EOF == iputc(byt))
				return (0);      /* disk write error */
			return (2);
		}
	}

	return (0);
}

/* This subroutine encodes one scanline and writes it to a file.
It returns number of bytes written into outBuff, 0 if failed. */

ILuint encLine(ILubyte *inBuff, ILint inLen, ILubyte Stride)
{
	ILubyte _this, last;
	ILint srcIndex, i;
	ILint total;
	ILubyte runCount;     // max single runlength is 63
	total = 0;
	runCount = 1;
	last = *(inBuff);

	// Find the pixel dimensions of the image by calculating 
	//[XSIZE = Xmax - Xmin + 1] and [YSIZE = Ymax - Ymin + 1].  
	//Then calculate how many bytes are in a "run"

	for (srcIndex = 1; srcIndex < inLen; srcIndex++) {
		inBuff += Stride;
		_this = *(++inBuff);
		if (_this == last) {  // There is a "run" in the data, encode it
			runCount++;
			if (runCount == 63) {
				if (! (i = encput(last, runCount)))
						return (0);
				total += i;
				runCount = 0;
			}
		}
		else {  // No "run"  -  _this != last
			if (runCount) {
				if (! (i = encput(last, runCount)))
					return(0);
				total += i;
			}
			last = _this;
			runCount = 1;
		}
	}  // endloop

	if (runCount) {  // finish up
		if (! (i = encput(last, runCount)))
			return (0);
		if (inLen % 2)
			iputc(0);
		return (total + i);
	}
	else {
		if (inLen % 2)
			iputc(0);
	}

	return (total);
}

#endif//IL_NO_PCX
