//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 06/13/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_tiff.c
//
// Description: Tiff (.tif) functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_TIF

#ifdef MACOSX
#include <libtiff/tiffio.h>
#else
#include <tiffio.h>
#endif

#include <time.h>
#include "il_manip.h"

#define MAGIC_HEADER1	0x4949
#define MAGIC_HEADER2	0x4D4D

/*----------------------------------------------------------------------------*/

// No need for a separate header
ILboolean	iLoadTiffInternal(ILvoid);
char*		iMakeString(ILvoid);
TIFF*		iTIFFOpen(char *Mode);
ILboolean	iSaveTiffInternal(char *Filename);

/*----------------------------------------------------------------------------*/

ILboolean ilisValidTiffExtension(const ILstring FileName)
{
	if (!iCheckExtension((ILstring)FileName, IL_TEXT("tif")) &&
		!iCheckExtension((ILstring)FileName, IL_TEXT("tiff")))
		return IL_FALSE;
	else
		return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

//! Checks if the file specified in FileName is a valid tiff file.
ILboolean ilIsValidTiff(const ILstring FileName)
{
	ILHANDLE	TiffFile;
	ILboolean	bTiff = IL_FALSE;

	if (!ilisValidTiffExtension((ILstring) FileName)) {
		ilSetError(IL_INVALID_EXTENSION);
		return bTiff;
	}

	TiffFile = iopenr((ILstring)FileName);
	if (TiffFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bTiff;
	}

	bTiff = ilIsValidTiffF(TiffFile);
	icloser(TiffFile);

	return bTiff;
}

/*----------------------------------------------------------------------------*/

ILboolean ilisValidTiffFunc()
{
	ILushort Header1, Header2;

	Header1 = GetLittleUShort();
	
	if (Header1 != MAGIC_HEADER1 && Header1 != MAGIC_HEADER2)
		return IL_FALSE;

	if (Header1 == MAGIC_HEADER1)
		Header2 = GetLittleUShort();
	else
		Header2 = GetBigUShort();

	if (Header2 != 42)
		return IL_FALSE;

	return IL_TRUE;
	
}

/*----------------------------------------------------------------------------*/

//! Checks if the ILHANDLE contains a valid tiff file at the current position.
ILboolean ilIsValidTiffF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = ilisValidTiffFunc();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}

/*----------------------------------------------------------------------------*/

//! Checks if Lump is a valid Tiff lump.
ILboolean ilIsValidTiffL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return ilisValidTiffFunc();
}

/*----------------------------------------------------------------------------*/

//! Reads a Tiff file
ILboolean ilLoadTiff(const ILstring FileName)
{
	ILHANDLE	TiffFile;
	ILboolean	bTiff = IL_FALSE;

	TiffFile = iopenr(FileName);
	if (TiffFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
	}
	else {
		bTiff = ilLoadTiffF(TiffFile);
		icloser(TiffFile);
	}
	
	return bTiff;
}

/*----------------------------------------------------------------------------*/

//! Reads an already-opened Tiff file
ILboolean ilLoadTiffF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadTiffInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}

/*----------------------------------------------------------------------------*/

//! Reads from a memory "lump" that contains a Tiff
ILboolean ilLoadTiffL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadTiffInternal();
}

/*----------------------------------------------------------------------------*/

void warningHandler(const char* mod, const char* fmt, va_list ap)
{
	char buff[1024];
	_vsnprintf(buff, 1024, fmt, ap);
}

void errorHandler(const char* mod, const char* fmt, va_list ap)
{
	char buff[1024];
	_vsnprintf(buff, 1024, fmt, ap);
}

