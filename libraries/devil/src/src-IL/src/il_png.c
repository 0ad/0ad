//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 02/01/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_png.c
//
// Description: Portable network graphics file (.png) functions
//
// 20040223 XIX : now may spit out pngs with a transparent index, this is mostly a hack
// but the proper way of doing it would be to change the pal stuff to think in argb rather than rgb
// which is something of a bigger job.
//
//-----------------------------------------------------------------------------

// Most of the comments are left in this file from libpng's excellent example.c

#include "il_internal.h"
#ifndef IL_NO_PNG
#ifdef MACOSX
#include <libpng/png.h>
#else
#include <png.h>
#endif
#include "il_manip.h"
#include <stdlib.h>
#if PNG_LIBPNG_VER < 10200
	#warning DevIL was designed with libpng 1.2.0 or higher in mind.  Consider upgrading at www.libpng.org
#endif

ILboolean	iIsValidPng(ILvoid);
ILboolean	iLoadPngInternal(ILvoid);
ILboolean	iSavePngInternal(ILvoid);
ILvoid		pngSwitchData(ILubyte *Data, ILuint SizeOfData, ILubyte Bpc);

ILint		readpng_init(ILvoid);
ILboolean	readpng_get_image(ILdouble display_exponent);
ILvoid		readpng_cleanup(ILvoid);

png_structp png_ptr = NULL;
png_infop info_ptr = NULL;
ILuint color_type;

#define GAMMA_CORRECTION 1.0  // Doesn't seem to be doing anything...


ILboolean ilIsValidPng(const ILstring FileName)
{
	ILHANDLE	PngFile;
	ILboolean	bPng = IL_FALSE;

	if (!iCheckExtension(FileName, "png")) {
		ilSetError(IL_INVALID_EXTENSION);
		return bPng;
	}

	PngFile = iopenr(FileName);
	if (PngFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPng;
	}

	bPng = ilIsValidPngF(PngFile);
	icloser(PngFile);

	return bPng;
}


ILboolean ilIsValidPngF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iIsValidPng();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


ILboolean ilIsValidPngL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iIsValidPng();
}


ILboolean iIsValidPng()
{
	ILubyte 	Signature[8];
	ILint		Read;

	Read = iread(Signature, 1, 8);
	iseek(-Read, IL_SEEK_CUR);

	return png_check_sig(Signature, 8);
}


// Reads a file
ILboolean ilLoadPng(const ILstring FileName)
{
	ILHANDLE	PngFile;
	ILboolean	bPng = IL_FALSE;

	PngFile = iopenr(FileName);
	if (PngFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPng;
	}

	bPng = ilLoadPngF(PngFile);
	icloser(PngFile);

	return bPng;
}


// Reads an already-opened file
ILboolean ilLoadPngF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadPngInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


// Reads from a memory "lump"
ILboolean ilLoadPngL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadPngInternal();
}


ILboolean iLoadPngInternal()
{
	png_ptr = NULL;
	info_ptr = NULL;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	if (!iIsValidPng()) {
		ilSetError(IL_INVALID_VALUE);
		return IL_FALSE;
	}

	if (readpng_init())
		return IL_FALSE;
	if (!readpng_get_image(GAMMA_CORRECTION))
		return IL_FALSE;

	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;  // correct?

	// @TODO:  Reimplement!
	/*if (png_ptr->num_palette > 0) {
		iCurImage->Pal.PalSize = png_ptr->num_palette * 3;	// just a guess...
		iCurImage->Pal.PalType = IL_PAL_RGB24;	// just another guess...
		iCurImage->Pal.Palette = (ILubyte*)ialloc(png_ptr->num_palette * 3);
		if (iCurImage->Pal.Palette == NULL) {
			return IL_FALSE;
		}
		memcpy(iCurImage->Pal.Palette, png_ptr->palette, png_ptr->num_palette * 3);
	}*/

	switch (iCurImage->Bpp)
	{
		case 1: // @TODO:	FIX THIS! Will never happen right now, because
				//paletted images are expanded to rgb at the moment...
			iCurImage->Format = IL_COLOUR_INDEX;
			break;
		case 2: //this has to be gray + alpha, since palette images were
				//expanded to rgb earlier (added 20040224)
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

	if (color_type == PNG_COLOR_TYPE_GRAY)
		iCurImage->Format = IL_LUMINANCE;
	
	readpng_cleanup();

	pngSwitchData(iCurImage->Data, iCurImage->SizeOfData, iCurImage->Bpc);

	ilFixImage();

	return IL_TRUE;
}


static ILvoid png_read(png_structp png_ptr, png_bytep data, png_size_t length)
{
	(ILvoid)png_ptr;
	iread(data, 1, length);
	return;
}


static void png_error_func(png_structp png_ptr, png_const_charp message)
{
	ilSetError(IL_LIB_PNG_ERROR);

	/*
	  changed 20040224
	  From the libpng docs:
	  "Errors handled through png_error() are fatal, meaning that png_error()
	   should never return to its caller. Currently, this is handled via
	   setjmp() and longjmp()"
	*/
	//return;
	longjmp(png_jmpbuf(png_ptr), 1);
}

static void png_warn_func(png_structp png_ptr, png_const_charp message)
{
	return;
}

ILint readpng_init()
{
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_error_func, png_warn_func);
	if (!png_ptr)
		return 4;	/* out of memory */

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return 4;	/* out of memory */
	}


	/* we could create a second info struct here (end_info), but it's only
	 * useful if we want to keep pre- and post-IDAT chunk info separated
	 * (mainly for PNG-aware image editors and converters) */


	/* setjmp() must be called in every function that calls a PNG-reading
	 * libpng function */

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return 2;
	}


	png_set_read_fn(png_ptr, NULL, png_read);
	png_set_error_fn(png_ptr, NULL, png_error_func, png_warn_func);

