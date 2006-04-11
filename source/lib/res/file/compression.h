/**
 * =========================================================================
 * File        : compression.h
 * Project     : 0 A.D.
 * Description : interface for compressing/decompressing data streams.
 *             : currently implements "deflate" (RFC1951).
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef COMPRESSION_H__
#define COMPRESSION_H__

enum ContextType
{
	CT_COMPRESSION,
	CT_DECOMPRESSION
};

enum CompressionMethod
{
	CM_NONE,

	// zlib "deflate" - see RFC 1750, 1751.
	CM_DEFLATE,

	CM_UNSUPPORTED
};

extern uintptr_t comp_alloc(ContextType type, CompressionMethod method);

// set output buffer. all subsequent comp_feed() calls will write into it.
// should only be called once (*) due to the comp_finish() interface - since
// that allows querying the output buffer, it must not be fragmented.
// * the previous output buffer is wiped out by comp_reset, so
// setting it again (once!) after that is allowed and required.
extern void comp_set_output(uintptr_t ctx, void* out, size_t out_size);

// [compression contexts only:] allocate an output buffer big enough to
// hold worst_case_compression_ratio*in_size bytes.
// rationale: this interface is useful because callers cannot
// reliably estimate how much output space is needed.
// raises a warning for decompression contexts because this operation
// does not make sense there:
// - decompression ratio is quite large - ballpark 1000x;
// - exact uncompressed size is known to caller (via archive file header).
// note: buffer is held until comp_free; it can be reused after a
// comp_reset. this reduces malloc/free calls.
extern LibError comp_alloc_output(uintptr_t ctx, size_t in_size);

// get current position in output buffer.
// precondition: valid calls to EITHER comp_alloc_output OR comp_set_output.
extern void* comp_get_output(uintptr_t ctx);

// 'feed' the given buffer to the compressor/decompressor.
// returns number of output bytes produced (*), or a negative LibError code.
// * 0 is a legitimate return value - this happens if the input buffer is
// small and the codec hasn't produced any output.
// note: the buffer may be overwritten or freed immediately after - we take
// care of copying and queuing any data that remains (e.g. due to
// lack of output buffer space).
extern ssize_t comp_feed(uintptr_t ctx, const void* in, size_t in_size);

// feed any remaining queued input data, finish the compress/decompress and
// pass back the output buffer.
extern LibError comp_finish(uintptr_t ctx, void** out, size_t* out_size);

// prepare this context for reuse. the effect is similar to freeing this
// context and creating another.
// rationale: this API avoids reallocating a considerable amount of
// memory (ballbark 200KB LZ window plus output buffer).
extern LibError comp_reset(uintptr_t ctx);

// free this context and all associated memory.
extern void comp_free(uintptr_t ctx);

#endif	// #ifndef COMPRESSION_H__
