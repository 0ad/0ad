/**
 * =========================================================================
 * File        : vfs.h
 * Project     : 0 A.D.
 * Description : Virtual File System API - allows transparent access to
 *             : files in archives and modding via multiple mount points.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_VFS
#define INCLUDED_VFS

#include "lib/file/file_system.h"	// FileInfo
#include "lib/file/path.h"
#include "lib/file/vfs/vfs_path.h"

// VFS paths are of the form: "(dir/)*file?"
// in English: '/' as path separator; trailing '/' required for dir names;
// no leading '/', since "" is the root dir.

// there is no restriction on path length; when dimensioning character
// arrays, prefer PATH_MAX.

// pathnames are case-insensitive.
// implementation:
//   when mounting, we get the exact filenames as reported by the OS;
//   we allow open requests with mixed case to match those,
//   but still use the correct case when passing to other libraries
//   (e.g. the actual open() syscall).
// rationale:
//   necessary, because some exporters output .EXT uppercase extensions
//   and it's unreasonable to expect that users will always get it right.

namespace ERR
{
	const LibError VFS_DIR_NOT_FOUND   = -110100;
	const LibError VFS_FILE_NOT_FOUND  = -110101;
	const LibError VFS_ALREADY_MOUNTED = -110102;
}

// (recursive mounting and mounting archives are no longer optional since they don't hurt)
enum VfsMountFlags
{
	// all real directories mounted during this operation will be watched
	// for changes. this flag is provided to avoid watches in output-only
	// directories, e.g. screenshots/ (only causes unnecessary overhead).
	VFS_MOUNT_WATCH = 1,

	// anything mounted from here should be added to archive when
	// building via vfs_optimizer.
	VFS_MOUNT_ARCHIVABLE = 2
};

struct IVFS
{
	/**
	 * mount a directory into the VFS.
	 *
	 * @param mountPoint (will be created if it does not already exist)
	 * @param path real directory path
	 *
	 * if files are encountered that already exist in the VFS (sub)directories,
	 * the most recent / highest priority/precedence version is preferred.
	 *
	 * if files with archive extensions are seen, their contents are added
	 * as well.
	 **/
	virtual LibError Mount(const VfsPath& mountPoint, const Path& path, int flags = 0, size_t priority = 0) = 0;

	virtual LibError GetFileInfo(const VfsPath& pathname, FileInfo* pfileInfo) const = 0;

	// note: this interface avoids having to lock a directory while an
	// iterator is extant.
	// (don't split this into 2 functions because POSIX can't implement
	// that efficiently)
	virtual LibError GetDirectoryEntries(const VfsPath& path, FileInfos* files, DirectoryNames* subdirectoryNames) const = 0;


	// note: only allowing either reads or writes simplifies file cache
	// coherency (need only invalidate when closing a FILE_WRITE file).
	virtual LibError CreateFile(const VfsPath& pathname, shared_ptr<u8> fileContents, size_t size) = 0;

	// read the entire file.
	// return number of bytes transferred (see above), or a negative error code.
	//
	// if non-NULL, <cb> is called for each block transferred, passing <cbData>.
	// it returns how much data was actually transferred, or a negative error
	// code (in which case we abort the transfer and return that value).
	// the callback mechanism is useful for user progress notification or
	// processing data while waiting for the next I/O to complete
	// (quasi-parallel, without the complexity of threads).
	virtual LibError LoadFile(const VfsPath& pathname, shared_ptr<u8>& fileContents, size_t& size) = 0;

	virtual void Display() const = 0;
	virtual void Clear() = 0;

	virtual LibError GetRealPath(const VfsPath& pathname, Path& path) = 0;
};

typedef shared_ptr<IVFS> PIVFS;
LIB_API PIVFS CreateVfs(size_t cacheSize);

#endif	// #ifndef INCLUDED_VFS
