#include "precompiled.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* this is not a core library module, so it doesn't define JPEG_INTERNALS */
#include "jinclude.h"
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

METHODDEF(void) init_source(j_decompress_ptr cinfo)
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

METHODDEF(void) term_source(j_decompress_ptr cinfo)
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

	/* first time for this JPEG object? */
	if(!cinfo->src)
		cinfo->src = (struct jpeg_source_mgr*)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				SIZEOF(MemSrcMgr));
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

#ifdef __cplusplus
}
#endif