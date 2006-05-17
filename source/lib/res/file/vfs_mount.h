/**
 * =========================================================================
 * File        : vfs_mount.h
 * Project     : 0 A.D.
 * Description : mounts files and archives into VFS; provides x_* API
 *             : that dispatches to file or archive implementation.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef VFS_MOUNT_H__
#define VFS_MOUNT_H__

struct Mount;	// must come before vfs_tree.h

#include "file.h"
#include "zip.h"
#include "vfs_tree.h"

extern void mount_init();
extern void mount_shutdown();


// If it was possible to forward-declare enums in gcc, this one wouldn't be in
// the header. Don't use.
enum MountType
{
	// the relative ordering of values expresses efficiency of the sources
	// (e.g. archives are faster than loose files). mount_should_replace
	// makes use of this.

	MT_NONE    = 0,
	MT_FILE    = 1,
	MT_ARCHIVE = 2
};


//
// accessor routines that obviate the need to access Mount fields directly:
//

extern bool mount_is_archivable(const Mount* m);

extern bool mount_should_replace(const Mount* m_old, const Mount* m_new,
	size_t size_old, size_t size_new, time_t mtime_old, time_t mtime_new);

extern char mount_get_type(const Mount* m);

extern Handle mount_get_archive(const Mount* m);

// given Mount and V_path, return its actual location (portable path).
extern LibError mount_realpath(const char* V_path, const Mount* m, char* P_real_path);



// stored by vfs_tree in TDir
struct RealDir
{
	// if exactly one real directory is mounted into this virtual dir,
	// this points to its location. used to add files to VFS when writing.
	//
	// the Mount is actually in the mount info and is invalid when
	// that's unmounted, but the VFS would then be rebuilt anyway.
	//
	// = 0 if no real dir mounted here; = -1 if more than one.
	const Mount* m;
#ifndef NO_DIR_WATCH
	intptr_t watch;
#endif
};

extern LibError mount_attach_real_dir(RealDir* rd, const char* P_path, const Mount* m, uint flags);
extern void mount_detach_real_dir(RealDir* rd);

extern LibError mount_create_real_dir(const char* V_path, const Mount* m);

extern LibError mount_populate(TDir* td, RealDir* rd);


// "backs off of" all archives - closes their files and allows them to
// be rewritten or deleted (required by archive builder).
// must call mount_rebuild when done with the rewrite/deletes,
// because this call leaves the VFS in limbo!!
extern void mount_release_all_archives();

// 'relocate' tf to the mounting established by vfs_set_write_target.
// call if <tf> is being opened with FILE_WRITE_TO_TARGET flag set.
extern LibError set_mount_to_write_target(TFile* tf);


// rebuild the VFS, i.e. re-mount everything. open files are not affected.
// necessary after loose files or directories change, so that the VFS
// "notices" the changes and updates file locations. res calls this after
// dir_watch reports changes; can also be called from the console after a
// rebuild command. there is no provision for updating single VFS dirs -
// it's not worth the trouble.
extern LibError mount_rebuild();

// if <path> or its ancestors are mounted,
// return a VFS path that accesses it.
// used when receiving paths from external code.
extern LibError mount_make_vfs_path(const char* P_path, char* V_path);

#endif	// #ifndef VFS_MOUNT_H__
