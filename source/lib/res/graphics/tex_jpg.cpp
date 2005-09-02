#include "precompiled.h"

extern "C" {
// this is not a core library module, so it doesn't define JPEG_INTERNALS
#include "jpeglib.h"
#include "jerror.h"
}

#include "lib.h"
#include "lib/res/res.h"
#include "tex_codec.h"

#if MSC_VERSION
# ifdef NDEBUG
#  pragma comment(lib, "jpeg-6b.lib")
# else
# pragma comment(lib, "jpeg-6bd.lib")
# endif	// #ifdef NDEBUG
#endif	// #ifdef MSC_VERSION



/* IMPORTANT: we assume that JOCTET is 8 bits. */
cassert(sizeof(JOCTET) == 1 && CHAR_BIT	== 8);

//-----------------------------------------------------------------------------
// mem source manager
//-----------------------------------------------------------------------------


/* Expanded data source object for memory input */
typedef struct
{
	struct jpeg_source_mgr pub;	/* public fields */

	JOCTET* buf;
	size_t size;	/* total size (bytes) */
	size_t pos;		/* offset (bytes) to new data */
}
MemSrcMgr;

typedef MemSrcMgr* SrcPtr;


/*
* Initialize source --- called by jpeg_read_header
* before any data is actually read.
*/

METHODDEF(void) init_source(j_decompress_ptr UNUSED(cinfo))
{
}


/*
* Fill the input buffer --- called whenever buffer is emptied.
*
* In typical applications, this should read fresh data into the buffer
* (ignoring the current state of next_input_byte & bytes_in_buffer),
* reset the pointer & count to the start of the buffer, and return TRUE
* indicating that the buffer has been reloaded.  It is not necessary to
* fill the buffer entirely, only to obtain at least one more byte.
*
* There is no such thing as an EOF return.  If the end of the file has been
* reached, the routine has a choice of ERREXIT() or inserting fake data into
* the buffer.  In most cases, generating a warning message and inserting a
* fake EOI marker is the best course of action --- this will allow the
* decompressor to output however much of the image is there.  However,
* the resulting error message is misleading if the real problem is an empty
* input file, so we handle that case specially.
*/

METHODDEF(boolean) fill_input_buffer(j_decompress_ptr cinfo)
{
	SrcPtr src = (SrcPtr)cinfo->src;
	static const JOCTET eoi[2] = { 0xFF, JPEG_EOI };

	/*
	* since jpeg_mem_src fills the buffer with everything we've got,
	* jpeg is trying to read beyond end of buffer. return a fake EOI marker.
	* note: don't modify input buffer: it might be read-only.
	*/

	WARNMS(cinfo, JWRN_JPEG_EOF);


	src->pub.next_input_byte = eoi;
	src->pub.bytes_in_buffer = 2;
	return TRUE;
}


/*
* Skip data --- used to skip over a potentially large amount of
* uninteresting data (such as an APPn marker).
*/

METHODDEF(void) skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	SrcPtr src = (SrcPtr)cinfo->src;
	size_t skip_count = (size_t)num_bytes;

	/* docs say non-positive num_byte skips should be ignored */
	if(num_bytes <= 0)
		return;

	/*
	* just subtract bytes available in buffer,
	* making sure we don't underflow the size_t.
	* note: if we skip to or beyond end of buffer,
	* bytes_in_buffer = 0 => fill_input_buffer called => abort.
	*/
	if(skip_count > src->pub.bytes_in_buffer)
		skip_count = src->pub.bytes_in_buffer;

	src->pub.bytes_in_buffer -= skip_count;
	src->pub.next_input_byte += skip_count;
}


/*
* An additional method that can be provided by data source modules is the
* resync_to_restart method for error recovery in the presence of RST markers.
* For the moment, this source module just uses the default resync method
* provided by the JPEG library.  That method assumes that no backtracking
* is possible.
*/


/*
* Terminate source --- called by jpeg_finish_decompress
* after all data has been read.  Often a no-op.
*
* NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
* application must deal with any cleanup that should happen even
* for error exit.
*/

