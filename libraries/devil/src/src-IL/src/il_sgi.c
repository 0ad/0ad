//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_states.c
//
// Description: Reads from and writes to SGI graphics files.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_SGI
#include "il_sgi.h"
#include "il_manip.h"
#include <limits.h>

static char *FName = NULL;

/*----------------------------------------------------------------------------*/

/*! Checks if the file specified in FileName is a valid .sgi file. */
ILboolean ilIsValidSgi(const ILstring FileName)
{
	ILHANDLE	SgiFile;
	ILboolean	bSgi = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("sgi"))) {
		ilSetError(IL_INVALID_EXTENSION);
		return bSgi;
	}

	FName = (char*) FileName;

	SgiFile = iopenr(FileName);
	if (SgiFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bSgi;
	}

	bSgi = ilIsValidSgiF(SgiFile);
	icloser(SgiFile);

	return bSgi;
}

/*----------------------------------------------------------------------------*/

/*! Checks if the ILHANDLE contains a valid .sgi file at the current position.*/
ILboolean ilIsValidSgiF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iIsValidSgi();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}

/*----------------------------------------------------------------------------*/

//! Checks if Lump is a valid .sgi lump.
ILboolean ilIsValidSgiL(ILvoid *Lump, ILuint Size)
{
	FName = NULL;
	iSetInputLump(Lump, Size);
	return iIsValidSgi();
}

/*----------------------------------------------------------------------------*/

// Internal function used to get the .sgi header from the current file.
ILboolean iGetSgiHead(iSgiHeader *Header)
{
	if (iread(Header, sizeof(iSgiHeader), 1) != 1)
		return IL_FALSE;

	BigUShort(&Header->MagicNum);
	BigUShort(&Header->Dim);
	BigUShort(&Header->XSize);
	BigUShort(&Header->YSize);
	BigUShort(&Header->ZSize);
	BigInt(&Header->PixMin);
	BigInt(&Header->PixMax);
	BigInt(&Header->Dummy1);
	BigInt(&Header->ColMap);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

/* Internal function to get the header and check it. */
ILboolean iIsValidSgi()
{
	iSgiHeader	Head;

	if (!iGetSgiHead(&Head))
		return IL_FALSE;
	iseek(-(ILint)sizeof(iSgiHeader), IL_SEEK_CUR);  // Go ahead and restore to previous state

	return iCheckSgi(&Head);
}

/*----------------------------------------------------------------------------*/

/* Internal function used to check if the HEADER is a valid .sgi header. */
ILboolean iCheckSgi(iSgiHeader *Header)
{
	if (Header->MagicNum != SGI_MAGICNUM)
		return IL_FALSE;
	if (Header->Storage != SGI_RLE && Header->Storage != SGI_VERBATIM)
		return IL_FALSE;
	if (Header->Bpc == 0 || Header->Dim == 0)
		return IL_FALSE;
	if (Header->XSize == 0 || Header->YSize == 0 || Header->ZSize == 0)
		return IL_FALSE;

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

/*! Reads a SGI file */
ILboolean ilLoadSgi(const ILstring FileName)
{
	ILHANDLE	SgiFile;
	ILboolean	bSgi = IL_FALSE;

	SgiFile = iopenr(FileName);
	if (SgiFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bSgi;
	}

	bSgi = ilLoadSgiF(SgiFile);
	icloser(SgiFile);

	return bSgi;
}

/*----------------------------------------------------------------------------*/

/*! Reads an already-opened SGI file */
ILboolean ilLoadSgiF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadSgiInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}

/*----------------------------------------------------------------------------*/

/*! Reads from a memory "lump" that contains a SGI image */
ILboolean ilLoadSgiL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadSgiInternal();
}

/*----------------------------------------------------------------------------*/

/* Internal function used to load the SGI image */
ILboolean iLoadSgiInternal()
{
	iSgiHeader	Header;
	ILboolean	bSgi;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetSgiHead(&Header))
		return IL_FALSE;
	if (!iCheckSgi(&Header)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	
	if (Header.Storage == SGI_RLE) {  // RLE
		bSgi = iReadRleSgi(&Header);
	}
	else {  // Non-RLE  //(Header.Storage == SGI_VERBATIM)
		bSgi = iReadNonRleSgi(&Header);
	}

	ilFixImage();

	return bSgi;
}

