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
	CM_DEFLATE
};

extern uintptr_t comp_alloc(ContextType type, CompressionMethod method);

extern void comp_set_output(uintptr_t ctx, void* out, size_t out_size);
extern LibError comp_alloc_output(uintptr_t c_, size_t in_size);

extern void* comp_get_output(uintptr_t ctx_);

extern ssize_t comp_feed(uintptr_t ctx, const void* in, size_t in_size);

extern LibError comp_finish(uintptr_t ctx, void** out, size_t* out_size);

extern void comp_free(uintptr_t ctx);

#endif	// #ifndef COMPRESSION_H__
