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
// in-memory inflate routines (zlib wrapper)
//

extern uintptr_t inf_init_ctx();

extern int inf_start_read(uintptr_t ctx, void* out, size_t out_size);

extern ssize_t inf_inflate(uintptr_t ctx, void* in, size_t in_size);

extern int inf_finish_read(uintptr_t ctx);

extern int inf_free_ctx(uintptr_t ctx);


//
// archive
//

// open and return a handle to the zip archive indicated by <fn>
extern Handle zip_archive_open(const char* fn);

// close the archive <ha> and set ha to 0
extern int zip_archive_close(Handle& ha);


// keep in sync with file.cpp and vfs.cpp *_CB_FLAGS
enum ZIP_CB_FLAGS
{
	LOC_ZIP = BIT(1)
};

// all files in archive!
typedef int(*ZipFileCB)(const char* const fn, const uint flags, const ssize_t size, const uintptr_t user);
extern int zip_enum(const Handle ha, const ZipFileCB cb, const uintptr_t user);

//
// file
//

struct ZFile
{
#ifdef PARANOIA
	u32 magic;
#endif

	// keep offset of flags and size members in sync with struct ZFile!
	// it is accessed by VFS and must be the same for both (union).
	// dirty, but necessary because VFile is pushing the HDATA size limit.
	int flags;
	size_t ucsize;
		// size of logical file

	off_t ofs;
	off_t csize;
	off_t last_raw_ofs;

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
extern int zip_unmap(ZFile* zf);

// read from file <zf>, starting at offset <ofs> in the compressed data
extern ssize_t zip_read(ZFile* zf, off_t ofs, size_t size, void*& p);


#endif	// #ifndef __ZIP_H__
