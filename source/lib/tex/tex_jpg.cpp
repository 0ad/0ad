/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * JPEG codec using IJG jpeglib.
 */

#include "precompiled.h"

#include <setjmp.h>

#include "lib/external_libraries/libjpeg.h"
#include "lib/allocators/shared_ptr.h"

#include "tex_codec.h"


// squelch "dtor / setjmp interaction" warnings.
// all attempts to resolve the underlying problem failed; apparently
// the warning is generated if setjmp is used at all in C++ mode.
// (jpg_*code have no code that would trigger ctors/dtors, nor are any
// called in their prolog/epilog code).
#if MSC_VERSION
# pragma warning(disable: 4611)
#endif


/* IMPORTANT: we assume that JOCTET is 8 bits. */
cassert(sizeof(JOCTET) == 1 && CHAR_BIT	== 8);

//-----------------------------------------------------------------------------
// mem source manager
//-----------------------------------------------------------------------------


/* Expanded data source object for memory input */
typedef struct
{
	struct jpeg_source_mgr pub;	/* public fields */
	DynArray* da;
}
SrcMgr;
typedef SrcMgr* SrcPtr;


/*
* Initialize source --- called by jpeg_read_header
* before any data is actually read.
*/

METHODDEF(void) src_init(j_decompress_ptr UNUSED(cinfo))
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

METHODDEF(boolean) src_fill_buffer(j_decompress_ptr cinfo)
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

METHODDEF(void) src_skip_data(j_decompress_ptr cinfo, long num_bytes)
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

METHODDEF(void) src_term(j_decompress_ptr UNUSED(cinfo))
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

GLOBAL(void) src_prepare(j_decompress_ptr cinfo, rpU8 data, size_t size)
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
		sizeof(SrcMgr));
	/* (takes care of raising error if out of memory) */

	src = (SrcPtr)cinfo->src;
	src->pub.init_source       = src_init;
	src->pub.fill_input_buffer = src_fill_buffer;
	src->pub.skip_input_data   = src_skip_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* default */
	src->pub.term_source       = src_term;

	/*
	* fill buffer with everything we have.
	* if fill_input_buffer is called, the buffer was overrun.
	*/
	src->pub.bytes_in_buffer   = size;
	src->pub.next_input_byte   = (JOCTET*)data;
}


//-----------------------------------------------------------------------------
// mem destination manager
//-----------------------------------------------------------------------------

/* Expanded data destination object for memory output */
typedef struct {
	struct jpeg_destination_mgr pub; /* public fields */
	DynArray* da;
} DstMgr;

typedef DstMgr* DstPtr;

// this affects how often dst_empty_output_buffer is called (which
// efficiently expands the DynArray) and how much tail memory we waste
// (not an issue because it is freed immediately after compression).
#define OUTPUT_BUF_SIZE  64*KiB	/* choose an efficiently writeable size */

// note: can't call dst_empty_output_buffer from dst_init or vice versa
// because only the former must advance da->pos.
static void make_room_in_buffer(j_compress_ptr cinfo)
{
	DstPtr dst = (DstPtr)cinfo->dest;
	DynArray* da = dst->da;

	void* start = da->base + da->cur_size;

	if(da_set_size(da, da->cur_size+OUTPUT_BUF_SIZE) != 0)
		ERREXIT(cinfo, JERR_FILE_WRITE);

	dst->pub.next_output_byte = (JOCTET*)start;
	dst->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}


/*
* Initialize destination --- called by jpeg_start_compress
* before any data is actually written.
*/
METHODDEF(void) dst_init(j_compress_ptr cinfo)
{
	make_room_in_buffer(cinfo);
}


/*
* Empty the output buffer --- called whenever buffer fills up.
*
* In typical applications, this should write the entire output buffer
* (ignoring the current state of next_output_byte & free_in_buffer),
* reset the pointer & count to the start of the buffer, and return TRUE
* indicating that the buffer has been dumped.
*
* <snip comments on suspended IO>
*/
METHODDEF(boolean) dst_empty_output_buffer(j_compress_ptr cinfo)
{
	DstPtr dst = (DstPtr)cinfo->dest;
	DynArray* da = dst->da;

	// writing out OUTPUT_BUF_SIZE-dst->pub.free_in_buffer bytes
	// sounds reasonable, but makes for broken output.
	da->pos += OUTPUT_BUF_SIZE;

	make_room_in_buffer(cinfo);

	return TRUE;	// not suspended
}


/*
* Terminate destination --- called by jpeg_finish_compress
* after all data has been written.  Usually needs to flush buffer.
*
* NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
* application must deal with any cleanup that should happen even
* for error exit.
*/
METHODDEF(void) dst_term(j_compress_ptr cinfo)
{
	DstPtr dst = (DstPtr)cinfo->dest;
	DynArray* da = dst->da;

	// account for nbytes left in buffer
	da->pos += OUTPUT_BUF_SIZE - dst->pub.free_in_buffer;
}


/*
* Prepare for output to a buffer.
* The caller is responsible for allocating and writing out to disk after
* compression is complete.
*/

