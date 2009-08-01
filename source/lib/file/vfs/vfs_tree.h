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
 * 'tree' of VFS directories and files
 */

#ifndef INCLUDED_VFS_TREE
#define INCLUDED_VFS_TREE

#include <map>

#include "lib/file/file_system.h"	// FileInfo
#include "lib/file/common/file_loader.h"	// PIFileLoader
#include "lib/file/common/real_directory.h"	// PRealDirectory

class VfsFile
{
public:
	VfsFile(const std::string& name, size_t size, time_t mtime, size_t priority, const PIFileLoader& provider);

	const std::string& Name() const
	{
		return m_name;
	}

	size_t Size() const
	{
		return m_size;
	}

	time_t MTime() const
	{
		return m_mtime;
	}

	bool IsSupersededBy(const VfsFile& file) const;

	// store file attributes (timestamp, location, size) in a string.
	void GenerateDescription(char* text, size_t maxChars) const;

	LibError Load(const shared_ptr<u8>& buf) const;

private:
	std::string m_name;
	size_t m_size;
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
