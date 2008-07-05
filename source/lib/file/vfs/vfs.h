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

namespace ERR
{
	const LibError VFS_DIR_NOT_FOUND   = -110100;
	const LibError VFS_FILE_NOT_FOUND  = -110101;
	const LibError VFS_ALREADY_MOUNTED = -110102;
}

// (recursive mounting and mounting archives are no longer optional since they don't hurt)
enum VfsMountFlags
{
	/**
	 * all real directories mounted during this operation will be watched
	 * for changes. this flag is provided to avoid watches in output-only
	 * directories, e.g. screenshots/ (only causes unnecessary overhead).
	 **/
	VFS_MOUNT_WATCH = 1,

	/**
	 * anything mounted from here should be included when building archives.
	 **/
	VFS_MOUNT_ARCHIVABLE = 2
};

struct IVFS
{
	/**
	 * mount a directory into the VFS.
	 *
	 * @param mountPoint (will be created if it does not already exist)
	 * @param path real directory path
	 * @return LibError.
	 *
	 * if files are encountered that already exist in the VFS (sub)directories,
	 * the most recent / highest priority/precedence version is preferred.
	 *
	 * if files with archive extensions are seen, their contents are added
	 * as well.
	 **/
	virtual LibError Mount(const VfsPath& mountPoint, const Path& path, int flags = 0, size_t priority = 0) = 0;

	/**
	 * retrieve information about a file (similar to POSIX stat)
	 * 
	 * @return LibError.
	 **/
	virtual LibError GetFileInfo(const VfsPath& pathname, FileInfo* pfileInfo) const = 0;

	/**
	 * retrieve lists of all files and subdirectories in a directory.
	 *
	 * @return LibError.
	 *
	 * rationale:
	 * - this interface avoids having to lock the directory while an
	 *   iterator is extant.
	 * - we cannot efficiently provide routines for returning files and
	 *   subdirectories separately due to the underlying POSIX interface.
	 **/
	virtual LibError GetDirectoryEntries(const VfsPath& path, FileInfos* files, DirectoryNames* subdirectoryNames) const = 0;

	/**
	 * create a file with the given contents.
	 *
	 * @param size [bytes] of the contents, will match that of the file.
	 * @return LibError.
	 *
	 * rationale: disallowing partial writes simplifies file cache coherency
	 * (need only be invalidated when closing a FILE_WRITE file).
	 **/
	virtual LibError CreateFile(const VfsPath& pathname, shared_ptr<u8> fileContents, size_t size) = 0;

	/**
	 * read an entire file into memory.
	 *
	 * @param fileContents receives a smart pointer to the contents.
	 *   CAVEAT: this will be taken from the file cache if the VFS was
	 *   created with cacheSize != 0 and size < cacheSize. there is no
	 *   provision for Copy-on-Write, which means that such buffers
	 *   must not be modified (this is enforced via mprotect).
	 * @param size receives the size [bytes] of the file contents.
	 * @return LibError.
	 **/
	virtual LibError LoadFile(const VfsPath& pathname, shared_ptr<u8>& fileContents, size_t& size) = 0;

	/**
	 * dump a text representation of the filesystem to debug output.
	 **/
	virtual void Display() const = 0;

	/**
	 * empty the contents of the filesystem.
	 *
	 * the effect is as if nothing had been mounted.
	 **/
	virtual void Clear() = 0;

	/**
	 * retrieve the real (POSIX) path underlying a VFS file.
	 *
	 * this is useful when passing paths to external libraries.
	 **/
	virtual LibError GetRealPath(const VfsPath& pathname, Path& path) = 0;
};

typedef shared_ptr<IVFS> PIVFS;

/**
 * create an instance of a Virtual File System.
 *
 * @param cacheSize size [bytes] of memory to reserve for a file cache,
 * or zero to disable it. if small enough to fit, file contents are
 * stored here until no references remain and they are evicted.
 *
 * note: there is no limitation to a single instance, it may make sense
 * to create and destroy VFS instances during each unit test.
 **/
LIB_API PIVFS CreateVfs(size_t cacheSize);

#endif	// #ifndef INCLUDED_VFS
