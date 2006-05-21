/**
 * =========================================================================
 * File        : file_cache.h
 * Project     : 0 A.D.
 * Description : cache for entire files and I/O blocks. also allocates
 *             : file buffers, allowing zero-copy I/O.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2006 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef FILE_CACHE_H__
#define FILE_CACHE_H__

struct BlockId
{
	const char* atom_fn;
	u32 block_num;
};

extern bool block_eq(BlockId b1, BlockId b2);

// create an id for use with the cache that uniquely identifies
// the block from the file <atom_fn> starting at <ofs>.
extern BlockId block_cache_make_id(const char* atom_fn, const off_t ofs);

extern void* block_cache_alloc(BlockId id);

extern void block_cache_mark_completed(BlockId id);

extern void* block_cache_find(BlockId id);
extern void block_cache_release(BlockId id);




// interpret file_io parameters (pbuf, size, flags, cb) and allocate a
// file buffer if necessary.
// called by file_io and afile_read.
extern LibError file_io_get_buf(FileIOBuf* pbuf, size_t size,
	const char* atom_fn, uint flags, FileIOCB cb);

// inform us that the buffer address will be increased by <padding>-bytes.
// this happens when reading uncompressed files from archive: they
// start at unaligned offsets and file_io rounds offset down to
// next block boundary. the buffer therefore starts with padding, which
// is skipped so the user only sees their data.
// we make note of the new buffer address so that it can be freed correctly
// by passing the new padded buffer.
extern void file_buf_add_padding(FileIOBuf exact_buf, size_t exact_size, size_t padding);

// if buf is not in extant list, complain; otherwise, mark it as
// coming from the file <atom_fn>.
// this is needed in the following case: uncompressed reads from archive
// boil down to a file_io of the archive file. the buffer is therefore
// tagged with the archive filename instead of the desired filename.
// afile_read sets things right by calling this.
extern LibError file_buf_set_real_fn(FileIOBuf buf, const char* atom_fn);

// if file_cache_add-ing the given buffer, would it be added?
// this is referenced by trace_entry_causes_io; see explanation there.
extern bool file_cache_would_add(size_t size, const char* atom_fn,
	uint file_flags);

// "give" <buf> to the cache, specifying its size and owner filename.
// since this data may be shared among users of the cache, it is made
// read-only (via MMU) to make sure no one can corrupt/change it.
//
// note: the reference added by file_buf_alloc still exists! it must
// still be file_buf_free-d after calling this.
extern LibError file_cache_add(FileIOBuf buf, size_t size,
	const char* atom_fn, uint file_flags);



// check if the contents of the file <atom_fn> are in file cache.
// if not, return 0; otherwise, return buffer address and optionally
// pass back its size.
//
// note: does not call stats_cache because it does not know the file size
// in case of cache miss! doing so is left to the caller.
extern FileIOBuf file_cache_retrieve(const char* atom_fn, size_t* psize, uint fb_flags = 0);

// invalidate all data loaded from the file <fn>. this ensures the next
// load of this file gets the (presumably new) contents of the file,
// not previous stale cache contents.
// call after hotloading code detects file has been changed.
extern LibError file_cache_invalidate(const char* P_fn);

// reset entire state of the file cache to what it was after initialization.
// that means completely emptying the extant list and cache.
// used after simulating cache operation, which fills the cache with
// invalid data.
extern void file_cache_reset();

extern void file_cache_init();
extern void file_cache_shutdown();

#endif	// #ifndef FILE_CACHE_H__
