/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
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
 * this is useful for interleaving e.g. decompression with IOs.
 **/
typedef LibError (*IoCallback)(uintptr_t cbData, const u8* block, size_t blockSize);

LIB_API LibError io_Scan(const PFile& file, off_t ofs, off_t size, IoCallback cb, uintptr_t cbData);

LIB_API LibError io_Read(const PFile& file, off_t ofs, u8* alignedBuf, off_t size, u8*& data);

LIB_API LibError io_WriteAligned(const PFile& file, off_t alignedOfs, const u8* alignedData, off_t size);
LIB_API LibError io_ReadAligned(const PFile& file, off_t alignedOfs, u8* alignedBuf, off_t size);

#endif	// #ifndef INCLUDED_IO