METHODDEF(void) term_source(j_decompress_ptr UNUSED(cinfo))
{
	/*
	* no-op (we don't own the buffer and shouldn't,
	* to make possible multiple images in a source).
	*/
}


/*
* Prepare for input from a buffer.
* The caller is responsible for freeing it after finishing decompression.
*/

GLOBAL(void) jpeg_mem_src(j_decompress_ptr cinfo, void* p, size_t size)
{
	SrcPtr src;

	/* Treat 0-length buffer as fatal error */
	if(size == 0)
		ERREXIT(cinfo, JERR_INPUT_EMPTY);

	/*
	* The source object is made permanent so that
	* a series of JPEG images can be read from the same file
	* by calling jpeg_mem_src only before the first one.
	* This makes it unsafe to use this manager and a different source
	* manager serially with the same JPEG object.  Caveat programmer.
	*/

	/* first time for this JPEG object? */
	if(!cinfo->src)
		cinfo->src = (struct jpeg_source_mgr*)
		(*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT,
		sizeof(MemSrcMgr));
	/* (takes care of raising error if out of memory) */

	src = (SrcPtr)cinfo->src;
	src->pub.init_source       = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data   = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* default */
	src->pub.term_source       = term_source;

	/*
	* fill buffer with everything we have.
	* if fill_input_buffer is called, the buffer was overrun.
	*/
	src->pub.bytes_in_buffer   = size;
	src->pub.next_input_byte   = (JOCTET*)p;
}


//-----------------------------------------------------------------------------
// mem destination manager
//-----------------------------------------------------------------------------

/* Expanded data destination object for VFS output */
typedef struct {
	struct jpeg_destination_mgr pub; /* public fields */

	Handle hf;
	JOCTET* buf;
	// jpeg-6b interface needs a memory buffer
} VfsDstMgr;

typedef VfsDstMgr* DstPtr;

#define OUTPUT_BUF_SIZE  16*KiB	/* choose an efficiently writeable size */


/*
* Initialize destination --- called by jpeg_start_compress
* before any data is actually written.
*/

METHODDEF(void) init_destination(j_compress_ptr cinfo)
{
	DstPtr dst = (DstPtr)cinfo->dest;

	/* Allocate the output buffer --- it will be released when done with image */
	dst->buf = (JOCTET*)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_IMAGE,
		OUTPUT_BUF_SIZE * sizeof(JOCTET));

	dst->pub.next_output_byte = dst->buf;
	dst->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}


/*
* Empty the output buffer --- called whenever buffer fills up.
*
* In typical applications, this should write the entire output buffer
* (ignoring the current state of next_output_byte & free_in_buffer),
* reset the pointer & count to the start of the buffer, and return TRUE
* indicating that the buffer has been dumped.
*
* In applications that need to be able to suspend compression due to output
* overrun, a FALSE return indicates that the buffer cannot be emptied now.
* In this situation, the compressor will return to its caller (possibly with
* an indication that it has not accepted all the supplied scanlines).  The
* application should resume compression after it has made more room in the
* output buffer.  Note that there are substantial restrictions on the use of
* suspension --- see the documentation.
*
* When suspending, the compressor will back up to a convenient restart point
* (typically the start of the current MCU). next_output_byte & free_in_buffer
* indicate where the restart point will be if the current call returns FALSE.
* Data beyond this point will be regenerated after resumption, so do not
* write it out when emptying the buffer externally.
*/

METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo)
{
	DstPtr dst = (DstPtr)cinfo->dest;

	// writing out OUTPUT_BUF_SIZE-dst->pub.free_in_buffer bytes
	// sounds reasonable, but make for broken output.
	if(vfs_io(dst->hf, OUTPUT_BUF_SIZE, (void**)&dst->buf) != OUTPUT_BUF_SIZE)
		ERREXIT(cinfo, JERR_FILE_WRITE);

	dst->pub.next_output_byte = dst->buf;
	dst->pub.free_in_buffer = OUTPUT_BUF_SIZE;

	return TRUE;
}


/*
* Terminate destination --- called by jpeg_finish_compress
* after all data has been written.  Usually needs to flush buffer.
*
* NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
* application must deal with any cleanup that should happen even
* for error exit.
*/

