// virtual file system - transparent access to files in archives;
// allows multiple search paths
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

#ifndef __VFS_H__
#define __VFS_H__

#include "h_mgr.h"
#include "posix.h"

#define VFS_MAX_PATH 63

extern int vfs_set_root(const char* argv0, const char* root);
extern int vfs_mount(const char* path);
extern int vfs_umount(const char* path);

extern int vfs_stat(const char* fn, struct stat *buffer);
extern int vfs_realpath(const char* fn, char* realpath);

extern Handle vfs_load(const char* fn, void*& p, size_t& size);

extern Handle vfs_open(const char* fn, int flags = 0);
extern int vfs_close(Handle& h);

extern Handle vfs_map(Handle hf, int flags, void*& p, size_t& size);

//
// async read interface
//

extern Handle vfs_start_read(const Handle hf, size_t ofs, size_t& advance, void* buf);
extern int vfs_wait_read(Handle hr, void*& p, size_t& size);
extern int vfs_discard_read(Handle& hr);

extern ssize_t vfs_io(Handle hf, size_t ofs, size_t size, void*& p);


// keep in sync with File flags!

enum
{
	VFS_WRITE = 1,			// write-only access; otherwise, read only
	VFS_MEM_READONLY = 2,	// !want to be able to change in memory data
	VFS_NOCACHE = 4,		// don't cache whole file, e.g. if cached on a higher level
	VFS_RANDOM = 8			// random access hint, allow offset
};


#endif	// #ifndef __VFS_H__