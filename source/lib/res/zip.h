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

#ifndef ZIP_H__
#define ZIP_H__

#include "h_mgr.h"
#include "file.h"	// FileCB for zip_enum


// note: filenames are case-insensitive.


//
// archive
//

// open and return a handle to the zip archive indicated by <fn>
extern Handle zip_archive_open(const char* fn);

// close the archive <ha> and set ha to 0
extern int zip_archive_close(Handle& ha);

// for all files in archive <ha>: call <cb>, passing <user>
// and the entries's complete path.
extern int zip_enum(const Handle ha, const FileCB cb, const uintptr_t user);


//
// file
//

struct ZFile
{
#ifdef PARANOIA
	u32 magic;
#endif

	// keep offset of flags and size members in sync with struct File!
	// it is accessed by VFS and must be the same for both (union).
	// dirty, but necessary because VFile is pushing the HDATA size limit.
	uint flags;
	size_t ucsize;
		// size of logical file

	off_t ofs;
	off_t csize;
	off_t last_raw_ofs;

	Handle ha;
	uintptr_t inf_ctx;
};

// return file information for <fn> in archive <ha>
extern int zip_stat(Handle ha, const char* fn, struct stat* s);

// open the file <fn> in archive <ha>, and fill *zf with information about it.
extern int zip_open(Handle ha, const char* fn, ZFile* zf);

// close the file <zf>
extern int zip_close(ZFile* zf);


//
// memory mapping
//

// map the entire file <zf> into memory. mapping compressed files
// isn't allowed, since the compression algorithm is unspecified.
// output parameters are zeroed on failure.
//
// the mapping will be removed (if still open) when its archive is closed.
// however, map/unmap calls should still be paired so that the archive mapping
// may be removed when no longer needed.
extern int zip_map(ZFile* zf, void*& p, size_t& size);

// remove the mapping of file <zf>; fail if not mapped.
//
// the mapping will be removed (if still open) when its archive is closed.
// however, map/unmap calls should be paired so that the archive mapping
// may be removed when no longer needed.
extern int zip_unmap(ZFile* zf);


//
// asynchronous read
//

// currently only supported for compressed files to keep things simple.
// see rationale in source.

// begin transferring <size> bytes, starting at <ofs>. get result
// with zip_wait_io; when no longer needed, free via zip_discard_io.
extern int zip_start_io(ZFile* const zf, off_t ofs, size_t size, void* buf, FileIO* io);

// indicates if the IO referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
extern int zip_io_complete(FileIO io);

// wait until the transfer <hio> completes, and return its buffer.
// output parameters are zeroed on error.
extern int zip_wait_io(FileIO io, void*& p, size_t& size);

// finished with transfer <hio> - free its buffer (returned by vfs_wait_io)
extern int zip_discard_io(FileIO io);


//
// synchronous read
//

// read from file <zf>, starting at offset <ofs> in the compressed data.
extern ssize_t zip_read(ZFile* zf, off_t ofs, size_t size, void** p, FileIOCB cb = 0, uintptr_t ctx = 0);


#endif	// #ifndef ZIP_H__
