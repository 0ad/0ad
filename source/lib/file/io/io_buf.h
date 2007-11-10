/**
 * =========================================================================
 * File        : io_buf.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_IO_BUF
#define INCLUDED_IO_BUF

typedef const u8* IoBuf;
const IoBuf IO_BUF_TEMP = 0;
const IoBuf IO_BUF_ALLOC = (IoBuf)1;

// memory will be allocated from the heap, not the (limited) file cache.
// this makes sense for write buffers that are never used again,
// because we avoid having to displace some other cached items.
extern IoBuf io_buf_Allocate(size_t size);
extern void io_buf_Deallocate(IoBuf buf, size_t size);

#endif	// #ifndef INCLUDED_IO_BUF
