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

#include "lib/file/filesystem.h"

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
	VFS_WATCH = 1,

	// anything mounted from here should be added to archive when
	// building via vfs_optimizer.
	VFS_ARCHIVABLE = 2
};


typedef boost::shared_ptr<const u8> FileContents;

class Filesystem_VFS : public IFilesystem
{
public:
	Filesystem_VFS(void* trace);
	~Filesystem_VFS();

	/**
	 * mount a directory into the VFS.
	 *
	 * @param vfsPath mount point (created if it does not already exist)
	 * @param path real directory path
	 *
	 * if files are encountered that already exist in the VFS (sub)directories,
	 * the most recent / highest priority/precedence version is preferred.
	 *
	 * the contents of archives in this directory (but not its subdirectories!)
	 * are added as well; they are processed in alphabetical order.
	 **/
	LibError Mount(const char* vfsPath, const char* path, uint flags = 0, uint priority = 0);

	// (see IFilesystem)
	virtual LibError GetFileInfo(const char* vfsPathname, FileInfo& fileInfo) const;
	virtual LibError GetDirectoryEntries(const char* vfsPath, FileInfos* files, Directories* subdirectories) const;

	// note: only allowing either reads or writes simplifies file cache
	// coherency (need only invalidate when closing a FILE_WRITE file).
	LibError CreateFile(const char* vfsPathname, const u8* data, size_t size);

	// read the entire file.
	// return number of bytes transferred (see above), or a negative error code.
	//
	// if non-NULL, <cb> is called for each block transferred, passing <cbData>.
	// it returns how much data was actually transferred, or a negative error
	// code (in which case we abort the transfer and return that value).
	// the callback mechanism is useful for user progress notification or
	// processing data while waiting for the next I/O to complete
	// (quasi-parallel, without the complexity of threads).
	LibError LoadFile(const char* vfsPathname, FileContents& contents, size_t& size);

	void RefreshFileInfo(const char* pathname);

	void Display() const;
	void Clear();

private:
	class Impl;
	boost::shared_ptr<Impl> impl;
};

#endif	// #ifndef INCLUDED_VFS
