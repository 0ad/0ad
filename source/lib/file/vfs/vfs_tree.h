/**
 * =========================================================================
 * File        : vfs_tree.h
 * Project     : 0 A.D.
 * Description : the actual 'filesystem' and its tree of directories.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_VFS_TREE
#define INCLUDED_VFS_TREE

#include "lib/file/filesystem.h"		// FileInfo

// we add/cancel directory watches from the VFS mount code for convenience -
// it iterates through all subdirectories anyway (*) and provides storage for
// a key to identify the watch (obviates separate TDir -> watch mapping).
//
// define this to strip out that code - removes .watch from struct TDir,
// and calls to res_watch_dir / res_cancel_watch.
//
// *: the add_watch code would need to iterate through subdirs and watch
//    each one, because the monitor API (e.g. FAM) may only be able to
//    watch single directories, instead of a whole subdirectory tree.
#define NO_DIR_WATCH

class DirectoryWatch
{
public:
	DirectoryWatch()
#ifndef NO_DIR_WATCH
		: m_watch(0)
#endif
	{
	}

	// 'watch' this directory for changes to support hotloading.
	void Register(const char* path)
	{
#ifndef NO_DIR_WATCH
		char osPath[PATH_MAX];
		(void)path_MakeAbsolute(path, osPath);
		(void)dir_add_watch(osPath, &m_watch);
#else
		UNUSED2(path);
#endif
	}

	void Unregister()
	{
#ifndef NO_DIR_WATCH
		if(m_watch)	// avoid dir_cancel_watch complaining
		{
			dir_cancel_watch(m_watch);
			m_watch = 0;
		}
#endif
	}

#ifndef NO_DIR_WATCH
private:
	intptr_t m_watch;
#endif
};


//-----------------------------------------------------------------------------

struct ArchiveEntry;

class VfsFile
{
	friend class VfsDirectory;

public:
	VfsFile(const FileInfo& fileInfo, unsigned priority)
		: m_fileInfo(fileInfo), m_priority(priority), m_path(0), m_archiveEntry(0)
	{
	}

	void GetFileInfo(FileInfo& fileInfo) const
	{
		fileInfo = m_fileInfo;
	}

	const char* Name() const
	{
		return m_fileInfo.Name();
	}

	off_t Size() const
	{
		return m_fileInfo.Size();
	}

	time_t MTime() const
	{
		return m_fileInfo.MTime();
	}

	LibError Load(u8* buf) const;

	unsigned Priority() const
	{
		return m_priority;
	}

	unsigned Precedence() const
	{
		return m_archiveEntry? 1u : 2u;
	}

	char LocationCode() const
	{
		return m_archiveEntry? 'A' : 'F';
	}

private:
	FileInfo m_fileInfo;

	unsigned m_priority;

	// location (directory or archive) of the file. this allows opening
	// the file in O(1) even when there are multiple mounted directories.
	const char* m_path;
	ArchiveEntry* m_archiveEntry;
};


//-----------------------------------------------------------------------------

class VfsDirectory
{
public:
	VfsDirectory(const char* vfsPath);
	~VfsDirectory();

	void AddFile(const FileInfo& fileInfo);
	void AddSubdirectory(const char* name);

	VfsFile* GetFile(const char* name) const;
	VfsDirectory* GetSubdirectory(const char* name) const;

	// (don't split this into 2 functions because POSIX can't implement
	// that efficiently)
	void GetEntries(std::vector<FileInfo>* files, std::vector<const char*>* subdirectories) const;

	void DisplayR() const;
	void ClearR();

	void AssociateWithRealDirectory(const char* path);
	void Populate();

	void CreateFile(const char* name, const u8* buf, size_t size, uint flags = 0);

private:
	typedef std::map<const char*, VfsFile> Files;
	Files m_files;

	typedef std::map<const char*, VfsDirectory> Subdirectories;
	Subdirectories m_subdirectories;

	const char* m_vfsPath;

	// if exactly one real directory is mounted into this virtual dir,
	// this points to its location. used to add files to VFS when writing.
	const char* m_path;

	DirectoryWatch m_watch;
};


//-----------------------------------------------------------------------------

class VfsTree
{
public:
	VfsTree();

	VfsDirectory& Root()
	{
		return m_rootDirectory;
	}

	const VfsDirectory& Root() const
	{
		return m_rootDirectory;
	}

	void Display() const;
	void Clear();

private:
	VfsDirectory m_rootDirectory;
};

#endif	// #ifndef INCLUDED_VFS_TREE