//	png_set_sig_bytes(png_ptr, 8);	/* we already read the 8 signature bytes */

	png_read_info(png_ptr, info_ptr);  /* read all PNG info up to image data */


	/* alternatively, could make separate calls to png_get_image_width(),
	 * etc., but want bit_depth and color_type for later [don't care about
	 * compression_type and filter_type => NULLs] */

	/* OK, that's all we need for now; return happy */

	return 0;
}


/* display_exponent == LUT_exponent * CRT_exponent */

ILboolean readpng_get_image(ILdouble display_exponent)
{
	ILuint		i;
	png_bytepp	row_pointers = NULL;
	ILuint		width, height, channels;
	ILdouble	screen_gamma = 1.0, image_gamma;
	ILuint		bit_depth;


	/* setjmp() must be called in every function that calls a PNG-reading
	 * libpng function */

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return IL_FALSE;
	}

	png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)&width, (png_uint_32*)&height,
		&bit_depth, &color_type, NULL, NULL, NULL);

	// Expand palette images to RGB, low-bit-depth grayscale images to 8 bits,
	//	transparency chunks to full alpha channel; strip 16-bit-per-sample
	//	images to 8 bits per sample; and convert grayscale to RGB[A]
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_gray_1_2_4_to_8(png_ptr);
	}
	// Expand paletted colors into true RGB triplets
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	// Expand paletted or RGB images with transparency to full alpha channels
	//	so the data will be available as RGBA quartets.
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	//refresh information (added 20040224)
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)&width, (png_uint_32*)&height,
		&bit_depth, &color_type, NULL, NULL, NULL);

	if (bit_depth < 8)	// Expanded earlier.
		bit_depth = 8;

	// Perform gamma correction.
	// @TODO:  Determine if we should call png_set_gamma if image_gamma is 1.0.
#if _WIN32 || DJGPP
	screen_gamma = 2.2;
	if (png_get_gAMA(png_ptr, info_ptr, &image_gamma))
		png_set_gamma(png_ptr, screen_gamma, image_gamma);
#else
	screen_gamma = screen_gamma;
	image_gamma = image_gamma;
#endif

	if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}

	png_read_update_info(png_ptr, info_ptr);
	channels = (ILint)png_get_channels(png_ptr, info_ptr);
	//added 20040224: update color_type so that it has the correct value
	//in iLoadPngInternal (globals rule...)
	color_type = png_get_color_type(png_ptr, info_ptr);

	if (!ilTexImage(width, height, 1, (ILubyte)channels, 0, ilGetTypeBpc((ILubyte)(bit_depth >> 3)), NULL)) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	if ((row_pointers = (png_bytepp)ialloc(height * sizeof(png_bytep))) == NULL) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return IL_FALSE;
	}


	// Set the individual row_pointers to point at the correct offsets */

	for (i = 0; i < height; i++)
		row_pointers[i] = iCurImage->Data + i * iCurImage->Bps;


	// Now we can go ahead and just read the whole image
	png_read_image(png_ptr, row_pointers);


	/* and we're done!	(png_read_end() can be omitted if no processing of
	 * post-IDAT text/time/etc. is desired) */
	//png_read_end(png_ptr, NULL);
	ifree(row_pointers);

	return IL_TRUE;
}


ILvoid readpng_cleanup()
{
	if (png_ptr && info_ptr) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		png_ptr = NULL;
		info_ptr = NULL;
	}
}


