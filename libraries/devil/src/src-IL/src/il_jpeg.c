//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/26/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_jpeg.c
//
// Description: Jpeg (.jpg) functions
//
//-----------------------------------------------------------------------------


//
// Most of the comments here are sufficient, as we're just using libjpeg.
//  I have left most of the libjpeg example's comments intact, though.
//

#include "il_internal.h"
#ifndef IL_NO_JPG
	#ifndef IL_USE_IJL
		#ifdef RGB_RED
			#undef RGB_RED
			#undef RGB_GREEN
			#undef RGB_BLUE
		#endif
		#define RGB_RED		0
		#define RGB_GREEN	1
		#define RGB_BLUE	2

		#ifdef MACOSX
			#include <libjpeg/jpeglib.h>
		#else
			#include "jpeglib.h"
		#endif

		#if JPEG_LIB_VERSION < 62
			#warning DevIL was designed with libjpeg 6b or higher in mind.  Consider upgrading at www.ijg.org
		#endif
	#else
		#include <ijl.h>
		#include <limits.h>
	#endif
#include "il_jpeg.h"
#include "il_manip.h"
#include <setjmp.h>

static ILboolean jpgErrorOccured = IL_FALSE;


// Internal function used to get the .jpg header from the current file.
ILvoid iGetJpgHead(ILubyte *Header)
{
	Header[0] = igetc();
	Header[1] = igetc();
	return;
}


// Internal function used to check if the HEADER is a valid .Jpg header.
ILboolean iCheckJpg(ILubyte Header[2])
{
	if (Header[0] != 0xFF || Header[1] != 0xD8)
		return IL_FALSE;
	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidJpg()
{
	ILubyte Head[2];

	iGetJpgHead(Head);
	iseek(-2, IL_SEEK_CUR);  // Go ahead and restore to previous state

	return iCheckJpg(Head);
}


//! Checks if the file specified in FileName is a valid .jpg file.
ILboolean ilIsValidJpg(const ILstring FileName)
{
	ILHANDLE	JpegFile;
	ILboolean	bJpeg = IL_FALSE;

	if (!iCheckExtension(FileName, "jpg") &&
		!iCheckExtension(FileName, "jpe") &&
		!iCheckExtension(FileName, "jpeg")) {
		ilSetError(IL_INVALID_EXTENSION);
		return bJpeg;
	}

	JpegFile = iopenr(FileName);
	if (JpegFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bJpeg;
	}

	bJpeg = ilIsValidJpgF(JpegFile);
	icloser(JpegFile);

	return bJpeg;
}


//! Checks if the ILHANDLE contains a valid .jpg file at the current position.
ILboolean ilIsValidJpgF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iIsValidJpg();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


ILboolean ilIsValidJpgL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iIsValidJpg();
}


#ifndef IL_USE_IJL // Use libjpeg instead of the IJL.

// Overrides libjpeg's stupid error/warning handlers. =P
void ExitErrorHandle (struct jpeg_common_struct *JpegInfo)
{
	ilSetError(IL_LIB_JPEG_ERROR);
	jpgErrorOccured = IL_TRUE;
	return;
}
void OutputMsg(struct jpeg_common_struct *JpegInfo)
{
	return;
}


//! Reads a jpeg file
ILboolean ilLoadJpeg(const ILstring FileName)
{
	ILHANDLE	JpegFile;
	ILboolean	bJpeg = IL_FALSE;

	JpegFile = iopenr(FileName);
	if (JpegFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bJpeg;
	}

	bJpeg = ilLoadJpegF(JpegFile);
	icloser(JpegFile);

	return bJpeg;
}


//! Reads an already-opened jpeg file
ILboolean ilLoadJpegF(ILHANDLE File)
{
	ILboolean	bRet;
	ILuint		FirstPos;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadJpegInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


// Reads from a memory "lump" containing a jpeg
ILboolean ilLoadJpegL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadJpegInternal();
}


typedef struct {
  struct jpeg_source_mgr pub;	/* public fields */

  JOCTET * buffer;		/* start of buffer */
  boolean start_of_file;	/* have we gotten any data yet? */
} iread_mgr;

