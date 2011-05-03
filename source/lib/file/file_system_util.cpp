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

/*
 * helper functions for directory access
 */

#include "precompiled.h"
#include "lib/file/file_system_util.h"

#include <queue>
#include <cstring>
#include <cstdio>

#include "lib/sysdep/filesystem.h"
#include "lib/regex.h"


namespace fs_util {

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


Status GetPathnames(const PIVFS& fs, const VfsPath& path, const wchar_t* filter, VfsPaths& pathnames)
{
	std::vector<FileInfo> files;
	RETURN_STATUS_IF_ERR(fs->GetDirectoryEntries(path, &files, 0));

	pathnames.clear();
	pathnames.reserve(files.size());

	for(size_t i = 0; i < files.size(); i++)
	{
		if(match_wildcard(files[i].Name().string().c_str(), filter))
			pathnames.push_back(path / files[i].Name());
	}

	return INFO::OK;
}


Status ForEachFile(const PIVFS& fs, const VfsPath& startPath, FileCallback cb, uintptr_t cbData, const wchar_t* pattern, size_t flags)
{
	// (declare here to avoid reallocations)
	FileInfos files; DirectoryNames subdirectoryNames;

	// (a FIFO queue is more efficient than recursion because it uses less
	// stack space and avoids seeks due to breadth-first traversal.)
	std::queue<VfsPath> pendingDirectories;
	pendingDirectories.push(startPath/"");
	while(!pendingDirectories.empty())
	{
		const VfsPath& path = pendingDirectories.front();

		RETURN_STATUS_IF_ERR(fs->GetDirectoryEntries(path, &files, &subdirectoryNames));

		for(size_t i = 0; i < files.size(); i++)
		{
			const FileInfo fileInfo = files[i];
			if(!match_wildcard(fileInfo.Name().string().c_str(), pattern))
				continue;

			const VfsPath pathname(path / fileInfo.Name());	// (FileInfo only stores the name)
			cb(pathname, fileInfo, cbData);
		}

		if(!(flags & DIR_RECURSIVE))
			break;

		for(size_t i = 0; i < subdirectoryNames.size(); i++)
			pendingDirectories.push(path / subdirectoryNames[i]/"");
		pendingDirectories.pop();
	}

	return INFO::OK;
}


void NextNumberedFilename(const PIVFS& fs, const VfsPath& pathnameFormat, size_t& nextNumber, VfsPath& nextPathname)
{
	// (first call only:) scan directory and set nextNumber according to
	// highest matching filename found. this avoids filling "holes" in
	// the number series due to deleted files, which could be confusing.
	// example: add 1st and 2nd; [exit] delete 1st; [restart]
	// add 3rd -> without this measure it would get number 1, not 3. 
	if(nextNumber == 0)
	{
		const VfsPath nameFormat = pathnameFormat.Filename();
		const VfsPath path = pathnameFormat.Parent()/"";

		size_t maxNumber = 0;
		FileInfos files;
		fs->GetDirectoryEntries(path, &files, 0);
		for(size_t i = 0; i < files.size(); i++)
		{
			int number;
			if(swscanf_s(files[i].Name().string().c_str(), nameFormat.string().c_str(), &number) == 1)
				maxNumber = std::max(size_t(number), maxNumber);
		}

		nextNumber = maxNumber+1;
	}

	// now increment number until that file doesn't yet exist.
	// this is fairly slow, but typically only happens once due
	// to scan loop above. (we still need to provide for looping since
	// someone may have added files in the meantime)
	// we don't bother with binary search - this isn't a bottleneck.
	do
	{
		wchar_t pathnameBuf[PATH_MAX];
		swprintf_s(pathnameBuf, ARRAY_SIZE(pathnameBuf), pathnameFormat.string().c_str(), nextNumber++);
		nextPathname = pathnameBuf;
	}
	while(fs->GetFileInfo(nextPathname, 0) == INFO::OK);
}

}	// namespace fs_util