METHODDEF(void) term_destination(j_compress_ptr cinfo)
{
	DstPtr dst = (DstPtr)cinfo->dest;

	// make sure any data left in the buffer is written out
	const size_t bytes_in_buf = OUTPUT_BUF_SIZE - dst->pub.free_in_buffer;
	if(vfs_io(dst->hf, bytes_in_buf, (void**)&dst->buf) != (ssize_t)bytes_in_buf)
		ERREXIT(cinfo, JERR_FILE_WRITE);

	// flush file, if necessary.
}


/*
* Prepare for output to a stdio stream.
* The caller must have already opened the stream, and is responsible
* for closing it after finishing compression.
*/

GLOBAL(void) jpeg_vfs_dst(j_compress_ptr cinfo, Handle hf)
{
	/* The destination object is made permanent so that multiple JPEG images
	* can be written to the same file without re-executing jpeg_stdio_dest.
	* This makes it dangerous to use this manager and a different destination
	* manager serially with the same JPEG object, because their private object
	* sizes may be different.  Caveat programmer.
	*/
	if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr*)(*cinfo->mem->alloc_small)
			((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(VfsDstMgr));
	}

	DstPtr dst = (DstPtr)cinfo->dest;
	dst->pub.init_destination    = init_destination;
	dst->pub.empty_output_buffer = empty_output_buffer;
	dst->pub.term_destination    = term_destination;
	dst->hf = hf;
}


//-----------------------------------------------------------------------------
// error handler, shared by jpg_(en|de)code
//-----------------------------------------------------------------------------

// the JPEG library's standard error handler (jerror.c) is divided into
// several "methods" which we can override individually. This allows
// adjusting the behavior without duplicating a lot of code, which may
// have to be updated with each future release.
//
// we here override error_exit to return control to the library's caller
// (i.e. jpg_(de|en)code) when a fatal error occurs, rather than calling exit.
//
// the replacement error_exit does a longjmp back to the caller's
// setjmp return point. it needs access to the jmp_buf,
// so we store it in a "subclass" of jpeg_error_mgr.

struct JpgErrMgr
{
	struct jpeg_error_mgr pub;	// "public" fields

	jmp_buf call_site; // jump here (back to JPEG lib caller) on error 
	char msg[JMSG_LENGTH_MAX];	// description of first error encountered
	// must store per JPEG context for thread safety.
	// initialized as part of JPEG context error setup.
};

METHODDEF(void) jpg_error_exit(j_common_ptr cinfo)
{
	// get subclass
	JpgErrMgr* err_mgr = (JpgErrMgr*)cinfo->err;

	// "output" error message (i.e. store in JpgErrMgr;
	// call_site is responsible for displaying it via debug_printf)
	(*cinfo->err->output_message)(cinfo);

	// jump back to call site, i.e. jpg_(de|en)code
	longjmp(err_mgr->call_site, 1);
}


// stores message in JpgErrMgr for later output by jpg_(de|en)code.
// note: don't display message here, so the caller can
//   add some context (whether encoding or decoding, and filename).
METHODDEF(void) jpg_output_message(j_common_ptr cinfo)
{
	// get subclass
	JpgErrMgr* err_mgr = (JpgErrMgr*)cinfo->err;

	// this context already had an error message; don't overwrite it.
	// (subsequent errors probably aren't related to the real problem).
	// note: was set to '\0' by JPEG context error setup.
	if(err_mgr->msg[0] != '\0')
		return;

	// generate the message and store it
	(*cinfo->err->format_message)(cinfo, err_mgr->msg);
}


//-----------------------------------------------------------------------------


static int jpg_transform(Tex* t, int new_flags)
{
	return TEX_CODEC_CANNOT_HANDLE;
}


