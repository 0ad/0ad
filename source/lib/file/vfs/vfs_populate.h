/**
 * =========================================================================
 * File        : vfs_populate.h
 * Project     : 0 A.D.
 * Description : populate VFS directories with files
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_VFS_POPULATE
#define INCLUDED_VFS_POPULATE

#include "lib/file/common/real_directory.h"

class VfsDirectory;

/**
 * attach a real directory to a VFS directory.
 *
 * when the VFS directory is accessed, it will first be populated from
 * the real directory. (this delays the impact of mounting a large
 * directory, distributing the cost from startup to the first accesses
 * to each subdirectory.)
 *
 * note: the most recently attached real directory will be used when
 * creating files in the VFS directory.
 **/
extern LibError vfs_Attach(VfsDirectory* directory, const PRealDirectory& realDirectory);

/**
 * populate the directory from the attached real directory.
 *
 * adds each real file and subdirectory entry to the VFS directory.
 * the full contents of any archives in the real directory are also added.
 *
 * has no effect if no directory has been attached since the last populate.
 **/
extern LibError vfs_Populate(VfsDirectory* directory);

#endif	// #ifndef INCLUDED_VFS_POPULATE
