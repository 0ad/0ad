//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/20/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_dcx.c
//
// Description: Reads from a .dcx file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_DCX
#include "il_dcx.h"
#include "il_manip.h"


//! Checks if the file specified in FileName is a valid .dcx file.
ILboolean ilIsValidDcx(const ILstring FileName)
{
	ILHANDLE	DcxFile;
	ILboolean	bDcx = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("dcx"))) {
		ilSetError(IL_INVALID_EXTENSION);
		return bDcx;
	}

	DcxFile = iopenr(FileName);
	if (DcxFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bDcx;
	}

	bDcx = ilIsValidDcxF(DcxFile);
	icloser(DcxFile);

	return bDcx;
}


//! Checks if the ILHANDLE contains a valid .dcx file at the current position.
ILboolean ilIsValidDcxF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iIsValidDcx();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid .dcx lump.
ILboolean ilIsValidDcxL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iIsValidDcx();
}


// Internal function obtain the .dcx header from the current file.
ILboolean iGetDcxHead(DCXHEAD *Head)
{
	if (iread(Head, sizeof(DCXHEAD), 1) != 1)
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
ILboolean iIsValidDcx()
{
	ILuint Signature;

	if (iread(&Signature, 1, 4) != 4)
		return IL_FALSE;
	iseek(-4, IL_SEEK_CUR);

	return (Signature == 987654321);
}


// Internal function used to check if the HEADER is a valid .dcx header.
// Should we also do a check on Header->Bpp?
ILboolean iCheckDcx(DCXHEAD *Header)
{
	ILuint	Test, i;

	// There are other versions, but I am not supporting them as of yet.
	//	Got rid of the Reserved check, because I've seen some .dcx files with invalid values in it.
	if (Header->Manufacturer != 10 || Header->Version != 5 || Header->Encoding != 1/* || Header->Reserved != 0*/)
		return IL_FALSE;

	// See if the padding size is correct
	Test = Header->Xmax - Header->Xmin + 1;
	/*if (Header->Bpp >= 8) {
		if (Test & 1) {
			if (Header->Bps != Test + 1)
				return IL_FALSE;
		}
		else {
			if (Header->Bps != Test)  // No padding
				return IL_FALSE;
		}
	}*/

	for (i = 0; i < 54; i++) {
		if (Header->Filler[i] != 0)
			return IL_FALSE;
	}

	return IL_TRUE;
}


//! Reads a .dcx file
ILboolean ilLoadDcx(const ILstring FileName)
{
	ILHANDLE	DcxFile;
	ILboolean	bDcx = IL_FALSE;

	DcxFile = iopenr(FileName);
	if (DcxFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bDcx;
	}

	bDcx = ilLoadDcxF(DcxFile);
	icloser(DcxFile);

	return bDcx;
}


//! Reads an already-opened .dcx file
ILboolean ilLoadDcxF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadDcxInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .dcx
ILboolean ilLoadDcxL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadDcxInternal();
}


// Internal function used to load the .dcx.
ILboolean iLoadDcxInternal()
{
	DCXHEAD	Header;
	ILuint	Signature, i, Entries[1024], Num = 0;
	ILimage	*Image, *Base;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iIsValidDcx())
		return IL_FALSE;
	iread(&Signature, 1, 4);

	do {
		if (iread(&Entries[Num], 1, 4) != 4)
			return IL_FALSE;
		Num++;
	} while (Entries[Num-1] != 0);

	iCurImage->NumNext = Num - 1;

	for (i = 0; i < Num; i++) {
		iseek(Entries[i], IL_SEEK_SET);
		iGetDcxHead(&Header);
		/*if (!iCheckDcx(&Header)) {
			ilSetError(IL_INVALID_FILE_HEADER);
			return IL_FALSE;
		}*/

		Image = iUncompressDcx(&Header);
		if (Image == NULL)
			return IL_FALSE;

		if (i == 0) {
			ilTexImage(Image->Width, Image->Height, 1, Image->Bpp, Image->Format, Image->Type, Image->Data);
			Base = iCurImage;
			Base->Origin = IL_ORIGIN_UPPER_LEFT;
			ilCloseImage(Image);
		}
		else {
			iCurImage->Next = Image;
			iCurImage = iCurImage->Next;
		}
	}

	ilFixImage();

	return IL_TRUE;
}


