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
	VfsFile(const std::wstring& name, size_t size, time_t mtime, size_t priority, const PIFileLoader& provider);

	const std::wstring& Name() const
	{
		return m_name;
	}

	wchar_t LocationCode() const
	{
		return m_loader->LocationCode();
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

	LibError Load(const shared_ptr<u8>& buf) const;

private:
	std::wstring m_name;
	size_t m_size;
	time_t m_mtime;

	size_t m_priority;

	PIFileLoader m_loader;
};


class VfsDirectory
{
public:
	typedef std::map<std::wstring, VfsFile> VfsFiles;
	typedef std::map<std::wstring, VfsDirectory> VfsSubdirectories;

	VfsDirectory();

	/**
	 * @return address of existing or newly inserted file.
	 **/
	VfsFile* AddFile(const VfsFile& file);

	/**
	 * @return address of existing or newly inserted subdirectory.
	 **/
	VfsDirectory* AddSubdirectory(const std::wstring& name);

	/**
	 * @return file with the given name.
	 * (note: non-const to allow changes to the file)
	 **/
	VfsFile* GetFile(const std::wstring& name);

	/**
	 * @return subdirectory with the given name.
	 * (note: non-const to allow changes to the subdirectory)
	 **/
	VfsDirectory* GetSubdirectory(const std::wstring& name);

	// note: exposing only iterators wouldn't enable callers to reserve space.

	const VfsFiles& Files() const
	{
		return m_files;
	}

	const VfsSubdirectories& Subdirectories() const
	{
		return m_subdirectories;
	}

	VfsSubdirectories& Subdirectories()
	{
		return m_subdirectories;
	}

	/**
	 * empty file and subdirectory lists (e.g. when rebuilding VFS).
	 * CAUTION: this invalidates all previously returned pointers.
	 **/
	void Clear();

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

	/**
	 * cause the directory to be (re)populated when next accessed
	 * by having ShouldPopulate return true.
	 * this is typically called in response to file change notifications.
	 **/
	void Invalidate();

private:
	VfsFiles m_files;
	VfsSubdirectories m_subdirectories;

	PRealDirectory m_realDirectory;
	volatile uintptr_t m_shouldPopulate;	// (cpu_CAS can't be used on bool)
};


/**
 * @return a string containing file attributes (location, size, timestamp) and name.
 **/
extern std::wstring FileDescription(const VfsFile& file);

/**
 * @return a string holding each files' description (one per line).
 **/
extern std::wstring FileDescriptions(const VfsDirectory& directory, size_t indentLevel);

#endif	// #ifndef INCLUDED_VFS_TREE