ILvoid pngSwitchData(ILubyte *Data, ILuint SizeOfData, ILubyte Bpc)
{
#ifdef __LITTLE_ENDIAN__
	ILuint	Temp;
	ILuint	i;

	switch (Bpc)
	{
		case 2:
			for (i = 0; i < SizeOfData; i += 2) {
				Temp = Data[i];
				Data[i] = Data[i+1];
				Data[i+1] = Temp;
			}
			break;

		case 4:
			for (i = 0; i < SizeOfData; i += 4) {
				*((ILuint*)Data+i) = SwapInt(*((ILuint*)Data+i));
			}
			break;
	}

#endif
	return;
}


//! Writes a Png file
ILboolean ilSavePng(const ILstring FileName)
{
	ILHANDLE	PngFile;
	ILboolean	bPng = IL_FALSE;

	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	PngFile = iopenw(FileName);
	if (PngFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPng;
	}

	bPng = ilSavePngF(PngFile);
	iclosew(PngFile);

	return bPng;
}


//! Writes a Png to an already-opened file
ILboolean ilSavePngF(ILHANDLE File)
{
	iSetOutputFile(File);
	return iSavePngInternal();
}


//! Writes a Png to a memory "lump"
ILboolean ilSavePngL(ILvoid *Lump, ILuint Size)
{
	iSetOutputLump(Lump, Size);
	return iSavePngInternal();
}


ILvoid png_write(png_structp png_ptr, png_bytep data, png_size_t length)
{
	(ILvoid)png_ptr;
	iwrite(data, 1, length);
	return;
}

ILvoid flush_data(png_structp png_ptr)
{
	return;
}


// Internal function used to save the Png.
ILboolean iSavePngInternal()
{
	png_structp png_ptr;
	png_infop	info_ptr;
	png_text	text[3];
	ILenum		PngType;
	ILuint		BitDepth, i, j;
	ILubyte 	**RowPtr = NULL;
	ILimage 	*Temp = NULL;
	ILpal		*TempPal = NULL;

//XIX alpha
	ILubyte		transpart[1];
	ILint		trans;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	/* Create and initialize the png_struct with the desired error handler
	* functions.  If you want to use the default stderr and longjump method,
	* you can supply NULL for the last three parameters.  We also check that
	* the library version is compatible with the one used at compile time,
	* in case we are using dynamically linked libraries.  REQUIRED.
	*/
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, png_error_func, png_warn_func);
	if (png_ptr == NULL) {
		ilSetError(IL_LIB_PNG_ERROR);
		return IL_FALSE;
	}

	// Allocate/initialize the image information data.	REQUIRED
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		ilSetError(IL_LIB_PNG_ERROR);
		goto error_label;
	}

	/*// Set error handling.  REQUIRED if you aren't supplying your own
	//	error handling functions in the png_create_write_struct() call.
	if (setjmp(png_jmpbuf(png_ptr))) {
		// If we get here, we had a problem reading the file
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		ilSetError(IL_LIB_PNG_ERROR);
		return IL_FALSE;
	}*/

