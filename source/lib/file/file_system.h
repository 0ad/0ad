/* Copyright (c) 2016 Wildfire Games
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
 * higher-level interface on top of sysdep/filesystem.h
 */

#ifndef INCLUDED_FILE_SYSTEM
#define INCLUDED_FILE_SYSTEM

#include "lib/os_path.h"
#include "lib/posix/posix_filesystem.h"	// mode_t


LIB_API bool DirectoryExists(const OsPath& path);
LIB_API bool FileExists(const OsPath& pathname);

LIB_API u64 FileSize(const OsPath& pathname);


// (bundling size and mtime avoids a second expensive call to stat())
class CFileInfo
{
public:
	CFileInfo()
	{
	}

	CFileInfo(const OsPath& name, off_t size, time_t mtime)
		: name(name), size(size), mtime(mtime)
	{
	}

	const OsPath& Name() const
	{
		return name;
	}

	off_t Size() const
	{
		return size;
	}

	time_t MTime() const
	{
		return mtime;
	}

private:
	OsPath name;
	off_t size;
	time_t mtime;
};

LIB_API Status GetFileInfo(const OsPath& pathname, CFileInfo* fileInfo);

typedef std::vector<CFileInfo> CFileInfos;
typedef std::vector<OsPath> DirectoryNames;

LIB_API Status GetDirectoryEntries(const OsPath& path, CFileInfos* files, DirectoryNames* subdirectoryNames);

// same as boost::filesystem::create_directories, except that mkdir is invoked with
// <mode> instead of 0755.
// If the breakpoint is enabled, debug_break will be called if the directory didn't exist and couldn't be created.
LIB_API Status CreateDirectories(const OsPath& path, mode_t mode, bool breakpoint = true);

LIB_API Status DeleteDirectory(const OsPath& dirPath);

#endif	// #ifndef INCLUDED_FILE_SYSTEM
