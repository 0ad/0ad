// virtual file system - transparent access to files in archives;
// allows multiple mount points
//
// Copyright (c) 2004 Jan Wassenberg
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
#include "posix.h"	// struct stat

// the VFS doesn't require this length restriction - VFS internal storage
// is not fixed-length. the purpose here is to allow fixed-sized path buffers
// allocated on the stack.
//
// length includes trailing '\0'.
#define VFS_MAX_PATH 256

extern int vfs_mount(const char* vfs_mount_point, const char* name, uint pri);
extern int vfs_umount(const char* name);

extern int vfs_stat(const char* fn, struct stat*);
extern int vfs_realpath(const char* fn, char* realpath);

extern Handle vfs_load(const char* fn, void*& p, size_t& size);

extern Handle vfs_open(const char* fn, uint flags = 0);
extern int vfs_close(Handle& h);

extern Handle vfs_map(Handle hf, uint flags, void*& p, size_t& size);


struct vfsDirEnt
{
	// the filename is currently stored internally as std::string. returning as char* for C compat
	// stored internally as std::string. returning as char* for C compat
	// would mean we have to return a copy. we try to avoid fixed-size
	// buffers, so that leaves a reference.
	const char* name;
};


extern Handle vfs_open_dir(const char* path);
extern int vfs_close_dir(Handle& hd);
extern int vfs_next_dirent(Handle hd, vfsDirEnt* ent, const char* filter);




extern int vfs_rebuild();




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