GLOBAL(void) dst_prepare(j_compress_ptr cinfo, DynArray* da)
{
	/* The destination object is made permanent so that multiple JPEG images
	* can be written to the same file without re-executing dst_prepare.
	* This makes it dangerous to use this manager and a different destination
	* manager serially with the same JPEG object, because their private object
	* sizes may be different.  Caveat programmer.
	*/
	if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr*)(*cinfo->mem->alloc_small)
			((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(DstMgr));
	}

	DstPtr dst = (DstPtr)cinfo->dest;
	dst->pub.init_destination    = dst_init;
	dst->pub.empty_output_buffer = dst_empty_output_buffer;
	dst->pub.term_destination    = dst_term;
	dst->da = da;
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

struct JpgErrorMgr
{
	struct jpeg_error_mgr pub;	// "public" fields

	// jump here (back to JPEG lib caller) on error 
	jmp_buf call_site;

	// description of first error encountered; must store in JPEG context
	// for thread safety. initialized in setup_err_mgr.
	char msg[JMSG_LENGTH_MAX];

	JpgErrorMgr(jpeg_compress_struct& cinfo);
	JpgErrorMgr(jpeg_decompress_struct& cinfo);
private:
	void init();
};


METHODDEF(void) err_error_exit(j_common_ptr cinfo)
{
	// get subclass
	JpgErrorMgr* err_mgr = (JpgErrorMgr*)cinfo->err;

	// "output" error message (i.e. store in JpgErrorMgr;
	// call_site is responsible for displaying it via debug_printf)
	(*cinfo->err->output_message)(cinfo);

	// jump back to call site, i.e. jpg_(de|en)code
	longjmp(err_mgr->call_site, 1);
}


// stores message in JpgErrorMgr for later output by jpg_(de|en)code.
// note: don't display message here, so the caller can
//   add some context (whether encoding or decoding, and filename).
METHODDEF(void) err_output_message(j_common_ptr cinfo)
{
	// get subclass
	JpgErrorMgr* err_mgr = (JpgErrorMgr*)cinfo->err;

	// this context already had an error message; don't overwrite it.
	// (subsequent errors probably aren't related to the real problem).
	// note: was set to '\0' by ctor.
	if(err_mgr->msg[0] != '\0')
		return;

	// generate the message and store it
	(*cinfo->err->format_message)(cinfo, err_mgr->msg);
}


void JpgErrorMgr::init()
{
	// fill in pub fields
	jpeg_std_error(&pub);
	// .. and override some methods:
	pub.error_exit = err_error_exit;
	pub.output_message = err_output_message;

	// required for "already have message" check in err_output_message
	msg[0] = '\0';
}

JpgErrorMgr::JpgErrorMgr(jpeg_compress_struct& cinfo)
{
	init();
	// hack: register this error manager with cinfo.
	// must be done before jpeg_create_* in case that fails
	// (unlikely, but possible if out of memory).
	cinfo.err = &pub;
}

JpgErrorMgr::JpgErrorMgr(jpeg_decompress_struct& cinfo)
{
	init();
	// hack: register this error manager with cinfo.
	// must be done before jpeg_create_* in case that fails
	// (unlikely, but possible if out of memory).
	cinfo.err = &pub;
}


//-----------------------------------------------------------------------------


Status TexCodecJpg::transform(Tex* UNUSED(t), size_t UNUSED(transforms)) const
{
	return INFO::TEX_CODEC_CANNOT_HANDLE;
}


// note: jpg_encode and jpg_decode cannot be combined due to
// libjpg interface differences.
// we do split them up into interface and impl to simplify
// resource cleanup and avoid "dtor / setjmp interaction" warnings.
//
// rationale for row array: jpeg won't output more than a few
// scanlines at a time, so we need an output loop anyway. however,
// passing at least 2..4 rows is more efficient in low-quality modes
// due to less copying.


