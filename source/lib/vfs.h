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

#include "res.h"

#define VFS_MAX_PATH 40

extern int vfs_set_root(const char* argv0, const char* root);
extern int vfs_mount(const char* path);
extern int vfs_umount(const char* path);

extern int vfs_stat(const char* fn, struct stat *buffer);
extern int vfs_realpath(const char* fn, char* realpath);

extern Handle vfs_load(const char* fn, void*& p, size_t& size, bool dont_map = false);

extern Handle vfs_open(const char* fn);
extern int vfs_close(Handle h);
extern u32 vfs_start_read(Handle hf, size_t& ofs, void** buf = 0);
extern int vfs_finish_read(u32 slot, void*& p, size_t& size);

#endif	// #ifndef __VFS_H__