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

#include "posix.h"		// struct stat


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

	FILE_NO_AIO       = 0x10,

	FILE_CACHE_BLOCK  = 0x20
};


// is s2 a subpath of s1, or vice versa? used by VFS and wdir_watch.
// works for portable and native paths.
extern bool file_is_subpath(const char* s1, const char* s2);


//
// path conversion functions (native <--> portable),
// for external libraries that require the real filename.
//
// replaces '/' with platform's directory separator and vice versa.
// verifies path length < PATH_MAX (otherwise return ERR_PATH_LENGTH).
//

// relative paths (relative to root, established with file_rel_chdir)
extern int file_make_native_path(const char* path, char* n_path);
extern int file_make_portable_path(const char* n_path, char* path);

// as above, but with full native paths (portable paths are always relative).
// prepends current directory, resp. makes sure it matches the given path.
extern int file_make_full_native_path(const char* path, char* n_full_path);
extern int file_make_full_portable_path(const char* n_full_path, char* path);


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


// called by file_enum for each entry in the directory.
// name doesn't include path!
// return non-zero to immediately abort; file_enum will return that value.
typedef int(*FileCB)(const char* name, const struct stat* s, const uintptr_t user);

// call <cb> for each file and subdirectory in <dir> (alphabetical order),
// passing the entry name (not full path!), stat info, and <user>.
//
// first builds a list of entries (sorted) and remembers if an error occurred.
// if <cb> returns non-zero, abort immediately and return that; otherwise,
// return first error encountered while listing files, or 0 on success.
extern int file_enum(const char* dir, FileCB cb, uintptr_t user);

// get file status. output param is zeroed on error.
extern int file_stat(const char* path, struct stat*);

extern int file_open(const char* fn, uint flags, File* f);
extern int file_close(File* f);


// remove all blocks loaded from the file <fn>. used when reloading the file.
extern int file_invalidate_cache(const char* fn);


//
// asynchronous IO
//

struct FileIO
{
	void* cb;
};

extern int file_start_io(File* f, off_t ofs, size_t size, void* buf, FileIO* io);

// indicates if the given IO has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
extern int file_io_complete(FileIO* io);

extern int file_wait_io(FileIO* io, void*& p, size_t& size);

extern int file_discard_io(FileIO* io);


//
// synchronous IO
//

// return value:
// < 0: failed; abort transfer.
// >= 0: bytes output; continue.
typedef ssize_t(*FileIOCB)(uintptr_t ctx, void* p, size_t size);

// transfer <size> bytes, starting at <ofs>, to/from the given file.
// (read or write access was chosen at file-open time).
//
// if non-NULL, <cb> is called for each block transferred, passing <ctx>.
// it returns how much data was actually transferred, or a negative error
// code (in which case we abort the transfer and return that value).
// the callback mechanism is useful for user progress notification or
// processing data while waiting for the next I/O to complete
// (quasi-parallel, without the complexity of threads).
//
// return number of bytes transferred (see above), or a negative error code.
extern ssize_t file_io(File* f, off_t ofs, size_t size, void* buf, FileIOCB cb = 0, uintptr_t ctx = 0);


//
// memory mapping
//

// useful for files that are too large to be loaded into memory,
// or if only (non-sequential) portions of a file are needed at a time.


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

#endif	// #ifndef FILE_H