static int jpg_decode(u8* file, size_t file_size, Tex* t, const char** perr_msg)
{
	// JFIF requires SOI marker at start of stream.
	// we compare single bytes to be endian-safe.
	if(file[0] != 0xff || file[1] == 0xd8)
		return TEX_CODEC_CANNOT_HANDLE;

	int err = -1;

	// freed when ret is reached:
	struct jpeg_decompress_struct cinfo;
	// contains the JPEG decompression parameters and pointers to
	// working space (allocated as needed by the JPEG library).
	RowArray rows = 0;
	// array of pointers to scanlines in img, set by alloc_rows.
	// jpeg won't output more than a few scanlines at a time,
	// so we need an output loop anyway, but passing at least 2..4
	// rows is more efficient in low-quality modes (due to less copying).

	// freed when fail is reached:
	Handle img_hm;	// decompressed image memory

	// set up our error handler, which overrides jpeg's default
	// write-to-stderr-and-exit behavior.
	// notes:
	// - must be done before jpeg_create_decompress, in case that fails
	//   (unlikely, but possible if out of memory).
	// - valid over cinfo lifetime (avoids dangling pointer in cinfo)
	JpgErrMgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpg_error_exit;
	jerr.pub.output_message = jpg_output_message;
	jerr.msg[0] = '\0';
	// required for "already have message" check in output_message
	if(setjmp(jerr.call_site))
	{
fail:
		// either JPEG has raised an error, or code below failed.
		mem_free_h(img_hm);
		goto ret;
	}

	// goto scoping
	{
		jpeg_create_decompress(&cinfo);

		jpeg_mem_src(&cinfo, file, file_size);


		//
		// read header, determine format
		//

		(void) jpeg_read_header(&cinfo, TRUE);
		// we can ignore the return value since:
		// - suspension is not possible with the mem data source
		// - we passed TRUE to raise an error if table-only JPEG file

		int bpp = cinfo.num_components * 8;
		// preliminary; set below to reflect output params

		// make sure we get a colour format we know
		// (exception: if bpp = 8, go greyscale below)
		// necessary to support non-standard CMYK files written by Photoshop.
		cinfo.out_color_space = JCS_RGB;

		int flags = 0;
		if(bpp == 8)
		{
			flags |= TEX_GREY;
			cinfo.out_color_space = JCS_GRAYSCALE;
		}

		// lower quality, but faster
		cinfo.dct_method = JDCT_IFAST;
		cinfo.do_fancy_upsampling = FALSE;


		(void) jpeg_start_decompress(&cinfo);
		// we can ignore the return value since
		// suspension is not possible with the mem data source.

		// scaled output image dimensions and final bpp are now available.
		int w = cinfo.output_width;
		int h = cinfo.output_height;
		bpp = cinfo.output_components * 8;

		// note: since we've set out_color_space, JPEG will always
		// return an acceptable image format; no need to check.


		//
		// allocate memory for uncompressed image
		//

		size_t pitch = w * bpp / 8;
		// needed by alloc_rows
		const size_t img_size = pitch * h;
		// cannot free old t->hm until after jpeg_finish_decompress,
		// but need to set to this handle afterwards => need tmp var.
		u8* img = (u8*)mem_alloc(img_size, 64*KiB, 0, &img_hm);
		if(!img)
		{
			err = ERR_NO_MEM;
			goto fail;
		}
		int ret = tex_codec_alloc_rows(img, h, pitch, TEX_TOP_DOWN, rows);
		if(ret < 0)
		{
			err = ret;
			goto fail;
		}


		// could use cinfo.output_scanline to keep track of progress,
		// but we need to count lines_left anyway (paranoia).
		JSAMPARRAY row = (JSAMPARRAY)rows;
		JDIMENSION lines_left = h;
		while(lines_left != 0)
		{
			JDIMENSION lines_read = jpeg_read_scanlines(&cinfo, row, lines_left);
			row += lines_read;
			lines_left -= lines_read;

			// we've decoded in-place; no need to further process
		}

		(void)jpeg_finish_decompress(&cinfo);
		// we can ignore the return value since suspension
		// is not possible with the mem data source.

		if(jerr.pub.num_warnings != 0)
			debug_printf("jpg_decode: corrupt-data warning(s) occurred\n");

		// store image info
		// .. transparently switch handles - free the old (compressed)
		//    buffer and replace it with the decoded-image memory handle.
		mem_free_h(t->hm);
		t->hm    = img_hm;
		t->ofs   = 0;	// jpeg returns decoded image data; no header
		t->w     = w;
		t->h     = h;
		t->bpp   = bpp;
		t->flags = flags;

		err = 0;

	}

	// shared cleanup
ret:
	jpeg_destroy_decompress(&cinfo);
	// releases a "good deal" of memory

	free(rows);

	return err;
}



