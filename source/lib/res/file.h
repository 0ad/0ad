// file layer on top of POSIX.
// provides streaming support and caching.
//
// Copyright (c) 2004 Jan Wassenberg
//
// This file is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef FILE_H
#define FILE_H

#include "h_mgr.h"
#include "lib.h"


struct File
{
#ifdef PARANOIA
	u32 magic;
#endif

	// keep offset of flags and size members in sync with struct ZFile!
	// it is accessed by VFS and must be the same for both (union).
	// dirty, but necessary because VFile is pushing the HDATA size limit.
	uint flags;
	off_t size;

	u32 fn_hash;

	void* mapping;
	uint map_refs;

	int fd;
};


enum
{
	// write-only access; otherwise, read only
	FILE_WRITE        = 0x01,

	
	// the file's contents aren't cached at a higher level; do so here.
	// we keep the file open (until the cache is "full enough"). if it
	// is loaded, we keep the buffer there to satisfy later loads.
	FILE_CACHE        = 0x04,

	// random access hint
//	FILE_RANDOM       = 0x08,

	FILE_NO_AIO       = 0x10
};


// keep in sync with zip.cpp and vfs.cpp *_CB_FLAGS!
enum FILE_CB_FLAGS
{
	// location
	LOC_DIR = BIT(0)
};


// convert to/from our portable path representation,
// e.g. for external libraries that require the real filename.
// note: also removes/adds current directory.
extern int file_make_native_path(const char* path, char* n_path);
extern int file_make_portable_path(const char* n_path, char* path);


// set current directory to rel_path, relative to the path to the executable,
// which is taken from argv0.
//
// example: executable in "$install_dir/system"; desired root dir is
// "$install_dir/data" => rel_path = "../data".
//
// this is necessary because the current directory is unknown at startup
// (e.g. it isn't set when invoked via batch file), and this is the
// easiest portable way to find our install directory.
//
// can only be called once, by design (see source). rel_path is trusted.
extern int file_rel_chdir(const char* argv0, const char* rel_path);


typedef int(*FileCB)(const char* const name, const uint flags, const ssize_t size, const uintptr_t user);

// not recursive - only the files in <dir>!
extern int file_enum(const char* dir, FileCB cb, uintptr_t user);

extern int file_stat(const char* path, struct stat*);

extern int file_open(const char* fn, uint flags, File* f);
extern int file_close(File* f);


//
// memory mapping
//

// map the entire file <f> into memory. if already currently mapped,
// return the previous mapping (reference-counted).
// output parameters are zeroed on failure.
//
// the mapping will be removed (if still open) when its file is closed.
// however, map/unmap calls should still be paired so that the mapping
// may be removed when no longer needed.
//
// rationale: reference counting is required for zip_map: several
// Zip "mappings" each reference one ZArchive's actual file mapping.
// implement it here so that we also get refcounting for normal files.
extern int file_map(File* f, void*& p, size_t& size);

// decrement the reference count for the mapping belonging to file <f>.
// fail if there are no references; remove the mapping if the count reaches 0.
//
// the mapping will be removed (if still open) when its file is closed.
// however, map/unmap calls should still be paired so that the mapping
// may be removed when no longer needed.
extern int file_unmap(File* f);


//
// async IO
//

extern Handle file_start_io(File* f, off_t ofs, size_t size, void* buf);
extern int file_wait_io(const Handle hio, void*& p, size_t& size);
extern int file_discard_io(Handle& hio);



// return value:
// < 0: failed (do not call again) - abort transfer.
// = 0: done (no more data needed) - stop transfer gracefully.
// > 0: bytes output (not used ATM; useful for statistics) - continue.
typedef ssize_t(*FILE_IO_CB)(uintptr_t ctx, void* p, size_t size);

extern ssize_t file_io(File* f, off_t ofs, size_t size, void** p,
	FILE_IO_CB cb = 0, uintptr_t ctx = 0);


#endif	// #ifndef FILE_H