// Internal function to uncompress the .dcx (all .dcx files are rle compressed)
ILimage *iUncompressDcx(DCXHEAD *Header)
{
	ILubyte		ByteHead, Colour, *ScanLine = NULL /* Only one plane */;
	ILuint		c, i, x, y;//, Read = 0;
	ILimage		*Image = NULL;

	if (Header->Bpp < 8) {
		/*ilSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;*/
		return iUncompressDcxSmall(Header);
	}

	Image = ilNewImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 1);
	if (Image == NULL)
		return NULL;
	/*if (!ilTexImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}*/
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	ScanLine = (ILubyte*)ialloc(Header->Bps);
	if (ScanLine == NULL)
		goto dcx_error;

	switch (Image->Bpp)
	{
		case 1:
			Image->Format = IL_COLOUR_INDEX;
			Image->Pal.PalType = IL_PAL_RGB24;
			Image->Pal.PalSize = 256 * 3; // Need to find out for sure...
			Image->Pal.Palette = (ILubyte*)ialloc(Image->Pal.PalSize);
			if (Image->Pal.Palette == NULL)
				goto dcx_error;
			break;
		//case 2:  // No 16-bit images in the dcx format!
		case 3:
			Image->Format = IL_RGB;
			Image->Pal.Palette = NULL;
			Image->Pal.PalSize = 0;
			Image->Pal.PalType = IL_PAL_NONE;
			break;
		case 4:
			Image->Format = IL_RGBA;
			Image->Pal.Palette = NULL;
			Image->Pal.PalSize = 0;
			Image->Pal.PalType = IL_PAL_NONE;
			break;

		default:
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			goto dcx_error;
	}


		/*StartPos = itell();
		Compressed = (ILubyte*)ialloc(Image->SizeOfData * 4 / 3);
		iread(Compressed, 1, Image->SizeOfData * 4 / 3);

		for (y = 0; y < Image->Height; y++) {
			for (c = 0; c < Image->Bpp; c++) {
				x = 0;
				while (x < Header->Bps) {
					ByteHead = Compressed[Read++];
					if ((ByteHead & 0xC0) == 0xC0) {
						ByteHead &= 0x3F;
						Colour = Compressed[Read++];
						for (i = 0; i < ByteHead; i++) {
							ScanLine[x++] = Colour;
						}
					}
					else {
						ScanLine[x++] = ByteHead;
					}
				}

				for (x = 0; x < Image->Width; x++) {  // 'Cleverly' ignores the pad bytes ;)
					Image->Data[y * Image->Bps + x * Image->Bpp + c] = ScanLine[x];
				}
			}
		}

		ifree(Compressed);
		iseek(StartPos + Read, IL_SEEK_SET);*/

	//changed 2003-09-01
	if (iGetHint(IL_MEM_SPEED_HINT) == IL_FASTEST)
		iPreCache(iCurImage->SizeOfData);

	//TODO: because the .pcx-code was broken this
	//code is probably broken, too
	for (y = 0; y < Image->Height; y++) {
		for (c = 0; c < Image->Bpp; c++) {
			x = 0;
			while (x < Header->Bps) {
				if (iread(&ByteHead, 1, 1) != 1) {
					iUnCache();
					goto dcx_error;
				}
				if ((ByteHead & 0xC0) == 0xC0) {
					ByteHead &= 0x3F;
					if (iread(&Colour, 1, 1) != 1) {
						iUnCache();
						goto dcx_error;
					}
					for (i = 0; i < ByteHead; i++) {
						ScanLine[x++] = Colour;
					}
				}
				else {
					ScanLine[x++] = ByteHead;
				}
			}

			for (x = 0; x < Image->Width; x++) {  // 'Cleverly' ignores the pad bytes ;)
				Image->Data[y * Image->Bps + x * Image->Bpp + c] = ScanLine[x];
			}
		}
	}

	iUnCache();


	ifree(ScanLine);

	// Read in the palette
	if (Image->Bpp == 1) {
		ByteHead = igetc();	// the value 12, because it signals there's a palette for some reason...
							//	We should do a check to make certain it's 12...
		if (ByteHead != 12)
			iseek(-1, IL_SEEK_CUR);
		if (iread(Image->Pal.Palette, 1, Image->Pal.PalSize) != Image->Pal.PalSize) {
			ilCloseImage(Image);
			return NULL;
		}
	}

	return Image;

