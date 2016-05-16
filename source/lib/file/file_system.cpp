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

#include "precompiled.h"
#include "lib/file/file_system.h"

#include <vector>
#include <algorithm>
#include <string>

#include "lib/sysdep/filesystem.h"


bool DirectoryExists(const OsPath& path)
{
	WDIR* dir = wopendir(path);
	if(dir)
	{
		wclosedir(dir);
		return true;
	}
	return false;
}


bool FileExists(const OsPath& pathname)
{
	struct stat s;
	const bool exists = wstat(pathname, &s) == 0;
	return exists;
}


u64 FileSize(const OsPath& pathname)
{
	struct stat s;
	ENSURE(wstat(pathname, &s) == 0);
	return s.st_size;
}


Status GetFileInfo(const OsPath& pathname, CFileInfo* pPtrInfo)
{
	errno = 0;
	struct stat s;
	memset(&s, 0, sizeof(s));
	if(wstat(pathname, &s) != 0)
		WARN_RETURN(StatusFromErrno());

	*pPtrInfo = CFileInfo(pathname.Filename(), s.st_size, s.st_mtime);
	return INFO::OK;
}


struct DirDeleter
{
	void operator()(WDIR* osDir) const
	{
		const int ret = wclosedir(osDir);
		ENSURE(ret == 0);
	}
};

Status GetDirectoryEntries(const OsPath& path, CFileInfos* files, DirectoryNames* subdirectoryNames)
{
	// open directory
	errno = 0;
	WDIR* pDir = wopendir(path);
	if(!pDir)
		return StatusFromErrno();	// NOWARN
	shared_ptr<WDIR> osDir(pDir, DirDeleter());

	for(;;)
	{
		errno = 0;
		struct wdirent* osEnt = wreaddir(osDir.get());
		if(!osEnt)
		{
			// no error, just no more entries to return
			if(!errno)
				return INFO::OK;
			WARN_RETURN(StatusFromErrno());
		}

		for(size_t i = 0; osEnt->d_name[i] != '\0'; i++)
			RETURN_STATUS_IF_ERR(Path::Validate(osEnt->d_name[i]));
		const OsPath name(osEnt->d_name);

		// get file information (mode, size, mtime)
		struct stat s;
#if OS_WIN
		// .. return wdirent directly (much faster than calling stat).
		RETURN_STATUS_IF_ERR(wreaddir_stat_np(osDir.get(), &s));
#else
		// .. call regular stat().
		errno = 0;
		const OsPath pathname = path / name;
		if(wstat(pathname, &s) != 0)
			WARN_RETURN(StatusFromErrno());
#endif

		if(files && S_ISREG(s.st_mode))
			files->push_back(CFileInfo(name, s.st_size, s.st_mtime));
		else if(subdirectoryNames && S_ISDIR(s.st_mode) && name != L"." && name != L"..")
			subdirectoryNames->push_back(name);
	}
}


Status CreateDirectories(const OsPath& path, mode_t mode, bool breakpoint)
{
	if(path.empty())
		return INFO::OK;

	struct stat s;
	if(wstat(path, &s) == 0)
	{
		if(!S_ISDIR(s.st_mode))	// encountered a file
			WARN_RETURN(ERR::FAIL);
		return INFO::OK;
	}

	// If we were passed a path ending with '/', strip the '/' now so that
	// we can consistently use Parent to find parent directory names
	if(path.IsDirectory())
		return CreateDirectories(path.Parent(), mode, breakpoint);

	RETURN_STATUS_IF_ERR(CreateDirectories(path.Parent(), mode));

	errno = 0;
	if(wmkdir(path, mode) != 0)
	{
		debug_printf("CreateDirectories: failed to mkdir %s (mode %d)\n", path.string8().c_str(), mode);
		if (breakpoint)
			WARN_RETURN(StatusFromErrno());
		else
			return StatusFromErrno();
	}

	return INFO::OK;
}


Status DeleteDirectory(const OsPath& path)
{
	// note: we have to recursively empty the directory before it can
	// be deleted (required by Windows and POSIX rmdir()).

	CFileInfos files; DirectoryNames subdirectoryNames;
	RETURN_STATUS_IF_ERR(GetDirectoryEntries(path, &files, &subdirectoryNames));

	// delete files
	for(size_t i = 0; i < files.size(); i++)
	{
		const OsPath pathname = path / files[i].Name();
		errno = 0;
		if(wunlink(pathname) != 0)
			WARN_RETURN(StatusFromErrno());
	}

	// recurse over subdirectoryNames
	for(size_t i = 0; i < subdirectoryNames.size(); i++)
		RETURN_STATUS_IF_ERR(DeleteDirectory(path / subdirectoryNames[i]));

	errno = 0;
	if(wrmdir(path) != 0)
		WARN_RETURN(StatusFromErrno());

	return INFO::OK;
}