/*----------------------------------------------------------------------------*/

ILboolean iReadRleSgi(iSgiHeader *Head)
{       
        #ifdef __LITTLE_ENDIAN__
        ILuint ixTable;
        #endif
        ILuint 		ChanInt = 0;
        ILuint  	ixPlane, ixHeight,ixPixel, RleOff, RleLen;
	ILuint		*OffTable=NULL, *LenTable=NULL, TableSize, Cur;
	ILubyte		**TempData=NULL;

	if (!iNewSgi(Head))
		return IL_FALSE;

	TableSize = Head->YSize * Head->ZSize;
	OffTable = (ILuint*)ialloc(TableSize * sizeof(ILuint));
	LenTable = (ILuint*)ialloc(TableSize * sizeof(ILuint));
	if (OffTable == NULL || LenTable == NULL)
		goto cleanup_error;
	if (iread(OffTable, TableSize * sizeof(ILuint), 1) != 1)
		goto cleanup_error;
	if (iread(LenTable, TableSize * sizeof(ILuint), 1) != 1)
		goto cleanup_error;

#ifdef __LITTLE_ENDIAN__
	// Fix the offset/len table (it's big endian format)
	for (ixTable = 0; ixTable < TableSize; ixTable++) {
		*(OffTable + ixTable) = SwapInt(*(OffTable + ixTable));
		*(LenTable + ixTable) = SwapInt(*(LenTable + ixTable));
	}
#endif //__LITTLE_ENDIAN__

	// We have to create a temporary buffer for the image, because SGI
	//	images are plane-separated. */
	TempData = (ILubyte**)ialloc(Head->ZSize * sizeof(ILubyte*));
	if (TempData == NULL)
		goto cleanup_error;
	imemclear(TempData, Head->ZSize * sizeof(ILubyte*));  // Just in case ialloc fails then cleanup_error.
        for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		TempData[ixPlane] = (ILubyte*)ialloc(Head->XSize * Head->YSize * Head->Bpc);
		if (TempData[ixPlane] == NULL)
			goto cleanup_error;
	}

	// Read the Planes into the temporary memory
	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		for (ixHeight = 0, Cur = 0;	ixHeight < Head->YSize;
			ixHeight++, Cur += Head->XSize * Head->Bpc) {

			RleOff = OffTable[ixHeight + ixPlane * Head->YSize];
			RleLen = LenTable[ixHeight + ixPlane * Head->YSize];
			
			// Seeks to the offset table position
			iseek(RleOff, IL_SEEK_SET);
			if (iGetScanLine((TempData[ixPlane]) + (ixHeight * Head->XSize * Head->Bpc),
				Head, RleLen) != Head->XSize * Head->Bpc) {
					ilSetError(IL_ILLEGAL_FILE_VALUE);
					goto cleanup_error;
			}
		}
	}

	// DW: Removed on 05/25/2002.
	/*// Check if an alphaplane exists and invert it
	if (Head->ZSize == 4) {
		for (ixPixel=0; (ILint)ixPixel<Head->XSize * Head->YSize; ixPixel++) {
 			TempData[3][ixPixel] = TempData[3][ixPixel] ^ 255;
 		}	
	}*/
	
	// Assemble the image from its planes
	for (ixPixel = 0; ixPixel < iCurImage->SizeOfData;
		ixPixel += Head->ZSize * Head->Bpc, ChanInt += Head->Bpc) {
		for (ixPlane = 0; (ILint)ixPlane < Head->ZSize * Head->Bpc;	ixPlane += Head->Bpc) {
			iCurImage->Data[ixPixel + ixPlane] = TempData[ixPlane][ChanInt];
			if (Head->Bpc == 2)
				iCurImage->Data[ixPixel + ixPlane + 1] = TempData[ixPlane][ChanInt + 1];
		}
	}
        
        #ifdef __LITTLE_ENDIAN__
	if (Head->Bpc == 2)
		sgiSwitchData(iCurImage->Data, iCurImage->SizeOfData);
        #endif
        
	ifree(OffTable);
	ifree(LenTable);

	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		ifree(TempData[ixPlane]);
	}
	ifree(TempData);

	return IL_TRUE;

