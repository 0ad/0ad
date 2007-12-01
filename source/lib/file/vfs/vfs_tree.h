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

#include "lib/file/filesystem.h"		// FileInfo, PIFileProvider
#include "real_directory.h"


class VfsFile
{
public:
	VfsFile(const FileInfo& fileInfo, unsigned priority, PIFileProvider provider, const void* location);

	const char* Name() const
	{
		return m_fileInfo.Name();
	}
	off_t Size() const
	{
		return m_fileInfo.Size();
	}
	void GetFileInfo(FileInfo& fileInfo) const
	{
		fileInfo = m_fileInfo;
	}

	bool IsSupersededBy(const VfsFile& file) const;
	void GenerateDescription(char* text, size_t maxChars) const;

	LibError Store(const u8* fileContents, size_t size) const;
	LibError Load(u8* fileContents) const;

private:
	mutable FileInfo m_fileInfo;

	unsigned m_priority;

	PIFileProvider m_provider;
	const void* m_location;
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
	VfsDirectory* AddSubdirectory(const char* name);

	VfsFile* GetFile(const char* name);
	VfsDirectory* GetSubdirectory(const char* name);

	void GetEntries(FileInfos* files, Directories* subdirectories) const;

	void DisplayR(unsigned depth) const;
	void ClearR();

	void Attach(const RealDirectory& realDirectory);
	const std::vector<RealDirectory>& AttachedDirectories() const
	{
		return m_attachedDirectories;
	}

private:
	typedef std::map<const char*, VfsFile> Files;
	Files m_files;

	typedef std::map<const char*, VfsDirectory> Subdirectories;
	Subdirectories m_subdirectories;

	std::vector<RealDirectory> m_attachedDirectories;
};

#endif	// #ifndef INCLUDED_VFS_TREE