// Internal function used to load the Tiff.
ILboolean iLoadTiffInternal()
{
	TIFF		*tif;
	uint16	w, h, d, photometric, planarconfig;
	uint16	samplesperpixel, bitspersample, *sampleinfo, extrasamples;
	ILubyte		*pImageData;
	ILuint		i, ProfileLen, DirCount = 0;
	ILvoid		*Buffer;
	ILimage		*Image;
	ILushort	si;
//TIFFRGBAImage img;
//char emsg[1024];


	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	TIFFSetWarningHandler(NULL);
	TIFFSetErrorHandler(NULL);

	//for debugging only
	//TIFFSetWarningHandler(warningHandler);
	//TIFFSetErrorHandler(errorHandler);

		tif = iTIFFOpen("r");
		if (tif == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	do {
		DirCount++;
	} while (TIFFReadDirectory(tif));

/*
	if (!ilTexImage(1, 1, 1, 1, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
		TIFFClose(tif);
		return IL_FALSE;
	}
	Image = iCurImage;
	for (i = 1; i < DirCount; i++) {
		Image->Next = ilNewImage(1, 1, 1, 1, 1);
		if (Image->Next == NULL) {
			TIFFClose(tif);
			return IL_FALSE;
		}
		Image = Image->Next;
	}
	iCurImage->NumNext = DirCount - 1;
*/
	Image = NULL;
	for (i = 0; i < DirCount; i++) {
		TIFFSetDirectory(tif, (tdir_t)i);
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
		TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGEDEPTH, &d);
		TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
		TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &bitspersample);
		TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES, &extrasamples, &sampleinfo);

		//added 2003-08-31
		//1 bpp tiffs are not neccessarily greyscale, they can
		//have a palette (photometric == 3)...get this information
		TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC, &photometric);
		TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planarconfig);

		if (samplesperpixel - extrasamples == 1) { //luminance or palette
			ILubyte* strip;
			tsize_t stripsize;
			ILuint y;
			uint32 rowsperstrip;

			if(!Image) {
				if(!ilTexImage(w, h, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL)) {
					TIFFClose(tif);
					return IL_FALSE;
				}
				iCurImage->NumNext = 0;
				Image = iCurImage;
			}
			else {
				Image->Next = ilNewImage(w, h, 1, 1, 1);
				if(Image->Next == NULL) {
					TIFFClose(tif);
					return IL_FALSE;
				}
				Image = Image->Next;
				iCurImage->NumNext++;
			}

			TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
			stripsize = TIFFStripSize(tif);

			strip = ialloc(stripsize);

			for(y = 0; y < h; y += rowsperstrip) {
				//if(y + rowsperstrip > h)
				//	stripsize = (stripsize*(h - y))/rowsperstrip;
				if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), strip, stripsize) == -1) {
					ilSetError(IL_LIB_TIFF_ERROR);
					ifree(strip);
					TIFFClose(tif);
					return IL_FALSE;
				}
			}

			ifree(strip);
		}
		else {//rgb or rgba

			if(!Image) {
				if(!ilTexImage(w, h, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
					TIFFClose(tif);
					return IL_FALSE;
				}
				iCurImage->NumNext = 0;
				Image = iCurImage;
			}
			else {
				Image->Next = ilNewImage(w, h, 1, 4, 1);
				if(Image->Next == NULL) {
					TIFFClose(tif);
					return IL_FALSE;
				}
				Image = Image->Next;
				iCurImage->NumNext++;
			}

			if (samplesperpixel == 4) {
				TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES, &extrasamples, &sampleinfo);
				if (!sampleinfo || sampleinfo[0] == EXTRASAMPLE_UNSPECIFIED) {
					si = EXTRASAMPLE_ASSOCALPHA;
					TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, &si);
				}
			}
/*
			if (!ilResizeImage(Image, Image->Width, Image->Height, 1, 4, 1)) {
				TIFFClose(tif);
				return IL_FALSE;
			}*/
			Image->Format = IL_RGBA;
			Image->Type = IL_UNSIGNED_BYTE;

			// Siigron: added u_long cast to shut up compiler warning
			//2003-08-31: changed flag from 1 (exit on error) to 0 (keep decoding)
			//this lets me view text.tif, but can give crashes with unsupported
			//tiffs...
			//2003-09-04: keep flag 1 for official version for now
			if (!TIFFReadRGBAImage(tif, Image->Width, Image->Height, (uint32*)Image->Data, 1)) {
				TIFFClose(tif);
				ilSetError(IL_LIB_TIFF_ERROR);
				return IL_FALSE;
			}
		} //else rgb or rgba

		if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &ProfileLen, &Buffer)) {
			if (Image->Profile && Image->ProfileSize)
				ifree(Image->Profile);
			Image->Profile = (ILubyte*)ialloc(ProfileLen);
			if (Image->Profile == NULL) {
				TIFFClose(tif);
				return IL_FALSE;
			}

			memcpy(Image->Profile, Buffer, ProfileLen);
			Image->ProfileSize = ProfileLen;

			//removed on 2003-08-24 as explained in bug 579574 on sourceforge
			//_TIFFfree(Buffer);
		}

		Image->Origin = IL_ORIGIN_LOWER_LEFT;  // eiu...dunno if this is right
