/**
 * =========================================================================
 * File        : io.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_IO
#define INCLUDED_IO

#include "lib/file/file.h"

// memory will be allocated from the heap, not the (limited) file cache.
// this makes sense for write buffers that are never used again,
// because we avoid having to displace some other cached items.
LIB_API shared_ptr<u8> io_Allocate(size_t size, off_t ofs = 0);

/**
 * called after a block IO has completed.
 *
 * @return INFO::CB_CONTINUE to continue; any other value will cause the
 * IO splitter to abort immediately and return that.
 *
 * this is useful for interleaving e.g. decompression with IOs.
 **/
typedef LibError (*IoCallback)(uintptr_t cbData, const u8* block, size_t blockSize);

LIB_API LibError io_Scan(const PIFile& file, off_t ofs, off_t size, IoCallback cb, uintptr_t cbData);

LIB_API LibError io_Read(const PIFile& file, off_t ofs, u8* alignedBuf, size_t size, u8*& data);

LIB_API LibError io_WriteAligned(const PIFile& file, off_t alignedOfs, const u8* alignedData, size_t size);
LIB_API LibError io_ReadAligned(const PIFile& file, off_t alignedOfs, u8* alignedBuf, size_t size);

#endif	// #ifndef INCLUDED_IO
