/**
 * =========================================================================
 * File        : vfs_tree.h
 * Project     : 0 A.D.
 * Description : the actual 'filesystem' and its tree of directories.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef VFS_TREE_H__
#define VFS_TREE_H__

class TFile;	// must come before vfs_mount.h
class TDir;

#include "file.h"		// DirEnt
#include "vfs_mount.h"	// Mount


// establish a root node and prepare node_allocator for use.
extern void tree_init();

// shut down entirely; destroys node_allocator. any further use after this
// requires another tree_init.
extern void tree_shutdown();

extern void tree_display();

// empty all directories and free their memory.
// however, node_allocator's DynArray still remains initialized and
// the root directory is usable (albeit empty).
// use when remounting.
extern void tree_clear();

extern time_t tree_most_recent_mtime();

// attempt to add <fn> to <dir>, storing its attributes.
// overrides previously existing files of the same name if the new one
// is more important, determined via priority and file location.
// called by zip_cb and add_ent.
//
// note: if "priority" is the same, replace!
// this makes sure mods/patches etc. actually replace files.
extern LibError tree_add_file(TDir* td, const char* name, const Mount* m,
	off_t size, time_t mtime, uintptr_t memento);

extern LibError tree_add_dir(TDir* dir, const char* name, TDir** ptd);



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
//
// output params are only valid if ERR_OK is returned.
extern LibError tree_lookup(const char* path, TFile** ptf, uint flags = 0);

// starting at VFS root, traverse <path> and pass back information
// for its last directory component.
//
// if <flags> & LF_CREATE_MISSING, all missing subdirectory components are
//   added to the VFS.
// if <flags> & LF_START_DIR, traversal starts at *pdir
//   (used when looking up paths relative to a mount point).
//
// <path> can be to a file or dir (in which case it must end in '/',
// to make sure the last component is treated as a directory).
//
// output params are only valid if ERR_OK is returned.
extern LibError tree_lookup_dir(const char* path, TDir** ptd, uint flags = 0);


// documentation and rationale: see file.h's dir_next_ent interface
struct TreeDirIterator
{
	char opaque[32];
};

extern LibError tree_dir_open(const char* path_slash, TreeDirIterator* d);
extern LibError tree_dir_next_ent(TreeDirIterator* d, DirEnt* ent);
extern LibError tree_dir_close(TreeDirIterator* d);


// given a file that is stored on disk and its VFS path,
// return its OS path (for use with file.cpp).
// used by vfs_realpath and VFile_reopen.
extern LibError tree_realpath(TFile* tf, const char* V_path, char* P_real_path);

extern LibError tree_stat(const TFile* tf, struct stat* s);

extern const Mount* tfile_get_mount(const TFile* tf);
extern uintptr_t tfile_get_memento(const TFile* tf);
extern const char* tfile_get_atom_fn(const TFile* tf);

extern void tfile_set_mount(TFile* tf, const Mount* m);
extern void tree_update_file(TFile* tf, off_t size, time_t mtime);

struct RealDir;
extern RealDir* tree_get_real_dir(TDir* td);


// for use in vfs_mount
extern void tree_lock();
extern void tree_unlock();

#endif	// #ifndef VFS_TREE_H__