static Status jpg_decode_impl(rpU8 data, size_t size, jpeg_decompress_struct* cinfo, Tex* t)
{
	src_prepare(cinfo, data, size);

	// ignore return value since:
	// - suspension is not possible with the mem data source
	// - we passed TRUE to raise an error if table-only JPEG file
	(void)jpeg_read_header(cinfo, TRUE);

	// set libjpg output format. we cannot go with the default because
	// Photoshop writes non-standard CMYK files that must be converted to RGB.
	size_t flags = 0;
	cinfo->out_color_space = JCS_RGB;
	if(cinfo->num_components == 1)
	{
		flags |= TEX_GREY;
		cinfo->out_color_space = JCS_GRAYSCALE;
	}

	// lower quality, but faster
	cinfo->dct_method = JDCT_IFAST;
	cinfo->do_fancy_upsampling = FALSE;

	// ignore return value since suspension is not possible with the
	// mem data source.
	// note: since we've set out_color_space, JPEG will always
	// return an acceptable image format; no need to check.
	(void)jpeg_start_decompress(cinfo);

	// scaled output image dimensions and final bpp are now available.
	int w = cinfo->output_width;
	int h = cinfo->output_height;
	int bpp = cinfo->output_components * 8;

	// alloc destination buffer
	const size_t pitch = w * bpp / 8;
	const size_t imgSize = pitch * h;	// for allow_rows
	shared_ptr<u8> img;
	AllocateAligned(img, imgSize, pageSize);

	// read rows
	std::vector<RowPtr> rows = tex_codec_alloc_rows(img.get(), h, pitch, TEX_TOP_DOWN, 0);
	// could use cinfo->output_scanline to keep track of progress,
	// but we need to count lines_left anyway (paranoia).
	JSAMPARRAY row = (JSAMPARRAY)&rows[0];
	JDIMENSION lines_left = h;
	while(lines_left != 0)
	{
		JDIMENSION lines_read = jpeg_read_scanlines(cinfo, row, lines_left);
		row += lines_read;
		lines_left -= lines_read;

		// we've decoded in-place; no need to further process
	}

	// ignore return value since suspension is not possible with the
	// mem data source.
	(void)jpeg_finish_decompress(cinfo);

	Status ret = INFO::OK;
	if(cinfo->err->num_warnings != 0)
		ret = WARN::TEX_INVALID_DATA;

	// store image info and validate
	return ret | t->wrap(w,h,bpp,flags,img,0);
}


static Status jpg_encode_impl(Tex* t, jpeg_compress_struct* cinfo, DynArray* da)
{
	dst_prepare(cinfo, da);

	// describe image format
	// required:
	cinfo->image_width = (JDIMENSION)t->m_Width;
	cinfo->image_height = (JDIMENSION)t->m_Height;
	cinfo->input_components = (int)t->m_Bpp / 8;
	cinfo->in_color_space = (t->m_Bpp == 8)? JCS_GRAYSCALE : JCS_RGB;
	// defaults depend on cinfo->in_color_space already having been set!
	jpeg_set_defaults(cinfo);
	// (add optional settings, e.g. quality, here)

	// TRUE ensures that we will write a complete interchange-JPEG file.
	// don't change unless you are very sure of what you're doing.
	jpeg_start_compress(cinfo, TRUE);

	// if BGR, convert to RGB.
	WARN_IF_ERR(t->transform_to(t->m_Flags & ~TEX_BGR));

	const size_t pitch = t->m_Width * t->m_Bpp / 8;
	u8* data = t->get_data();
	std::vector<RowPtr> rows = tex_codec_alloc_rows(data, t->m_Height, pitch, t->m_Flags, TEX_TOP_DOWN);

	// could use cinfo->output_scanline to keep track of progress,
	// but we need to count lines_left anyway (paranoia).
	JSAMPARRAY row = (JSAMPARRAY)&rows[0];
	JDIMENSION lines_left = (JDIMENSION)t->m_Height;
	while(lines_left != 0)
	{
		JDIMENSION lines_read = jpeg_write_scanlines(cinfo, row, lines_left);
		row += lines_read;
		lines_left -= lines_read;

		// we've decoded in-place; no need to further process
	}

	jpeg_finish_compress(cinfo);

	Status ret = INFO::OK;
	if(cinfo->err->num_warnings != 0)
		ret = WARN::TEX_INVALID_DATA;

	return ret;
}



bool TexCodecJpg::is_hdr(const u8* file) const
{
	// JFIF requires SOI marker at start of stream.
	// we compare single bytes to be endian-safe.
	return (file[0] == 0xff && file[1] == 0xd8);
}


bool TexCodecJpg::is_ext(const OsPath& extension) const
{
	return extension == L".jpg" || extension == L".jpeg";
}


size_t TexCodecJpg::hdr_size(const u8* UNUSED(file)) const
{
	return 0;	// libjpg returns decoded image data; no header
}


Status TexCodecJpg::decode(rpU8 data, size_t size, Tex* RESTRICT t) const
{
	// contains the JPEG decompression parameters and pointers to
	//  working space (allocated as needed by the JPEG library).
	struct jpeg_decompress_struct cinfo;

	JpgErrorMgr jerr(cinfo);
	if(setjmp(jerr.call_site))
		return ERR::FAIL;

	jpeg_create_decompress(&cinfo);

	Status ret = jpg_decode_impl(data, size, &cinfo, t);

	jpeg_destroy_decompress(&cinfo); // releases a "good deal" of memory

	return ret;
}


// limitation: palette images aren't supported
Status TexCodecJpg::encode(Tex* RESTRICT t, DynArray* RESTRICT da) const
{
	// contains the JPEG compression parameters and pointers to
	// working space (allocated as needed by the JPEG library).
	struct jpeg_compress_struct cinfo;
	
	JpgErrorMgr jerr(cinfo);
	if(setjmp(jerr.call_site))
		WARN_RETURN(ERR::FAIL);

	jpeg_create_compress(&cinfo);

	Status ret = jpg_encode_impl(t, &cinfo, da);

	jpeg_destroy_compress(&cinfo); // releases a "good deal" of memory

	return ret;
}