typedef iread_mgr * iread_ptr;

#define INPUT_BUF_SIZE  4096  // choose an efficiently iread'able size


METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
	iread_ptr src = (iread_ptr) cinfo->src;
	src->start_of_file = TRUE;
}


METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
	iread_ptr src = (iread_ptr) cinfo->src;
	ILint nbytes;

	nbytes = iread(src->buffer, 1, INPUT_BUF_SIZE);

	if (nbytes <= 0) {
		if (src->start_of_file) {  // Treat empty input file as fatal error
			//ERREXIT(cinfo, JERR_INPUT_EMPTY);
			jpgErrorOccured = IL_TRUE;
		}
		//WARNMS(cinfo, JWRN_JPEG_EOF);
		// Insert a fake EOI marker
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
		return IL_FALSE;
	}
	if (nbytes < INPUT_BUF_SIZE) {
		ilGetError();  // Gets rid of the IL_FILE_READ_ERROR.
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = IL_FALSE;

	return IL_TRUE;
}


METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	iread_ptr src = (iread_ptr) cinfo->src;

	if (num_bytes > 0) {
		while (num_bytes > (long) src->pub.bytes_in_buffer) {
			num_bytes -= (long) src->pub.bytes_in_buffer;
			(void) fill_input_buffer(cinfo);
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}


METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
	// no work necessary here
}


GLOBAL(void)
devil_jpeg_read_init (j_decompress_ptr cinfo)
{
	iread_ptr src;

	if ( cinfo->src == NULL ) {  // first time for this JPEG object?
		cinfo->src = (struct jpeg_source_mgr *)
					 (*cinfo->mem->alloc_small)( (j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(iread_mgr) );
		src = (iread_ptr) cinfo->src;
		src->buffer = (JOCTET *)
					  (*cinfo->mem->alloc_small)( (j_common_ptr)cinfo, JPOOL_PERMANENT,
												  INPUT_BUF_SIZE * sizeof(JOCTET) );
	}

	src = (iread_ptr) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart;  // use default method
	src->pub.term_source = term_source;
	src->pub.bytes_in_buffer = 0;  // forces fill_input_buffer on first read
	src->pub.next_input_byte = NULL;  // until buffer loaded
}


jmp_buf	JpegJumpBuffer;

static void iJpegErrorExit( j_common_ptr cinfo )
{
	ilSetError( IL_LIB_JPEG_ERROR );
	jpeg_destroy( cinfo );
	longjmp( JpegJumpBuffer, 1 );
}

// Internal function used to load the jpeg.
ILboolean iLoadJpegInternal()
{
	struct jpeg_error_mgr			Error;
	struct jpeg_decompress_struct	JpegInfo;
	ILboolean						result;

	if ( iCurImage == NULL )
	{
		ilSetError( IL_ILLEGAL_OPERATION );
		return IL_FALSE;
	}

	JpegInfo.err = jpeg_std_error( &Error );		// init standard error handlers
	Error.error_exit = iJpegErrorExit;				// add our exit handler
	Error.output_message = OutputMsg;

	if ( (result = setjmp( JpegJumpBuffer ) == 0) != IL_FALSE )
	{
		jpeg_create_decompress( &JpegInfo );
		JpegInfo.do_block_smoothing = IL_TRUE;
		JpegInfo.do_fancy_upsampling = IL_TRUE;

		//jpeg_stdio_src(&JpegInfo, iGetFile());
		devil_jpeg_read_init( &JpegInfo );
		jpeg_read_header( &JpegInfo, IL_TRUE );

		result = ilLoadFromJpegStruct( &JpegInfo );

		jpeg_finish_decompress( &JpegInfo );
		jpeg_destroy_decompress( &JpegInfo );
	}
	return result;
}


typedef struct
{
	struct jpeg_destination_mgr		pub;
	JOCTET							*buffer;
	ILboolean						bah;
} iwrite_mgr;

typedef iwrite_mgr *iwrite_ptr;

#define OUTPUT_BUF_SIZE 4096


METHODDEF(void)
init_destination(j_compress_ptr cinfo)
{
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	dest->buffer = (JOCTET *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  OUTPUT_BUF_SIZE * sizeof(JOCTET));

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
	return;
}

METHODDEF(boolean)
empty_output_buffer (j_compress_ptr cinfo)
{
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	iwrite(dest->buffer, 1, OUTPUT_BUF_SIZE);
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
	return IL_TRUE;
}

METHODDEF(void)
term_destination (j_compress_ptr cinfo)
{
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	iwrite(dest->buffer, 1, OUTPUT_BUF_SIZE - dest->pub.free_in_buffer);
	return;
}


GLOBAL(void)
devil_jpeg_write_init(j_compress_ptr cinfo)
{
	iwrite_ptr dest;

	if (cinfo->dest == NULL) {	// first time for this JPEG object?
		cinfo->dest = (struct jpeg_destination_mgr *)
		  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
					  sizeof(struct jpeg_destination_mgr));
		dest = (iwrite_ptr)cinfo->dest;
	}

	dest = (iwrite_ptr)cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;

	return;
}