//	png_init_io(png_ptr, PngFile);
	png_set_write_fn(png_ptr, NULL, png_write, flush_data);

	switch (iCurImage->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			Temp = iCurImage;
			BitDepth = 8;
			break;
		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			Temp = iCurImage;
			BitDepth = 16;
			break;
		case IL_INT:
		case IL_UNSIGNED_INT:
			Temp = iConvertImage(iCurImage, iCurImage->Format, IL_UNSIGNED_SHORT);
			if (Temp == NULL) {
				png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
				return IL_FALSE;
			}
			BitDepth = 16;
			break;
		default:
			ilSetError(IL_INTERNAL_ERROR);
			goto error_label;
	}

	switch (iCurImage->Format)
	{
		case IL_COLOUR_INDEX:
			PngType = PNG_COLOR_TYPE_PALETTE;
			break;
		case IL_LUMINANCE:
			PngType = PNG_COLOR_TYPE_GRAY;
			break;
		case IL_RGB:
		case IL_BGR:
			PngType = PNG_COLOR_TYPE_RGB;
			break;
		case IL_RGBA:
		case IL_BGRA:
			PngType = PNG_COLOR_TYPE_RGB_ALPHA;
			break;
		default:
			ilSetError(IL_INTERNAL_ERROR);
			goto error_label;
	}

	// Set the image information here.	Width and height are up to 2^31,
	//	bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
	//	the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
	//	PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
	//	or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
	//	PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
	//	currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
	if (iGetInt(IL_PNG_INTERLACE) == IL_TRUE) {
		png_set_IHDR(png_ptr, info_ptr, iCurImage->Width, iCurImage->Height, BitDepth, PngType,
			PNG_INTERLACE_ADAM7, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	}
	else {
		png_set_IHDR(png_ptr, info_ptr, iCurImage->Width, iCurImage->Height, BitDepth, PngType,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	}

	if (iCurImage->Format == IL_COLOUR_INDEX) {
		// set the palette if there is one.  REQUIRED for indexed-color images.
		TempPal = iConvertPal(&iCurImage->Pal, IL_PAL_RGB24);
		png_set_PLTE(png_ptr, info_ptr, (png_colorp)TempPal->Palette,
			ilGetInteger(IL_PALETTE_NUM_COLS));

//XIX alpha
		trans=iGetInt(IL_PNG_ALPHA_INDEX);
		if ( trans>=0)
		{
			transpart[0]=(ILubyte)trans;
			png_set_tRNS(png_ptr, info_ptr, transpart, 1, 0);
		}
	}

	/*
	// optional significant bit chunk
	// if we are dealing with a grayscale image then 
	sig_bit.gray = true_bit_depth;
	// otherwise, if we are dealing with a color image then
	sig_bit.red = true_red_bit_depth;
	sig_bit.green = true_green_bit_depth;
	sig_bit.blue = true_blue_bit_depth;
	// if the image has an alpha channel then
	sig_bit.alpha = true_alpha_bit_depth;
	png_set_sBIT(png_ptr, info_ptr, sig_bit);*/


	/* Optional gamma chunk is strongly suggested if you have any guess
	* as to the correct gamma of the image.
	*/
	//png_set_gAMA(png_ptr, info_ptr, gamma);

	// Optionally write comments into the image.
	imemclear(text, sizeof(png_text) * 3);
	text[0].key = "Generated by";
	text[0].text = "Generated by the Developer's Image Library (DevIL)";
	text[0].compression = PNG_TEXT_COMPRESSION_NONE;
	text[1].key = "Author's name";
	text[1].text = iGetString(IL_PNG_AUTHNAME_STRING);
	text[1].compression = PNG_TEXT_COMPRESSION_NONE;
	text[2].key = "Author's comments";
	text[2].text = iGetString(IL_PNG_AUTHNAME_STRING);
	text[2].compression = PNG_TEXT_COMPRESSION_NONE;
	png_set_text(png_ptr, info_ptr, text, 3);

	// Write the file header information.  REQUIRED.
	png_write_info(png_ptr, info_ptr);

	// Free up our user-defined text.
	if (text[1].text)
		ifree(text[1].text);
	if (text[2].text)
		ifree(text[2].text);

	/* Shift the pixels up to a legal bit depth and fill in
	* as appropriate to correctly scale the image.
	*/
	//png_set_shift(png_ptr, &sig_bit);

	/* pack pixels into bytes */
	//png_set_packing(png_ptr);

	// swap location of alpha bytes from ARGB to RGBA
	//png_set_swap_alpha(png_ptr);

	// flip BGR pixels to RGB
	if (iCurImage->Format == IL_BGR || iCurImage->Format == IL_BGRA)
		png_set_bgr(png_ptr);

	// swap bytes of 16-bit files to most significant byte first
	#ifdef	__LITTLE_ENDIAN__
	png_set_swap(png_ptr);
	#endif//__LITTLE_ENDIAN__

	RowPtr = (ILubyte**)ialloc(iCurImage->Height * sizeof(ILubyte*));
	if (RowPtr == NULL)
		goto error_label;
	if (iCurImage->Origin == IL_ORIGIN_UPPER_LEFT) {
		for (i = 0; i < iCurImage->Height; i++) {
			RowPtr[i] = Temp->Data + i * Temp->Bps;
		}
	}
	else {
		j = iCurImage->Height - 1;
		for (i = 0; i < iCurImage->Height; i++, j--) {
			RowPtr[i] = Temp->Data + j * Temp->Bps;
		}
	}

	// Writes the image.
	png_write_image(png_ptr, RowPtr);

	// It is REQUIRED to call this to finish writing the rest of the file
	png_write_end(png_ptr, info_ptr);

	// clean up after the write, and ifree any memory allocated
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

	ifree(RowPtr);

	if (Temp != iCurImage)
		ilCloseImage(Temp);
	ilClosePal(TempPal);

	return IL_TRUE;

error_label:
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	ifree(RowPtr);
	if (Temp != iCurImage)
		ilCloseImage(Temp);
	ilClosePal(TempPal);
	return IL_FALSE;
}


#endif//IL_NO_PNG
