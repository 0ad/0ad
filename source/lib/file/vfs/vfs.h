/* Copyright (C) 2021 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Virtual File System API - allows transparent access to files in
 * archives, modding via multiple mount points and hotloading.
 */

#ifndef INCLUDED_VFS
#define INCLUDED_VFS

#include "lib/file/file_system.h"	// CFileInfo
#include "lib/file/vfs/vfs_path.h"

constexpr size_t VFS_MIN_PRIORITY = 0;
constexpr size_t VFS_MAX_PRIORITY = std::numeric_limits<size_t>::max();

namespace ERR
{
	const Status VFS_DIR_NOT_FOUND   = -110100;
	const Status VFS_FILE_NOT_FOUND  = -110101;
	const Status VFS_ALREADY_MOUNTED = -110102;
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
	VFS_MOUNT_ARCHIVABLE = 2,

	/**
	 * return ERR::VFS_DIR_NOT_FOUND if the given real path doesn't exist.
	 * (the default behavior is to create all real directories in the path)
	 **/
	VFS_MOUNT_MUST_EXIST = 4,

	/**
	 * keep the files named "*.DELETED" visible in the VFS directories.
	 * the standard behavior of hiding the file with the same name minus the
	 * ".DELETED" suffix will still apply.
	 * (the default behavior is to hide both the suffixed and unsuffixed files)
	 **/
	VFS_MOUNT_KEEP_DELETED = 8
};

// (member functions are thread-safe after the instance has been
// constructed - each acquires a mutex.)
struct IVFS
{
	virtual ~IVFS() {}

	/**
	 * mount a directory into the VFS.
	 *
	 * @param mountPoint (will be created if it does not already exist)
	 * @param path real directory path
	 * @param flags
	 * @param priority
	 * @return Status.
	 *
	 * if files are encountered that already exist in the VFS (sub)directories,
	 * the most recent / highest priority/precedence version is preferred.
	 *
	 * Note that the 'real directory' associated with a VFS Path
	 * will be relative to the highest priority subdirectory in the path,
	 * and that in case of equal priority, the order is _undefined_,
	 * and will depend on the exact order of populate() calls.
	 *
	 * if files with archive extensions are seen, their contents are added
	 * as well.
	 **/
	virtual Status Mount(const VfsPath& mountPoint, const OsPath& path, size_t flags = 0, size_t priority = 0) = 0;

	/**
	 * Retrieve information about a file (similar to POSIX stat).
	 *
	 * @param pathname
	 * @param pfileInfo receives information about the file. Passing NULL
	 *		  suppresses warnings if the file doesn't exist.
	 *
	 * @return Status.
	 **/
	virtual Status GetFileInfo(const VfsPath& pathname, CFileInfo* pfileInfo) const = 0;

	/**
	 * Retrieve mount priority for a file.
	 *
	 * @param pathname
	 * @param ppriority receives priority value, if the file can be found.
	 *
	 * @return Status.
	 **/
	virtual Status GetFilePriority(const VfsPath& pathname, size_t* ppriority) const = 0;

	/**
	 * Retrieve lists of all files and subdirectories in a directory.
	 *
	 * @return Status.
	 *
	 * Rationale:
	 * - this interface avoids having to lock the directory while an
	 *   iterator is extant.
	 * - we cannot efficiently provide routines for returning files and
	 *   subdirectories separately due to the underlying POSIX interface.
	 **/
	virtual Status GetDirectoryEntries(const VfsPath& path, CFileInfos* fileInfos, DirectoryNames* subdirectoryNames) const = 0;

	/**
	 * Create a file with the given contents.
	 * @param pathname
	 * @param fileContents
	 * @param size [bytes] of the contents, will match that of the file.
	 * @return Status.
	 **/
	virtual Status CreateFile(const VfsPath& pathname, const shared_ptr<u8>& fileContents, size_t size) = 0;

	/**
	 * Read an entire file into memory.
	 *
	 * @param pathname
	 * @param fileContents receives a smart pointer to the contents.
	 * @param size receives the size [bytes] of the file contents.
	 * @return Status.
	 **/
	virtual Status LoadFile(const VfsPath& pathname, shared_ptr<u8>& fileContents, size_t& size) = 0;

	/**
	 * @return a string representation of all files and directories.
	 **/
	virtual std::wstring TextRepresentation() const = 0;

	/**
	 * Retrieve the POSIX pathname a VFS file was loaded from.
	 * This is distinct from the current 'real' path, since that depends on the parent directory's real path,
	 * which may have been overwritten by a mod or another call to Mount().
	 *
	 * This is used by the caching to split by mod, and you also ought to call this to delete a file.
	 * (note that deleting has other issues, see below).
	 **/
	virtual Status GetOriginalPath(const VfsPath& filename, OsPath& loadedPathname) = 0;

	/**
	 * Retrieve the real (POSIX) pathname underlying a VFS file.
	 * This is useful for passing paths to external libraries.
	 * Note that this returns the real path relative to the highest priority VFS subpath.
	 * @param createMissingDirectories - if true, create subdirectories on the file system as required.
	 * (this defaults to true because, in general, this function is then used to create new files).
	 **/
	virtual Status GetRealPath(const VfsPath& pathname, OsPath& realPathname, bool createMissingDirectories = true) = 0;

	/**
	 * Retrieve the real (POSIX) pathname underlying a VFS directory.
	 * This is useful for passing paths to external libraries.
	 * Note that this returns the real path relative to the highest priority VFS subpath.
	 * @param createMissingDirectories - if true, create subdirectories on the file system as required.
	 * (this defaults to true because, in general, this function is then used to create new files).
	 **/
	virtual Status GetDirectoryRealPath(const VfsPath& pathname, OsPath& realPathname, bool createMissingDirectories = true) = 0;

	/**
	 * retrieve the VFS pathname that corresponds to a real file.
	 *
	 * this is useful for reacting to file change notifications.
	 *
	 * the current implementation requires time proportional to the
	 * number of directories; this could be accelerated by only checking
	 * directories below a mount point with a matching real path.
	 **/
	virtual Status GetVirtualPath(const OsPath& realPathname, VfsPath& pathname) = 0;

	/**
	 * remove file from the virtual directory listing.
	 **/
	virtual Status RemoveFile(const VfsPath& pathname) = 0;

	/**
	 * request the directory be re-populated when it is next accessed.
	 * useful for synchronizing with the underlying filesystem after
	 * files have been created or their metadata changed.
	 **/
	virtual Status RepopulateDirectory(const VfsPath& path) = 0;

	/**
	 * empty the contents of the filesystem.
	 * this is typically only necessary when changing the set of
	 * mounted directories, e.g. when switching mods.
	 * NB: open files are not affected.
	 **/
	virtual void Clear() = 0;
};

typedef shared_ptr<IVFS> PIVFS;

/**
 * create an instance of a Virtual File System.
 *
 * note: there is no limitation to a single instance, it may make sense
 * to create and destroy VFS instances during each unit test.
 **/
LIB_API PIVFS CreateVfs();

#endif	// #ifndef INCLUDED_VFS