/*
		Image = Image->Next;
		if (Image == NULL)  // Should never happen except when we reach the end, but check anyway.
			break;*/
	} //for tiff directories

	//TODO: put switch into the loop??
	switch (samplesperpixel)
	{
		case 1:
			//added 2003-08-31 to keep palettized tiffs colored
			/*
			if(photometric != 3)
				ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
			else //strip alpha as tiff supports no alpha palettes
				ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);*/
			break;

		case 3:
			//TODO: why the ifdef??
#ifdef __LITTLE_ENDIAN__
			ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
#endif			
			break; 

		case 4:
			pImageData = iCurImage->Data;
//removed on 2003-08-26...why was this here? libtiff should and does
//take care of these things???
/*			
			//invert alpha
			#ifdef __LITTLE_ENDIAN__
			pImageData += 3;
#endif			
			for (i = iCurImage->Width * iCurImage->Height; i > 0; i--) {
				*pImageData ^= 255;
				pImageData += 4;
			}
*/
			break;
	}

	TIFFClose(tif);

	ilFixImage();

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

/////////////////////////////////////////////////////////////////////////////////////////
// Extension to load tiff files from memory
// Marco Fabbricatore (fabbrica@ai-lab.fh-furtwangen.de)
/////////////////////////////////////////////////////////////////////////////////////////

static tsize_t
_tiffFileReadProc(thandle_t fd, tdata_t pData, tsize_t tSize)
{
	return iread(pData, 1, tSize);
}

/*----------------------------------------------------------------------------*/

static tsize_t
_tiffFileWriteProc(thandle_t fd, tdata_t pData, tsize_t tSize)
{
	/*TIFFWarning("TIFFMemFile", "_tiffFileWriteProc() Not implemented");
	return(0);*/
	return iwrite(pData, 1, tSize);
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSeekProc(thandle_t fd, toff_t tOff, int whence)
{
	/* we use this as a special code, so avoid accepting it */
	if (tOff == 0xFFFFFFFF)
		return 0xFFFFFFFF;

	iseek(tOff, whence);
	return tOff;
}

/*----------------------------------------------------------------------------*/

static int
_tiffFileCloseProc(thandle_t fd)
{
   	return (0);
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSizeProc(thandle_t fd)
{
	ILint Offset, Size;
	Offset = itell();
	iseek(0, IL_SEEK_END);
	Size = itell();
	iseek(Offset, IL_SEEK_SET);

	return Size;
}

/*----------------------------------------------------------------------------*/

#ifdef __BORLANDC__
#pragma argsused
#endif
static int
_tiffDummyMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	return 0;
}

/*----------------------------------------------------------------------------*/

#ifdef __BORLANDC__
#pragma argsused
#endif
static void
_tiffDummyUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
	return;
}

/*----------------------------------------------------------------------------*/

TIFF *iTIFFOpen(char *Mode)
{
	TIFF *tif;

	tif = TIFFClientOpen("TIFFMemFile", Mode,
						NULL,
						_tiffFileReadProc, _tiffFileWriteProc,
						_tiffFileSeekProc, _tiffFileCloseProc,
						_tiffFileSizeProc, _tiffDummyMapProc,
						_tiffDummyUnmapProc);

	return tif;
}

/*----------------------------------------------------------------------------*/


//! Writes a Tiff file
ILboolean ilSaveTiff(const ILstring FileName)
{
	ILHANDLE	TiffFile;
	ILboolean	bTiff = IL_FALSE;

	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	TiffFile = iopenw(FileName);
	if (TiffFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bTiff;
	}

	//bTiff = ilSaveTiffF(TiffFile);
	bTiff = iSaveTiffInternal(FileName);
	iclosew(TiffFile);

	return bTiff;
}


//! Writes a Tiff to an already-opened file
ILboolean ilSaveTiffF(ILHANDLE File)
{
//	iSetOutputFile(File);
//	return iSaveTiffInternal();
	ilSetError(IL_FILE_READ_ERROR);
	return IL_FALSE;
}


//! Writes a Tiff to a memory "lump"
ILboolean ilSaveTiffL(ILvoid *Lump, ILuint Size)
{
//	iSetOutputLump(Lump, Size);
//	return iSaveTiffInternal();
	ilSetError(IL_FILE_READ_ERROR);
	return IL_FALSE;
}


// @TODO:  Accept palettes!