//! Writes a Jpeg file
ILboolean ilSaveJpeg(const ILstring FileName)
{
	ILHANDLE	JpegFile;
	ILboolean	bJpeg = IL_FALSE;

	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	JpegFile = iopenw(FileName);
	if (JpegFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bJpeg;
	}

	bJpeg = ilSaveJpegF(JpegFile);
	iclosew(JpegFile);

	return bJpeg;
}


//! Writes a Jpeg to an already-opened file
ILboolean ilSaveJpegF(ILHANDLE File)
{
	iSetOutputFile(File);
	return iSaveJpegInternal();
}


//! Writes a Jpeg to a memory "lump"
ILboolean ilSaveJpegL(ILvoid *Lump, ILuint Size)
{
	iSetOutputLump(Lump, Size);
	return iSaveJpegInternal();
}


// Internal function used to save the Jpeg.
ILboolean iSaveJpegInternal()
{
	struct		jpeg_compress_struct JpegInfo;
	struct		jpeg_error_mgr Error;
	JSAMPROW	row_pointer[1];
	ILimage		*TempImage;
	ILubyte		*TempData;
	ILenum		Type = 0;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	/*if (iGetHint(IL_COMPRESSION_HINT) == IL_USE_COMPRESSION)
		Quality = 85;  // Not sure how low we should dare go...
	else
		Quality = 99;*/

	if ((iCurImage->Format != IL_RGB && iCurImage->Format != IL_LUMINANCE) || iCurImage->Bpc != 1) {
		TempImage = iConvertImage(iCurImage, IL_RGB, IL_UNSIGNED_BYTE);
		if (TempImage == NULL) {
			return IL_FALSE;
		}
	}
	else {
		TempImage = iCurImage;
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			if (TempImage != iCurImage)
				ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}


	JpegInfo.err = jpeg_std_error(&Error);
	// Now we can initialize the JPEG compression object.
	jpeg_create_compress(&JpegInfo);

	//jpeg_stdio_dest(&JpegInfo, JpegFile);
	devil_jpeg_write_init(&JpegInfo);

	JpegInfo.image_width = TempImage->Width;  // image width and height, in pixels
	JpegInfo.image_height = TempImage->Height;
	JpegInfo.input_components = TempImage->Bpp;  // # of color components per pixel

	// John Villar's addition
	if (TempImage->Bpp == 1)
		JpegInfo.in_color_space = JCS_GRAYSCALE;
	else
		JpegInfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&JpegInfo);

#ifndef IL_USE_JPEGLIB_UNMODIFIED
	Type = iGetInt(IL_JPG_SAVE_FORMAT);
	if (Type == IL_EXIF) {
		JpegInfo.write_JFIF_header = FALSE;
		JpegInfo.write_EXIF_header = TRUE;
	}
	else if (Type == IL_JFIF) {
		JpegInfo.write_JFIF_header = TRUE;
		JpegInfo.write_EXIF_header = FALSE;
	}
#else
	Type = Type;
	JpegInfo.write_JFIF_header = TRUE;
