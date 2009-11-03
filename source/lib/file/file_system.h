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

#ifndef INCLUDED_FILE_SYSTEM
#define INCLUDED_FILE_SYSTEM

class FileInfo
{
public:
	FileInfo()
	{
	}

	FileInfo(const std::wstring& name, off_t size, time_t mtime)
		: m_name(name), m_size(size), m_mtime(mtime)
	{
	}

	const std::wstring& Name() const
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

private:
	std::wstring m_name;
	off_t m_size;
	time_t m_mtime;
};

extern LibError GetFileInfo(const fs::wpath& pathname, FileInfo* fileInfo);

typedef std::vector<FileInfo> FileInfos;
typedef std::vector<std::wstring> DirectoryNames;

// jw 2007-12-20: we'd love to replace this with boost::filesystem,
// but basic_directory_iterator does not yet cache file_size and
// last_write_time in file_status. (they each entail a stat() call,
// which is unacceptably slow.)
extern LibError GetDirectoryEntries(const fs::wpath& path, FileInfos* files, DirectoryNames* subdirectoryNames);

// same as fs::create_directories, except that mkdir is invoked with
// <mode> instead of 0755.
extern LibError CreateDirectories(const fs::wpath& path, mode_t mode);

extern LibError DeleteDirectory(const fs::wpath& dirPath);

#endif	// #ifndef INCLUDED_FILE_SYSTEM
