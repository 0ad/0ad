/* Copyright (c) 2010 Wildfire Games
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

#ifndef INCLUDED_FILE_SYSTEM
#define INCLUDED_FILE_SYSTEM

#include "lib/external_libraries/boost_filesystem.h"

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
