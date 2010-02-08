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

#include "precompiled.h"
#include "file_system.h"

#include <vector>
#include <algorithm>
#include <string>

#include "lib/path_util.h"
#include "lib/wchar.h"	// wstring_from_utf8
#include "lib/posix/posix_filesystem.h"


struct DirDeleter
{
	void operator()(WDIR* osDir) const
	{
		const int ret = wclosedir(osDir);
		debug_assert(ret == 0);
	}
};

// is name "." or ".."?
static bool IsDummyDirectory(const std::wstring& name)
{
	if(name[0] != '.')
		return false;
	return (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

LibError GetDirectoryEntries(const fs::wpath& path, FileInfos* files, DirectoryNames* subdirectoryNames)
{
	// open directory
	errno = 0;
	WDIR* pDir = wopendir(path.string().c_str());
	if(!pDir)
		return LibError_from_errno(false);
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
			return LibError_from_errno();
		}

		const std::wstring name(osEnt->d_name);
		RETURN_ERR(path_component_validate(name.c_str()));

		// get file information (mode, size, mtime)
		struct stat s;
#if OS_WIN
		// .. return wdirent directly (much faster than calling stat).
		RETURN_ERR(wreaddir_stat_np(osDir.get(), &s));
#else
		// .. call regular stat().
		errno = 0;
		const fs::wpath pathname(path/name);
		if(wstat(pathname.string().c_str(), &s) != 0)
			return LibError_from_errno();
#endif

		if(files && S_ISREG(s.st_mode))
			files->push_back(FileInfo(name, s.st_size, s.st_mtime));
		else if(subdirectoryNames && S_ISDIR(s.st_mode) && !IsDummyDirectory(name))
			subdirectoryNames->push_back(name);
	}
}


LibError GetFileInfo(const fs::wpath& pathname, FileInfo* pfileInfo)
{
	errno = 0;
	struct stat s;
	memset(&s, 0, sizeof(s));
	if(wstat(pathname.string().c_str(), &s) != 0)
		return LibError_from_errno();

	*pfileInfo = FileInfo(pathname.leaf(), s.st_size, s.st_mtime);
	return INFO::OK;
}


LibError CreateDirectories(const fs::wpath& path, mode_t mode)
{
	if(path.empty() || fs::exists(path))
	{
		if(!path.empty() && !fs::is_directory(path))	// encountered a file
			WARN_RETURN(ERR::FAIL);
		return INFO::OK;
	}

	// If we were passed a path ending with '/', strip the '/' now so that
	// we can consistently use branch_path to find parent directory names
	if (path.leaf() == L".")
		return CreateDirectories(path.branch_path(), mode);

	RETURN_ERR(CreateDirectories(path.branch_path(), mode));

	errno = 0;
	if(wmkdir(path.string().c_str(), mode) != 0)
		return LibError_from_errno();

	return INFO::OK;
}


LibError DeleteDirectory(const fs::wpath& path)
{
	// note: we have to recursively empty the directory before it can
	// be deleted (required by Windows and POSIX rmdir()).

	FileInfos files; DirectoryNames subdirectoryNames;
	RETURN_ERR(GetDirectoryEntries(path, &files, &subdirectoryNames));

	// delete files
	for(size_t i = 0; i < files.size(); i++)
	{
		const fs::wpath pathname(path/files[i].Name());
		errno = 0;
		if(wunlink(pathname.string().c_str()) != 0)
			return LibError_from_errno();
	}

	// recurse over subdirectoryNames
	for(size_t i = 0; i < subdirectoryNames.size(); i++)
		RETURN_ERR(DeleteDirectory(path/subdirectoryNames[i]));

	errno = 0;
	if(wrmdir(path.string().c_str()) != 0)
		return LibError_from_errno();

	return INFO::OK;
}
