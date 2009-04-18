/**
 * =========================================================================
 * File        : vfs_tree.h
 * Project     : 0 A.D.
 * Description : 'tree' of VFS directories and files
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_VFS_TREE
#define INCLUDED_VFS_TREE

#include "lib/file/file_system.h"	// FileInfo
#include "lib/file/common/file_loader.h"	// PIFileLoader
#include "lib/file/common/real_directory.h"	// PRealDirectory

class VfsFile
{
public:
	VfsFile(const std::string& name, off_t size, time_t mtime, size_t priority, const PIFileLoader& provider);

	const std::string& Name() const
	{
		return m_name;
	}

	off_t Size() const
	{
		return m_size;
	}

	time_t MTime() const
	{
		return m_mtime;
	}

	bool IsSupersededBy(const VfsFile& file) const;

	void GenerateDescription(char* text, size_t maxChars) const;

	LibError Load(const shared_ptr<u8>& buf) const;

private:
	std::string m_name;
	off_t m_size;
	time_t m_mtime;

	size_t m_priority;

	PIFileLoader m_loader;
};


class VfsDirectory
{
public:
	VfsDirectory();

	/**
	 * @return address of existing or newly inserted file; remains
	 * valid until ClearR is called (i.e. VFS is rebuilt).
	 **/
	VfsFile* AddFile(const VfsFile& file);

	/**
	 * @return address of existing or newly inserted subdirectory; remains
	 * valid until ClearR is called (i.e. VFS is rebuilt).
	 **/
	VfsDirectory* AddSubdirectory(const std::string& name);

	VfsFile* GetFile(const std::string& name);
	VfsDirectory* GetSubdirectory(const std::string& name);

	void GetEntries(FileInfos* files, DirectoryNames* subdirectories) const;

	void DisplayR(size_t depth) const;

	void ClearR();

	/**
	 * side effect: the next ShouldPopulate() will return true.
	 **/
	void SetAssociatedDirectory(const PRealDirectory& realDirectory);

	const PRealDirectory& AssociatedDirectory() const
	{
		return m_realDirectory;
	}

	/**
	 * @return whether this directory should be populated from its
	 * AssociatedDirectory(). note that calling this is a promise to
	 * do so if true is returned -- the flag is immediately reset.
	 **/
	bool ShouldPopulate();

private:
	typedef std::map<std::string, VfsFile> VfsFiles;
	VfsFiles m_files;

	typedef std::map<std::string, VfsDirectory> VfsSubdirectories;
	VfsSubdirectories m_subdirectories;

	PRealDirectory m_realDirectory;
	volatile uintptr_t m_shouldPopulate;	// (cpu_CAS can't be used on bool)
};

#endif	// #ifndef INCLUDED_VFS_TREE