// limitation: palette images aren't supported
static int jpg_encode(const char* ext, Tex* t, u8** out, size_t* out_size, const char** perr_msg)
{
	if(stricmp(ext, "jpg") && stricmp(ext, "jpeg"))
		return TEX_CODEC_CANNOT_HANDLE;

	const char* msg = 0;
	int err = -1;

	// freed when ret is reached:
	struct jpeg_compress_struct cinfo;
	// contains the JPEG compression parameters and pointers to
	// working space (allocated as needed by the JPEG library).
	RowArray rows = 0;
	// array of pointers to scanlines in img, set by alloc_rows.
	// jpeg won't output more than a few scanlines at a time,
	// so we need an output loop anyway, but passing at least 2..4
	// rows is more efficient in low-quality modes (due to less copying).

	Handle hf = 0;

	// set up our error handler, which overrides jpeg's default
	// write-to-stderr-and-exit behavior.
	// notes:
	// - must be done before jpeg_create_compress, in case that fails
	//   (unlikely, but possible if out of memory).
	// - valid over cinfo lifetime (avoids dangling pointer in cinfo)
	JpgErrMgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpg_error_exit;
	jerr.pub.output_message = jpg_output_message;
	jerr.msg[0] = '\0';
	// required for "already have message" check in output_message
	if(setjmp(jerr.call_site))
	{
fail:
		// either JPEG has raised an error, or code below failed.
		goto ret;
	}

	// goto scoping
	{

		jpeg_create_compress(&cinfo);
/*
		hf = vfs_open(fn, FILE_WRITE|FILE_NO_AIO);
		if(hf <= 0)
		{
			err = (int)hf;
			goto fail;
		}
		jpeg_vfs_dst(&cinfo, hf);
*/

		//
		// describe input image
		//

		// required:
		cinfo.image_width = t->w;
		cinfo.image_height = t->h;
		cinfo.input_components = t->bpp / 8;
		cinfo.in_color_space = (t->bpp == 8)? JCS_GRAYSCALE : JCS_RGB;

		// defaults depend on cinfo.in_color_space already having been set!
		jpeg_set_defaults(&cinfo);

		// more settings (e.g. quality)


		jpeg_start_compress(&cinfo, TRUE);
		// TRUE ensures that we will write a complete interchange-JPEG file.
		// don't change unless you are very sure of what you're doing.


		// make sure we have RGB
		const int bgr_transform = t->flags & TEX_BGR;	// JPG is native RGB.
		WARN_ERR(tex_transform(t, bgr_transform));

		const size_t pitch = t->w * t->bpp / 8;
		u8* data = tex_get_data(t);
		int ret = tex_codec_alloc_rows(data, t->h, pitch, TEX_TOP_DOWN, rows);
		if(ret < 0)
		{
			err = ret;
			goto fail;
		}


		// could use cinfo.output_scanline to keep track of progress,
		// but we need to count lines_left anyway (paranoia).
		JSAMPARRAY row = (JSAMPARRAY)rows;
		JDIMENSION lines_left = t->h;
		while(lines_left != 0)
		{
			JDIMENSION lines_read = jpeg_write_scanlines(&cinfo, row, lines_left);
			row += lines_read;
			lines_left -= lines_read;

			// we've decoded in-place; no need to further process
		}

		jpeg_finish_compress(&cinfo);

		if(jerr.pub.num_warnings != 0)
			debug_printf("jpg_encode: corrupt-data warning(s) occurred\n");

		err = 0;

	}

	// shared cleanup
ret:
	jpeg_destroy_compress(&cinfo);
	// releases a "good deal" of memory

	free(rows);
	vfs_close(hf);

	return err;
}

TEX_CODEC_REGISTER(jpg);
