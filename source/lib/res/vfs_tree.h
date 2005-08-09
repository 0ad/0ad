// virtual file system: tree of files and directories
//
// Copyright (c) 2005 Jan Wassenberg
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

#ifndef VFS_TREE_H__
#define VFS_TREE_H__

#include "file.h"		// DirEnt
#include "vfs_mount.h"	// Mount

struct TFile;
struct TDir;

extern void tree_init();

// empties the tree and frees all resources. this is used when
// rebuilding VFS and on exit.
extern void tree_clear();

// attempt to add <fn> to <dir>, storing its attributes.
// overrides previously existing files of the same name if the new one
// is more important, determined via priority and file location.
// called by zip_cb and add_ent.
//
// note: if "priority" is the same, replace!
// this makes sure mods/patches etc. actually replace files.
extern int tree_add_file(TDir* td, const char* fn, const Mount* m,
	off_t size, time_t mtime);

extern int tree_add_dir(TDir* dir, const char* name, TDir** ptd);
extern int tree_attach_real_dir(TDir* dir, const char* path, int flags, const Mount* m);

enum TreeLookupFlags
{
	LF_CREATE_MISSING = 1,
	LF_START_DIR      = 2
};

// pass back file information for <path> (relative to VFS root).
//
// if <flags> & LF_CREATE_MISSING, the file is added to VFS unless
//   a higher-priority file of the same name already exists
//   (used by VFile_reload when opening for writing).
// if <exact_path> != 0, it receives a copy of <path> with the exact
//   case of each component as returned by the OS (useful for calling
//   external case-sensitive code). must hold at least VFS_MAX_PATH chars.
//
// return 0 on success, or a negative error code
// (in which case output params are undefined).
extern int tree_lookup(const char* path, TFile** ptf, uint flags = 0, char* exact_path = 0);

// starting at VFS root, traverse <path> and pass back information
// for its last directory component.
//
// if <flags> & LF_CREATE_MISSING, all missing subdirectory components are
//   added to the VFS.
// if <flags> & LF_START_DIR, traversal starts at *pdir
//   (used when looking up paths relative to a mount point).
// if <exact_path> != 0, it receives a copy of <path> with the exact
//   case of each component as returned by the OS (useful for calling
//   external case-sensitive code). must hold at least VFS_MAX_PATH chars.
//
// <path> can be to a file or dir (in which case it must end in '/',
// to make sure the last component is treated as a directory).
//
// return 0 on success, or a negative error code
// (in which case output params are undefined).
extern int tree_lookup_dir(const char* path, TDir** ptd, uint flags = 0, char* exact_path = 0);


// documentation and rationale: see file.h's dir_next_ent interface
struct TreeDirIterator
{
	char opaque[32];
};

extern int tree_dir_open(const char* path_slash, TreeDirIterator* d);
extern int tree_dir_next_ent(TreeDirIterator* d, DirEnt* ent);
extern int tree_dir_close(TreeDirIterator* d);


// given a file that is stored on disk and its VFS path,
// return its OS path (for use with file.cpp).
// used by vfs_realpath and VFile_reopen.
extern int tree_realpath(TFile* tf, const char* V_path, char* P_real_path);

extern int tree_stat(const TFile* tf, struct stat* s);

extern const Mount* tree_get_mount(const TFile* tf);

extern void tree_update_file(TFile* tf, off_t size, time_t mtime);

// for use in vfs_mount
extern void tree_lock();
extern void tree_unlock();

#endif	// #ifndef VFS_TREE_H__
