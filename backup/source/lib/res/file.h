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
	int flags;
	size_t size;

	u32 fn_hash;

	void* mapping;
	int fd;
};


enum
{
	// open for writing
	FILE_WRITE        = 1,

	FILE_MEM_READONLY = 2,

	// do not cache any part of the file
	// (e.g. if caching on a higher level)
	FILE_NO_CACHE     = 4


};


extern int file_open(const char* path, int flags, File* f);
extern int file_close(File* f);

extern int file_map(File* f, void*& p, size_t& size);
extern int file_unmap(File* f);

extern Handle file_start_io(File* f, size_t ofs, size_t size, void* buf);
extern int file_wait_io(const Handle hio, void*& p, size_t& size);
extern int file_discard_io(Handle& hio);



// return value:
// < 0: failed (do not call again) - abort transfer.
// = 0: done (no more data needed) - stop transfer gracefully.
// > 0: bytes output (not used ATM; useful for statistics) - continue.
typedef ssize_t(*FILE_IO_CB)(uintptr_t ctx, void* p, size_t size);

extern ssize_t file_io(File* f, size_t ofs, size_t size, void** p,
	FILE_IO_CB cb = 0, uintptr_t ctx = 0);


#endif	// #ifndef FILE_H