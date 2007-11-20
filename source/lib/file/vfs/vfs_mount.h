/**
 * =========================================================================
 * File        : vfs_mount.h
 * Project     : 0 A.D.
 * Description : mounts files and archives into VFS; provides x_* API
 *             : that dispatches to file or archive implementation.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_VFS_MOUNT
#define INCLUDED_VFS_MOUNT

namespace ERR
{
	const LibError ALREADY_MOUNTED      = -110700;
	const LibError NOT_MOUNTED          = -110701;
	const LibError MOUNT_INVALID_TYPE   = -110702;
}

// (recursive mounting and mounting archives are no longer optional since they don't hurt)
enum MountFlags
{
	// all real directories mounted during this operation will be watched
	// for changes. this flag is provided to avoid watches in output-only
	// directories, e.g. screenshots/ (only causes unnecessary overhead).
	MOUNT_WATCH = 4,

	// anything mounted from here should be added to archive when
	// building via vfs_optimizer.
	MOUNT_ARCHIVABLE = 8
};



#endif	// #ifndef INCLUDED_VFS_MOUNT
