// Zip archiving on top of ZLib.
//
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef __ZIP_H__
#define __ZIP_H__

#include "h_mgr.h"
#include "lib.h"
#include "file.h"


//
// low-level in-memory inflate routines
//

extern uintptr_t zip_init_ctx();

extern int zip_start_read(uintptr_t ctx, void* out, size_t out_size);

extern ssize_t zip_inflate(uintptr_t ctx, void* in, size_t in_size);

extern int zip_finish_read(uintptr_t ctx);

extern int zip_free_ctx(uintptr_t ctx);


//
// archive
//

// open and return a handle to the zip archive indicated by <fn>
extern Handle zip_archive_open(const char* fn);

// close the archive <ha> and set ha to 0
extern int zip_archive_close(Handle& ha);


//
// file
//

struct ZFile
{
#ifdef PARANOIA
	u32 magic;
#endif

	size_t ofs;
	size_t csize;
	size_t ucsize;

	size_t last_raw_ofs;

	Handle ha;
	uintptr_t read_ctx;
};

// return file information for <fn> (may include path) in archive <ha>
extern int zip_stat(Handle ha, const char* fn, struct stat* s);

// open the file <fn> in archive <ha>, and fill *zf with information about it.
extern int zip_open(Handle ha, const char* fn, ZFile* zf);

// close the file <zf>
extern int zip_close(ZFile* zf);


extern int zip_map(ZFile* zf, void*& p, size_t& size);

extern ssize_t zip_read(ZFile* zf, size_t ofs, size_t size, void*& p);


//--------

// read from file <hz>, starting at offset <ofs> in the compressed data
// (typically only used if the file is known to be stored).
// p == 0: allocate, read into, and return the output buffer
// p != 0: read into given output buffer, return handle to it
//         if file is compressed, size must be >= uncompressed file size
// size: no input value, unless specifying an output buffer (p != 0)
//       out: 




//extern void* zip_mmap(








#endif	// #ifndef __ZIP_H__
