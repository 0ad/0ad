/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Virtual File System API - allows transparent access to files in
 * archives, modding via multiple mount points and hotloading.
 */

#ifndef INCLUDED_VFS
#define INCLUDED_VFS

#include "lib/file/file_system.h"	// FileInfo
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
	virtual ~IVFS() {}

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
	virtual LibError Mount(const VfsPath& mountPoint, const fs::wpath& path, size_t flags = 0, size_t priority = 0) = 0;

	/**
	 * retrieve information about a file (similar to POSIX stat)
	 *
	 * @param pfileInfo receives information about the file. passing NULL
	 * suppresses warnings if the file doesn't exist.
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
	virtual LibError GetDirectoryEntries(const VfsPath& path, FileInfos* fileInfos, DirectoryNames* subdirectoryNames) const = 0;

	/**
	 * create a file with the given contents.
	 *
	 * @param size [bytes] of the contents, will match that of the file.
	 * @return LibError.
	 *
	 * rationale: disallowing partial writes simplifies file cache coherency
	 * (we need only invalidate cached data when closing a newly written file).
	 **/
	virtual LibError CreateFile(const VfsPath& pathname, const shared_ptr<u8>& fileContents, size_t size) = 0;

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
	 * @return a string representation of all files and directories.
	 **/
	virtual std::wstring TextRepresentation() const = 0;

	/**
	 * retrieve the real (POSIX) pathname underlying a VFS file.
	 *
	 * this is useful for passing paths to external libraries.
	 **/
	virtual LibError RealPath(const VfsPath& pathname, fs::wpath& realPathname) = 0;

	/**
	 * retrieve the VFS pathname that corresponds to a real file.
	 *
	 * this is useful for reacting to file change notifications.
	 *
	 * the current implementation requires time proportional to the
	 * number of directories; this could be accelerated by only checking
	 * directories below a mount point with a matching real path.
	 **/
	virtual LibError VirtualPath(const fs::wpath& realPathname, VfsPath& pathname) = 0;

	/**
	 * poll for directory change notifications and reload all affected files.
	 * must be called regularly (e.g. once a frame), else notifications
	 * may be lost.
	 * note: polling is much simpler than asynchronous notifications.
	 **/
	virtual LibError ReloadChangedFiles() = 0;

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
 * @param cacheSize size [bytes] of memory to reserve for a file cache,
 * or zero to disable it. if small enough to fit, file contents are
 * stored here until no references remain and they are evicted.
 *
 * note: there is no limitation to a single instance, it may make sense
 * to create and destroy VFS instances during each unit test.
 **/
LIB_API PIVFS CreateVfs(size_t cacheSize);

#endif	// #ifndef INCLUDED_VFS
