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
 * IO manager: splits requests into asynchronously issued blocks,
 * thus simplifying caching and enabling periodic callbacks.
 */

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
 * this is useful for user progress notification or processing data while
 * waiting for the next I/O to complete (without the complexity of threads).
 **/
typedef LibError (*IoCallback)(uintptr_t cbData, const u8* block, size_t blockSize);

LIB_API LibError io_Scan(const PFile& file, off_t ofs, off_t size, IoCallback cb, uintptr_t cbData);

LIB_API LibError io_Read(const PFile& file, off_t ofs, u8* alignedBuf, off_t size, u8*& data, IoCallback cb = 0, uintptr_t cbData = 0);

LIB_API LibError io_WriteAligned(const PFile& file, off_t alignedOfs, const u8* alignedData, off_t size, IoCallback cb = 0, uintptr_t cbData = 0);
LIB_API LibError io_ReadAligned(const PFile& file, off_t alignedOfs, u8* alignedBuf, off_t size, IoCallback cb = 0, uintptr_t cbData = 0);

#endif	// #ifndef INCLUDED_IO
