/**
 * =========================================================================
 * File        : compression.h
 * Project     : 0 A.D.
 * Description : interface for compressing/decompressing data streams.
 *             : currently implements "deflate" (RFC1951).
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_COMPRESSION
#define INCLUDED_COMPRESSION


namespace ERR
{
	const LibError COMPRESSION_UNKNOWN_METHOD = -110300;
}

enum ContextType
{
	CT_COMPRESSION,
	CT_DECOMPRESSION
};

enum CompressionMethod
{
	CM_NONE,

	// zlib "deflate" (RFC 1750, 1751) and CRC32
	CM_DEFLATE,

	CM_UNSUPPORTED
};

/**
 * allocate a new compression/decompression context.
 **/
extern uintptr_t comp_alloc(ContextType type, CompressionMethod method);

/**
 * free this context and all associated memory.
 **/
extern void comp_free(uintptr_t ctx);

/**
 * clear all previous state and prepare for reuse.
 *
 * this is as if the object were destroyed and re-created, but more
 * efficient since it avoids reallocating a considerable amount of memory
 * (about 200KB for LZ).
 **/
extern void comp_reset(uintptr_t ctx);

/**
 * @return an upper bound on the output size for the given amount of input.
 * this is used when allocating a single buffer for the whole operation.
 **/
extern size_t comp_max_output_size(uintptr_t ctx, size_t inSize);

/**
 * set output buffer for subsequent comp_feed() calls.
 *
 * due to the comp_finish interface, output buffers must be contiguous or
 * identical (otherwise IsAllowableOutputBuffer will complain).
 **/
extern void comp_set_output(uintptr_t ctx, u8* out, size_t outSize);

/**
 * allocate a new output buffer.
 *
 * @param size [bytes] to allocate.
 *
 * if a buffer had previously been allocated and is large enough, it is
 * reused (this reduces the number of allocations). the buffer is
 * automatically freed by comp_free.
 **/
extern LibError comp_alloc_output(uintptr_t ctx, size_t inSize);

/**
 * 'feed' the given buffer to the compressor/decompressor.
 *
 * @return number of output bytes produced or a negative LibError.
 * note that 0 is a legitimate return value - this happens if the input
 * buffer is small and the codec hasn't produced any output.
 *
 * note: after this call returns, the buffer may be overwritten or freed;
 * we take care of copying and queuing any data that remains (e.g. due to
 * lack of output buffer space).
 **/
extern ssize_t comp_feed(uintptr_t ctx, const u8* in, size_t inSize);

/**
 * conclude the compression/decompression operation.
 *
 * @param out, outSize receive the output buffer. this assumes identical or
 * contiguous addresses were passed, which comp_set_output ensures.
 * @param checksum
 *
 * note: this must always be called (even if the output buffer is already
 * known) because it feeds any remaining queued input buffers.
 **/
extern LibError comp_finish(uintptr_t ctx, u8** out, size_t* out_size, u32* checksum);

/**
 * update a checksum to reflect the contents of a buffer.
 *
 * @param checksum the initial value (must be 0 on first call)
 * @return the new checksum.
 *
 * note: this routine is stateless but still requires a context to establish
 * the type of checksum to calculate. the results are the same as yielded by
 * comp_finish after comp_feed-ing all input buffers.
 **/
extern u32 comp_update_checksum(uintptr_t ctx, u32 checksum, const u8* in, size_t inSize);

#endif	// #ifndef INCLUDED_COMPRESSION