#endif//IL_USE_JPEGLIB_UNMODIFIED

	jpeg_set_quality(&JpegInfo, iGetInt(IL_JPG_QUALITY), IL_TRUE);

	jpeg_start_compress(&JpegInfo, IL_TRUE);

	//row_stride = image_width * 3;	// JSAMPLEs per row in image_buffer

	while (JpegInfo.next_scanline < JpegInfo.image_height) {
		// jpeg_write_scanlines expects an array of pointers to scanlines.
		// Here the array is only one element long, but you could pass
		// more than one scanline at a time if that's more convenient.
		row_pointer[0] = &TempData[JpegInfo.next_scanline * TempImage->Bps];
		(ILvoid) jpeg_write_scanlines(&JpegInfo, row_pointer, 1);
	}

	// Step 6: Finish compression
	jpeg_finish_compress(&JpegInfo);

	// Step 7: release JPEG compression object

	// This is an important step since it will release a good deal of memory.
	jpeg_destroy_compress(&JpegInfo);

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (TempImage != iCurImage)
		ilCloseImage(TempImage);

	return IL_TRUE;
}



#else // Use the IJL instead of libjpeg.



//! Reads a jpeg file
ILboolean ilLoadJpeg(const ILstring FileName)
{
	if (!iFileExists(FileName)) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}
	return iLoadJpegInternal(FileName, NULL, 0);
}


// Reads from a memory "lump" containing a jpeg
ILboolean ilLoadJpegL(ILvoid *Lump, ILuint Size)
{
	return iLoadJpegInternal(NULL, Lump, Size);
}