cleanup_error:
	ifree(OffTable);
	ifree(LenTable);
	if (TempData) {
		for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
			ifree(TempData[ixPlane]);
		}
		ifree(TempData);
	}

	return IL_FALSE;
}

/*----------------------------------------------------------------------------*/

ILint iGetScanLine(ILubyte *ScanLine, iSgiHeader *Head, ILuint Length)
{
	ILushort Pixel, Count;  // For current pixel
	ILuint	 BppRead = 0, CurPos = 0, Bps = Head->XSize * Head->Bpc;

	while (BppRead < Length && CurPos < Bps)
	{
		Pixel = 0;
		if (iread(&Pixel, Head->Bpc, 1) != 1)
			return -1;
		
#ifndef __LITTLE_ENDIAN__
		Pixel = SwapShort(Pixel);
#endif

		if (!(Count = (Pixel & 0x7f)))  // If 0, line ends
			return CurPos;
		if (Pixel & 0x80) {  // If top bit set, then it is a "run"
			if (iread(ScanLine, Head->Bpc, Count) != Count)
				return -1;
			BppRead += Head->Bpc * Count + Head->Bpc;
			ScanLine += Head->Bpc * Count;
			CurPos += Head->Bpc * Count;
		}
		else {
			if (iread(&Pixel, Head->Bpc, 1) != 1)
				return -1;
#ifndef __LITTLE_ENDIAN__
			Pixel = SwapShort(Pixel);
#endif
			if (Head->Bpc == 1) {
				while (Count--) {
					*ScanLine = (ILubyte)Pixel;
					ScanLine++;
					CurPos++;
				}
			}
			else {
				while (Count--) {
					*(ILushort*)ScanLine = Pixel;
					ScanLine += 2;
					CurPos += 2;
				}
			}
			BppRead += Head->Bpc + Head->Bpc;
		}
	}

	return CurPos;
}


/*----------------------------------------------------------------------------*/

