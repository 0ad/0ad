/*
 * jmemdatasrc.c (adapted from IJG jdatasrc.c)
 *
 * Copyright (C) 2004, Jan Wassenberg.
 * Copyright (C) 1994-1996, Thomas G. Lane.
 *
 * This file contains decompression data source routines for the case of
 * reading JPEG data from a single memory buffer. Suspension isn't used.
 *
 * IMPORTANT: we assume that JOCTET is 8 bits.
 */

#include "precompiled.h"

#include "lib.h"
#include "lib/res/file/vfs.h"


// must come before jpeg-6b headers.
#ifdef __cplusplus
extern "C" {
#endif

/* this is not a core library module, so it doesn't define JPEG_INTERNALS */
#include "jpeglib.h"
#include "jerror.h"


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






















#ifdef __cplusplus
}
#endif