// Internal function used to load the jpeg.
ILboolean iLoadJpegInternal(const ILstring FileName, ILvoid *Lump, ILuint Size)
{
    JPEG_CORE_PROPERTIES Image;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (ijlInit(&Image) != IJL_OK) {
		ilSetError(IL_LIB_JPEG_ERROR);
		return IL_FALSE;
	}

	if (FileName != NULL) {
		Image.JPGFile = FileName;
		if (ijlRead(&Image, IJL_JFILE_READPARAMS) != IJL_OK) {
			ilSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}
	else {
		Image.JPGBytes = Lump;
		Image.JPGSizeBytes = Size > 0 ? Size : UINT_MAX;
		if (ijlRead(&Image, IJL_JBUFF_READPARAMS) != IJL_OK) {
			ilSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}

	switch (Image.JPGChannels)
	{
		case 1:
			Image.JPGColor		= IJL_G;
			Image.DIBChannels	= 1;
			Image.DIBColor		= IJL_G;
			iCurImage->Format	= IL_LUMINANCE;
			break;

		case 3:
			Image.JPGColor		= IJL_YCBCR;
			Image.DIBChannels	= 3;
			Image.DIBColor		= IJL_RGB;
			iCurImage->Format	= IL_RGB;
			break;

        case 4:
			Image.JPGColor		= IJL_YCBCRA_FPX;
			Image.DIBChannels	= 4;
			Image.DIBColor		= IJL_RGBA_FPX;
			iCurImage->Format	= IL_RGBA;
			break;

        default:
			// This catches everything else, but no
			// color twist will be performed by the IJL.
			/*Image.DIBColor = (IJL_COLOR)IJL_OTHER;
			Image.JPGColor = (IJL_COLOR)IJL_OTHER;
			Image.DIBChannels = Image.JPGChannels;
			break;*/
			ijlFree(&Image);
			ilSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
	}

	if (!ilTexImage(Image.JPGWidth, Image.JPGHeight, 1, (ILubyte)Image.DIBChannels, iCurImage->Format, IL_UNSIGNED_BYTE, NULL)) {
		ijlFree(&Image);
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	Image.DIBWidth		= Image.JPGWidth;
	Image.DIBHeight		= Image.JPGHeight;
	Image.DIBPadBytes	= 0;
	Image.DIBBytes		= iCurImage->Data;

	if (FileName != NULL) {
		if (ijlRead(&Image, IJL_JFILE_READWHOLEIMAGE) != IJL_OK) {
			ijlFree(&Image);
			ilSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}
	else {
		if (ijlRead(&Image, IJL_JBUFF_READWHOLEIMAGE) != IJL_OK) {
			ijlFree(&Image);
			ilSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}

	ijlFree(&Image);
	ilFixImage();

	return IL_TRUE;
}


//! Writes a Jpeg file
ILboolean ilSaveJpeg(const ILstring FileName)
{
	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	return iSaveJpegInternal(FileName, NULL, 0);
}


//! Writes a Jpeg to a memory "lump"
ILboolean ilSaveJpegL(ILvoid *Lump, ILuint Size)
{
	return iSaveJpegInternal(NULL, Lump, Size);
}


// Internal function used to save the Jpeg.
ILboolean iSaveJpegInternal(const ILstring FileName, ILvoid *Lump, ILuint Size)
{
	JPEG_CORE_PROPERTIES	Image;
	ILuint	Quality;
	ILimage	*TempImage;
	ILubyte	*TempData;

	imemclear(&Image, sizeof(JPEG_CORE_PROPERTIES));

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	if (FileName == NULL && Lump == NULL) {
		ilSetError(IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iGetHint(IL_COMPRESSION_HINT) == IL_USE_COMPRESSION)
		Quality = 85;  // Not sure how low we should dare go...
	else
		Quality = 99;

	if (ijlInit(&Image) != IJL_OK) {
		ilSetError(IL_LIB_JPEG_ERROR);
		return IL_FALSE;
	}

	if ((iCurImage->Format != IL_RGB && iCurImage->Format != IL_RGBA && iCurImage->Format != IL_LUMINANCE)
		|| iCurImage->Bpc != 1) {
		if (iCurImage->Format == IL_BGRA)
			Temp = iConvertImage(iCurImage, IL_RGBA, IL_UNSIGNED_BYTE);
		else
			Temp = iConvertImage(iCurImage, IL_RGB, IL_UNSIGNED_BYTE);
		if (Temp == NULL) {
			return IL_FALSE;
		}
	}
	else {
		Temp = iCurImage;
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			if (TempImage != iCurImage)
				ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	// Setup DIB
	Image.DIBWidth		= TempImage->Width;
	Image.DIBHeight		= TempImage->Height;
	Image.DIBChannels	= TempImage->Bpp;
	Image.DIBBytes		= TempData;
	Image.DIBPadBytes	= 0;

	// Setup JPEG
	Image.JPGWidth		= TempImage->Width;
	Image.JPGHeight		= TempImage->Height;
	Image.JPGChannels	= TempImage->Bpp;

	switch (Temp->Bpp)
	{
		case 1:
			Image.DIBColor			= IJL_G;
			Image.JPGColor			= IJL_G;
			Image.JPGSubsampling	= IJL_NONE;
			break;
		case 3:
			Image.DIBColor			= IJL_RGB;
			Image.JPGColor			= IJL_YCBCR;
			Image.JPGSubsampling	= IJL_411;
			break;
		case 4:
			Image.DIBColor			= IJL_RGBA_FPX;
			Image.JPGColor			= IJL_YCBCRA_FPX;
			Image.JPGSubsampling	= IJL_4114;
			break;
	}

	if (FileName != NULL) {
		Image.JPGFile = FileName;
		if (ijlWrite(&Image, IJL_JFILE_WRITEWHOLEIMAGE) != IJL_OK) {
			if (TempImage != iCurImage)
				ilCloseImage(TempImage);
			ilSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}
	else {
		Image.JPGBytes = Lump;
		Image.JPGSizeBytes = Size;
		if (ijlWrite(&Image, IJL_JBUFF_WRITEWHOLEIMAGE) != IJL_OK) {
			if (TempImage != iCurImage)
				ilCloseImage(TempImage);
			ilSetError(IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}

	ijlFree(&Image);

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (Temp != iCurImage)
		ilCloseImage(Temp);

	return IL_TRUE;
}

#endif//IL_USE_IJL




#endif//IL_NO_JPG


// Access point for applications wishing to use the jpeg library directly in
// conjunction with DevIL.
//
// The decompressor must be set up with an input source and all desired parameters
// this function is called. The caller must call jpeg_finish_decompress because
// the caller may still need decompressor after calling this for e.g. examining
// saved markers.
ILboolean ILAPIENTRY ilLoadFromJpegStruct(ILvoid *_JpegInfo)
{
#ifndef IL_NO_JPG
#ifndef IL_USE_IJL
	// sam. void (*errorHandler)(j_common_ptr);
	ILubyte	*TempPtr[1];
	ILuint	Returned;
	j_decompress_ptr JpegInfo = (j_decompress_ptr)_JpegInfo;

	//added on 2003-08-31 as explained in sf bug 596793
	jpgErrorOccured = IL_FALSE;

	// sam. errorHandler = JpegInfo->err->error_exit;
	// sam. JpegInfo->err->error_exit = ExitErrorHandle;
	jpeg_start_decompress((j_decompress_ptr)JpegInfo);

	if (!ilTexImage(JpegInfo->output_width, JpegInfo->output_height, 1, (ILubyte)JpegInfo->output_components, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (iCurImage->Bpp)
	{
		case 1:
			iCurImage->Format = IL_LUMINANCE;
			break;
		case 3:
			iCurImage->Format = IL_RGB;
			break;
		case 4:
			iCurImage->Format = IL_RGBA;
			break;
		default:
			// Anyway to get here?  Need to error out or something...
			break;
	}

	TempPtr[0] = iCurImage->Data;
	while (JpegInfo->output_scanline < JpegInfo->output_height) {
		Returned = jpeg_read_scanlines(JpegInfo, TempPtr, 1);  // anyway to make it read all at once?
		TempPtr[0] += iCurImage->Bps;
		if (Returned == 0)
			break;
	}

	// sam. JpegInfo->err->error_exit = errorHandler;

	if (jpgErrorOccured)
		return IL_FALSE;

	ilFixImage();
	return IL_TRUE;
#endif
#endif
	return IL_FALSE;
}



// Access point for applications wishing to use the jpeg library directly in
// conjunction with DevIL.
//
// The caller must set up the desired parameters by e.g. calling
// jpeg_set_defaults and overriding the parameters the caller wishes
// to change, such as quality, before calling this function. The caller
// is also responsible for calling jpeg_finish_compress in case the
// caller still needs to compressor for something.
// 
ILboolean ILAPIENTRY ilSaveFromJpegStruct(ILvoid *_JpegInfo)
{
#ifndef IL_NO_JPG
#ifndef IL_USE_IJL
	void (*errorHandler)(j_common_ptr);
	JSAMPROW	row_pointer[1];
	ILimage		*TempImage;
	ILubyte		*TempData;
	j_compress_ptr JpegInfo = (j_compress_ptr)_JpegInfo;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	//added on 2003-08-31 as explained in sf bug 596793
	jpgErrorOccured = IL_FALSE;

	errorHandler = JpegInfo->err->error_exit;
	JpegInfo->err->error_exit = ExitErrorHandle;


	if ((iCurImage->Format != IL_RGB && iCurImage->Format != IL_LUMINANCE) || iCurImage->Bpc != 1) {
		TempImage = iConvertImage(iCurImage, IL_RGB, IL_UNSIGNED_BYTE);
		if (TempImage == NULL) {
			return IL_FALSE;
		}
	}
	else {
		TempImage = iCurImage;
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			if (TempImage != iCurImage)
				ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	JpegInfo->image_width = TempImage->Width;  // image width and height, in pixels
	JpegInfo->image_height = TempImage->Height;
	JpegInfo->input_components = TempImage->Bpp;  // # of color components per pixel

	jpeg_start_compress(JpegInfo, IL_TRUE);

	//row_stride = image_width * 3;	// JSAMPLEs per row in image_buffer

	while (JpegInfo->next_scanline < JpegInfo->image_height) {
		// jpeg_write_scanlines expects an array of pointers to scanlines.
		// Here the array is only one element long, but you could pass
		// more than one scanline at a time if that's more convenient.
		row_pointer[0] = &TempData[JpegInfo->next_scanline * TempImage->Bps];
		(ILvoid) jpeg_write_scanlines(JpegInfo, row_pointer, 1);
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (TempImage != iCurImage)
		ilCloseImage(TempImage);

	return (!jpgErrorOccured);
#endif
#endif
	return IL_FALSE;
}
