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

/**
 * =========================================================================
 * File        : file_system_util.cpp
 * Project     : 0 A.D.
 * Description : helper functions for directory access
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "file_system_util.h"

#include <queue>
#include <cstring>

#include "lib/path_util.h"
#include "lib/regex.h"

LibError fs_GetPathnames(const PIVFS& fs, const VfsPath& path, const char* filter, VfsPaths& pathnames)
{
	std::vector<FileInfo> files;
	RETURN_ERR(fs->GetDirectoryEntries(path, &files, 0));

	pathnames.clear();
	pathnames.reserve(files.size());

	for(size_t i = 0; i < files.size(); i++)
	{
		if(match_wildcard(files[i].Name().c_str(), filter))
			pathnames.push_back(path/files[i].Name());
	}

	return INFO::OK;
}


struct FileInfoNameLess : public std::binary_function<const FileInfo, const FileInfo, bool>
{
	bool operator()(const FileInfo& fileInfo1, const FileInfo& fileInfo2) const
	{
		return strcasecmp(fileInfo1.Name().c_str(), fileInfo2.Name().c_str()) < 0;
	}
};

void fs_SortFiles(FileInfos& files)
{
	std::sort(files.begin(), files.end(), FileInfoNameLess());
}


struct NameLess : public std::binary_function<const std::string, const std::string, bool>
{
	bool operator()(const std::string& name1, const std::string& name2) const
	{
		return strcasecmp(name1.c_str(), name2.c_str()) < 0;
	}
};

void fs_SortDirectories(DirectoryNames& directories)
{
	std::sort(directories.begin(), directories.end(), NameLess());
}


LibError fs_ForEachFile(const PIVFS& fs, const VfsPath& startPath, FileCallback cb, uintptr_t cbData, const char* pattern, size_t flags)
{
	debug_assert(vfs_path_IsDirectory(startPath));

	// (declare here to avoid reallocations)
	FileInfos files; DirectoryNames subdirectoryNames;

	// (a FIFO queue is more efficient than recursion because it uses less
	// stack space and avoids seeks due to breadth-first traversal.)
	std::queue<VfsPath> pendingDirectories;
	pendingDirectories.push(startPath);
	while(!pendingDirectories.empty())
	{
		const VfsPath& path = pendingDirectories.front();

		RETURN_ERR(fs->GetDirectoryEntries(path/"/", &files, &subdirectoryNames));

		for(size_t i = 0; i < files.size(); i++)
		{
			const FileInfo fileInfo = files[i];
			if(!match_wildcard(fileInfo.Name().c_str(), pattern))
				continue;

			const VfsPath pathname(path/fileInfo.Name());	// (FileInfo only stores the name)
			cb(pathname, fileInfo, cbData);
		}

		if(!(flags & DIR_RECURSIVE))
			break;

		for(size_t i = 0; i < subdirectoryNames.size(); i++)
			pendingDirectories.push(path/subdirectoryNames[i]);
		pendingDirectories.pop();
	}

	return INFO::OK;
}


void fs_NextNumberedFilename(const PIVFS& fs, const VfsPath& pathnameFormat, size_t& nextNumber, VfsPath& nextPathname)
{
	// (first call only:) scan directory and set nextNumber according to
	// highest matching filename found. this avoids filling "holes" in
	// the number series due to deleted files, which could be confusing.
	// example: add 1st and 2nd; [exit] delete 1st; [restart]
	// add 3rd -> without this measure it would get number 1, not 3. 
	if(nextNumber == 0)
	{
		const std::string nameFormat = pathnameFormat.leaf();
		const VfsPath path = pathnameFormat.branch_path()/"/";

		size_t maxNumber = 0;
		FileInfos files;
		fs->GetDirectoryEntries(path, &files, 0);
		for(size_t i = 0; i < files.size(); i++)
		{
			size_t number;
			if(sscanf(files[i].Name().c_str(), nameFormat.c_str(), &number) == 1)
				maxNumber = std::max(number, maxNumber);
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
		char pathnameBuf[PATH_MAX];
		snprintf(pathnameBuf, PATH_MAX, pathnameFormat.string().c_str(), nextNumber++);
		nextPathname = VfsPath(pathnameBuf);
	}
	while(fs->GetFileInfo(nextPathname, 0) == INFO::OK);
}