// Internal function used to save the Tiff.
ILboolean iSaveTiffInternal(char *Filename)
{
	ILenum	Format;
	ILenum	Compression;
	ILuint	ixLine;
	TIFF	*File;
	char	Description[512];
	ILimage	*TempImage;

	if(iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}


	if (iGetHint(IL_COMPRESSION_HINT) == IL_USE_COMPRESSION)
		Compression = COMPRESSION_PACKBITS;
	else
		Compression = COMPRESSION_NONE;

	if (iCurImage->Format == IL_COLOUR_INDEX) {
		if (ilGetBppPal(iCurImage->Pal.PalType) == 4)  // Preserve the alpha.
			TempImage = iConvertImage(iCurImage, IL_RGBA, IL_UNSIGNED_BYTE);
		else
			TempImage = iConvertImage(iCurImage, IL_RGB, IL_UNSIGNED_BYTE);

		if (TempImage == NULL) {
			return IL_FALSE;
		}
	}
	else {
		TempImage = iCurImage;
	}
	
	File = TIFFOpen(Filename, "w");
	//File = iTIFFOpen("w");
	if (File == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	sprintf(Description, "Tiff generated by %s", ilGetString(IL_VERSION_NUM));

	TIFFSetField(File, TIFFTAG_IMAGEWIDTH, TempImage->Width);
	TIFFSetField(File, TIFFTAG_IMAGELENGTH, TempImage->Height);
	TIFFSetField(File, TIFFTAG_COMPRESSION, Compression);
	TIFFSetField(File, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField(File, TIFFTAG_BITSPERSAMPLE, TempImage->Bpc << 3);
	TIFFSetField(File, TIFFTAG_SAMPLESPERPIXEL, TempImage->Bpp);
	TIFFSetField(File, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(File, TIFFTAG_ROWSPERSTRIP, 1);
	TIFFSetField(File, TIFFTAG_SOFTWARE, ilGetString(IL_VERSION_NUM));
	/*TIFFSetField(File, TIFFTAG_DOCUMENTNAME,
						iGetString(IL_TIF_DOCUMENTNAME_STRING) ?
						iGetString(IL_TIF_DOCUMENTNAME_STRING) : FileName);*/
	if (iGetString(IL_TIF_DOCUMENTNAME_STRING))
		TIFFSetField(File, TIFFTAG_DOCUMENTNAME, iGetString(IL_TIF_DOCUMENTNAME_STRING));
	if (iGetString(IL_TIF_AUTHNAME_STRING))
		TIFFSetField(File, TIFFTAG_ARTIST, iGetString(IL_TIF_AUTHNAME_STRING));
	if (iGetString(IL_TIF_HOSTCOMPUTER_STRING))
		TIFFSetField(File, TIFFTAG_HOSTCOMPUTER,
					iGetString(IL_TIF_HOSTCOMPUTER_STRING));
	if (iGetString(IL_TIF_DESCRIPTION_STRING))
		TIFFSetField(File, TIFFTAG_IMAGEDESCRIPTION,
					iGetString(IL_TIF_DESCRIPTION_STRING));
	TIFFSetField(File, TIFFTAG_DATETIME, iMakeString());


        // 24/4/2003
        // Orientation flag is not always supported ( Photoshop, ...), orient the image data 
        // and set it always to normal view
        TIFFSetField(File, TIFFTAG_ORIENTATION,ORIENTATION_TOPLEFT );
        if( TempImage->Origin != IL_ORIGIN_UPPER_LEFT ) {
            ILubyte *Data = iGetFlipped(TempImage);
            ifree( (void*)TempImage->Data );
            TempImage->Data = Data;
        }
        
        /*
	TIFFSetField(File, TIFFTAG_ORIENTATION,
		TempImage->Origin == IL_ORIGIN_UPPER_LEFT ? ORIENTATION_TOPLEFT : ORIENTATION_BOTLEFT);
        */
        
	Format = TempImage->Format;
	if (Format == IL_BGR || Format == IL_BGRA)
		ilSwapColours();

	for (ixLine = 0; ixLine < TempImage->Height; ++ixLine) {
		if (TIFFWriteScanline(File, TempImage->Data + ixLine * TempImage->Bps, ixLine, 0) < 0) {
			TIFFClose(File);
			ilSetError(IL_LIB_TIFF_ERROR);
			return IL_FALSE;
		}
	}

	if (Format == IL_BGR || Format == IL_BGRA)
		ilSwapColours();

	if (TempImage != iCurImage)
		ilCloseImage(TempImage);

	TIFFClose(File);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/
// Makes a neat date string for the date field.
char *iMakeString()
{
	static char TimeStr[255];
	time_t		Time;
	struct tm	*CurTime;
    
        imemclear(TimeStr, 255);
        
	time(&Time);
#ifdef _WIN32
	_tzset();
#endif
	CurTime = localtime(&Time);

	strftime(TimeStr, 255, "%s (%z)", CurTime); // "%#c (%z)"	// %s was %c

	return TimeStr;
}

/*----------------------------------------------------------------------------*/

#endif//IL_NO_TIF