// Much easier to read - just assemble from planes, no decompression
ILboolean iReadNonRleSgi(iSgiHeader *Head)
{
	ILuint		i, c;
	// ILint		ChanInt = 0; Unused
        ILint 		ChanSize;
	ILboolean	Cache = IL_FALSE;

	if (!iNewSgi(Head)) {
		return IL_FALSE;
	}

	if (iGetHint(IL_MEM_SPEED_HINT) == IL_FASTEST) {
		Cache = IL_TRUE;
		ChanSize = Head->XSize * Head->YSize * Head->Bpc;
		iPreCache(ChanSize);
	}

	for (c = 0; c < iCurImage->Bpp; c++) {
		for (i = c; i < iCurImage->SizeOfData; i += iCurImage->Bpp) {
			if (iread(iCurImage->Data + i, 1, 1) != 1) {
				if (Cache)
					iUnCache();
				return IL_FALSE;
			}
		}
	}

	if (Cache)
		iUnCache();

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

ILvoid sgiSwitchData(ILubyte *Data, ILuint SizeOfData)
{	
	ILubyte	Temp;
	ILuint	i;
        #ifdef ALTIVEC
            i = 0;
            union {
                vector unsigned char vec;
                vector unsigned int load;
            }inversion_vector;
            
            inversion_vector.load  = (vector unsigned int)\
                {0x01000302,0x05040706,0x09080B0A,0x0D0C0F0E};
            while( i <= SizeOfData-16 ) {
                vector unsigned char data = vec_ld(i,Data);
                vec_perm(data,data,inversion_vector.vec);
                vec_st(data,i,Data);
                i+=16;
            }
            SizeOfData -= i;
        #endif
        for (i = 0; i < SizeOfData; i += 2) {
                Temp = Data[i];
                Data[i] = Data[i+1];
                Data[i+1] = Temp;
        }
        return;
}

/*----------------------------------------------------------------------------*/

// Just an internal convenience function for reading SGI files
ILboolean iNewSgi(iSgiHeader *Head)
{
	if (!ilTexImage(Head->XSize, Head->YSize, Head->Bpc, (ILubyte)Head->ZSize, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	switch (Head->ZSize)
	{
		case 1:
			iCurImage->Format = IL_LUMINANCE;
			break;
		case 2: 
                         iCurImage->Format = IL_LUMINANCE_ALPHA; 
                         break;
                case 3:
			iCurImage->Format = IL_RGB;
			break;
		case 4:
			iCurImage->Format = IL_RGBA;
			break;
		default:
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	switch (Head->Bpc)
	{
		case 1:
			if (Head->PixMin < 0)
				iCurImage->Type = IL_BYTE;
			else
				iCurImage->Type = IL_UNSIGNED_BYTE;
			break;
		case 2:
			if (Head->PixMin < 0)
				iCurImage->Type = IL_SHORT;
			else
				iCurImage->Type = IL_UNSIGNED_SHORT;
			break;
		default:
			ilSetError(IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

//! Writes a SGI file
ILboolean ilSaveSgi(const ILstring FileName)
{
	ILHANDLE	SgiFile;
	ILboolean	bSgi = IL_FALSE;

	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	SgiFile = iopenw(FileName);
	if (SgiFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bSgi;
	}

	bSgi = ilSaveSgiF(SgiFile);
	iclosew(SgiFile);

	return bSgi;
}

/*----------------------------------------------------------------------------*/

//! Writes a SGI to an already-opened file
ILboolean ilSaveSgiF(ILHANDLE File)
{
	iSetOutputFile(File);
	return iSaveSgiInternal();
}

/*----------------------------------------------------------------------------*/

//! Writes a SGI to a memory "lump"
ILboolean ilSaveSgiL(ILvoid *Lump, ILuint Size)
{
	iSetOutputLump(Lump, Size);
	return iSaveSgiInternal();
}


ILenum DetermineSgiType(ILenum Type)
{
	if (Type > IL_UNSIGNED_SHORT) {
		if (iCurImage->Type == IL_INT)
			return IL_SHORT;
		return IL_UNSIGNED_SHORT;
	}
	return Type;
}

/*----------------------------------------------------------------------------*/

// Rle does NOT work yet.

// Internal function used to save the Sgi.
ILboolean iSaveSgiInternal()
{
	ILuint		i, c;
	ILboolean	Compress;
	ILimage		*Temp = iCurImage;
	ILubyte		*TempData;

	Compress = iGetInt(IL_SGI_RLE);

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (iCurImage->Format != IL_RGB && iCurImage->Format != IL_RGBA) {
		if (iCurImage->Format == IL_BGRA)
			Temp = iConvertImage(iCurImage, IL_RGBA, DetermineSgiType(iCurImage->Type));
		else
			Temp = iConvertImage(iCurImage, IL_RGB, DetermineSgiType(iCurImage->Type));
	}
	else if (iCurImage->Type > IL_UNSIGNED_SHORT) {
		Temp = iConvertImage(iCurImage, iCurImage->Format, DetermineSgiType(iCurImage->Type));
	}

	if (Temp == NULL)
		return IL_FALSE;

	SaveBigUShort(SGI_MAGICNUM);  // 'Magic' number
	if (Compress)
		iputc(1);
	else
		iputc(0);

	if (Temp->Type == IL_UNSIGNED_BYTE)
		iputc(1);
	else if (Temp->Type == IL_UNSIGNED_SHORT)
		iputc(2);
	// Need to error here if not one of the two...

	if (Temp->Format == IL_LUMINANCE || Temp->Format == IL_COLOUR_INDEX)
		SaveBigUShort(2);
	else
		SaveBigUShort(3);

	SaveBigUShort((ILushort)Temp->Width);
	SaveBigUShort((ILushort)Temp->Height);
	SaveBigUShort((ILushort)Temp->Bpp);

	switch (Temp->Type)
	{
		case IL_BYTE:
			SaveBigInt(SCHAR_MIN);	// Minimum pixel value
			SaveBigInt(SCHAR_MAX);	// Maximum pixel value
			break;
		case IL_UNSIGNED_BYTE:
			SaveBigInt(0);			// Minimum pixel value
			SaveBigInt(UCHAR_MAX);	// Maximum pixel value
			break;
		case IL_SHORT:
			SaveBigInt(SHRT_MIN);	// Minimum pixel value
			SaveBigInt(SHRT_MAX);	// Maximum pixel value
			break;
		case IL_UNSIGNED_SHORT:
			SaveBigInt(0);			// Minimum pixel value
			SaveBigInt(USHRT_MAX);	// Maximum pixel value
			break;
	}

	SaveBigInt(0);  // Dummy value

	if (FName) {
		c = strlen(FName);
		c = c < 79 ? 79 : c;
		iwrite(FName, 1, c);
		c = 80 - c;
		for (i = 0; i < c; i++) {
			iputc(0);
		}
	}
	else {
		for (i = 0; i < 80; i++) {
			iputc(0);
		}
	}

	SaveBigUInt(0);  // Colormap

	// Padding
	for (i = 0; i < 101; i++) {
		SaveLittleInt(0);
	}


	if (iCurImage->Origin == IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(Temp);
		if (TempData == NULL) {
			if (Temp!= iCurImage)
				ilCloseImage(Temp);
			return IL_FALSE;
		}
	}
	else {
		TempData = Temp->Data;
	}


	if (!Compress) {
		for (c = 0; c < Temp->Bpp; c++) {
			for (i = c; i < Temp->SizeOfData; i += Temp->Bpp) {
				iputc(TempData[i]);  // Have to save each colour plane separately.
			}
		}
	}
	else {
		iSaveRleSgi(TempData);
	}


	if (TempData != Temp->Data)
		ifree(TempData);
	if (Temp != iCurImage)
		ilCloseImage(Temp);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

ILboolean iSaveRleSgi(ILubyte *Data)
{
	ILuint	c, i, y, j;
	ILubyte	*ScanLine = NULL, *CompLine = NULL;
	ILuint	*StartTable = NULL, *LenTable = NULL;
	ILuint	TableOff, DataOff = 0;


	ScanLine = (ILubyte*)ialloc(iCurImage->Width);
	CompLine = (ILubyte*)ialloc(iCurImage->Width * 2);  // Absolute worst case.
	StartTable = (ILuint*)ialloc(iCurImage->Height * iCurImage->Bpp * sizeof(ILuint));
	LenTable = (ILuint*)ialloc(iCurImage->Height * iCurImage->Bpp * sizeof(ILuint));
	if (!ScanLine || !StartTable || !LenTable) {
		ifree(ScanLine);
		ifree(CompLine);
		ifree(StartTable);
		ifree(LenTable);
		return IL_FALSE;
	}

	// These just contain dummy values at this point.
	TableOff = itellw();
	iwrite(StartTable, sizeof(ILuint), iCurImage->Height * iCurImage->Bpp);
	iwrite(LenTable, sizeof(ILuint), iCurImage->Height * iCurImage->Bpp);

	DataOff = itellw();
	for (c = 0; c < iCurImage->Bpp; c++) {
		for (y = 0; y < iCurImage->Height; y++) {
			i = y * iCurImage->Bps + c;
			for (j = 0; j < iCurImage->Width; j++, i += iCurImage->Bpp) {
				ScanLine[j] = Data[i];
			}

			ilRleCompressLine(ScanLine, iCurImage->Width, 1, CompLine, LenTable + iCurImage->Height * c + y, IL_SGICOMP);
			iwrite(CompLine, 1, *(LenTable + iCurImage->Height * c + y));
		}
	}

	iseek(TableOff, IL_SEEK_SET);

	j = iCurImage->Height * iCurImage->Bpp;
	for (y = 0; y < j; y++) {
		StartTable[y] = DataOff;
		StartTable[y] = SwapInt(StartTable[y]);
		DataOff += LenTable[y];
		LenTable[y] = SwapInt(LenTable[y]);
	}

	iwrite(StartTable, sizeof(ILuint), iCurImage->Height * iCurImage->Bpp);
	iwrite(LenTable, sizeof(ILuint), iCurImage->Height * iCurImage->Bpp);

	ifree(ScanLine);
	ifree(CompLine);
	ifree(StartTable);
	ifree(LenTable);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

#endif//IL_NO_SGI