dcx_error:
	ifree(ScanLine);
	ilCloseImage(Image);
	return NULL;
}


ILimage *iUncompressDcxSmall(DCXHEAD *Header)
{
	ILuint	i = 0, j, k, c, d, x, y, Bps;
	ILubyte	HeadByte, Colour, Data = 0, *ScanLine = NULL;
	ILimage	*Image;

	Image = ilNewImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 1);
	if (Image == NULL)
		return NULL;

	/*if (!ilTexImage(Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, 1, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}*/
	Image->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (Header->NumPlanes)
	{
		case 1:
			Image->Format = IL_LUMINANCE;
			break;
		case 4:
			Image->Format = IL_COLOUR_INDEX;
			break;
		default:
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			ilCloseImage(Image);
			return NULL;
	}

	if (Header->NumPlanes == 1) {
		for (j = 0; j < Image->Height; j++) {
			i = 0;
			while (i < Image->Width) {
				if (iread(&HeadByte, 1, 1) != 1)
					goto file_read_error;
				if (HeadByte >= 192) {
					HeadByte -= 192;
					if (iread(&Data, 1, 1) != 1)
						goto file_read_error;

					for (c = 0; c < HeadByte; c++) {
						k = 128;
						for (d = 0; d < 8 && i < Image->Width; d++) {
							Image->Data[j * Image->Width + i++] = (!!(Data & k) == 1 ? 255 : 0);
							k >>= 1;
						}
					}
				}
				else {
					k = 128;
					for (c = 0; c < 8 && i < Image->Width; c++) {
						Image->Data[j * Image->Width + i++] = (!!(HeadByte & k) == 1 ? 255 : 0);
						k >>= 1;
					}
				}
			}
			if (Data != 0)
				igetc();  // Skip pad byte if last byte not a 0
		}
	}
	else {   // 4-bit images
		Bps = Header->Bps * Header->NumPlanes * 2;
		Image->Pal.Palette = (ILubyte*)ialloc(16 * 3);  // Size of palette always (48 bytes).
		Image->Pal.PalSize = 16 * 3;
		Image->Pal.PalType = IL_PAL_RGB24;
		ScanLine = (ILubyte*)ialloc(Bps);
		if (Image->Pal.Palette == NULL || ScanLine == NULL) {
			ifree(ScanLine);
			ilCloseImage(Image);
			return NULL;
		}

		memcpy(Image->Pal.Palette, Header->ColMap, 16 * 3);
		imemclear(Image->Data, Image->SizeOfData);  // Since we do a += later.

		for (y = 0; y < Image->Height; y++) {
			for (c = 0; c < Header->NumPlanes; c++) {
				x = 0;
				while (x < Bps) {
					if (iread(&HeadByte, 1, 1) != 1)
						goto file_read_error;
					if ((HeadByte & 0xC0) == 0xC0) {
						HeadByte &= 0x3F;
						if (iread(&Colour, 1, 1) != 1)
							goto file_read_error;
						for (i = 0; i < HeadByte; i++) {
							k = 128;
							for (j = 0; j < 8; j++) {
								ScanLine[x++] = !!(Colour & k);
								k >>= 1;
							}
						}
					}
					else {
						k = 128;
						for (j = 0; j < 8; j++) {
							ScanLine[x++] = !!(HeadByte & k);
							k >>= 1;
						}
					}
				}

				for (x = 0; x < Image->Width; x++) {  // 'Cleverly' ignores the pad bytes. ;)
					Image->Data[y * Image->Width + x] += ScanLine[x] << c;
				}
			}
		}
		ifree(ScanLine);
	}

	return Image;

file_read_error:
	ifree(ScanLine);
	ilCloseImage(Image);
	return NULL;
}

#endif//IL_NO_DCX
