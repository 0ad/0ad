/**
 * =========================================================================
 * File        : dir_util.cpp
 * Project     : 0 A.D.
 * Description : helper functions for directory access
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "dir_util.h"

#include <queue>
#include <cstring>

#include "lib/path_util.h"
#include "lib/regex.h"


bool dir_FileExists(IFilesystem* fs, const char* pathname)
{
	FileInfo fileInfo;
	if(fs->GetFileInfo(pathname, fileInfo) < 0)
		return false;
	return true;
}


struct FileInfoNameLess : public std::binary_function<const FileInfo, const FileInfo, bool>
{
	bool operator()(const FileInfo& fileInfo1, const FileInfo& fileInfo2) const
	{
		return strcmp(fileInfo1.Name(), fileInfo2.Name()) < 0;
	}
};

void dir_SortFiles(FileInfos& files)
{
	std::sort(files.begin(), files.end(), FileInfoNameLess());
}


struct NameLess : public std::binary_function<const char*, const char*, bool>
{
	bool operator()(const char* name1, const char* name2) const
	{
		return strcmp(name1, name2) < 0;
	}
};

void dir_SortDirectories(Directories& directories)
{
	std::sort(directories.begin(), directories.end(), NameLess());
}


LibError dir_FilteredForEachEntry(IFilesystem* fs, const char* dirPath, DirCallback cb, uintptr_t cbData, const char* pattern, uint flags)
{
	debug_assert((flags & ~(VFS_DIR_RECURSIVE)) == 0);

	// (declare here to avoid reallocations)
	FileInfos files; Directories subdirectories;

	// (a FIFO queue is more efficient than recursion because it uses less
	// stack space and avoids seeks due to breadth-first traversal.)
	std::queue<const char*> dir_queue;
	dir_queue.push(path_Pool()->UniqueCopy(dirPath));
	while(!dir_queue.empty())
	{
		// get directory path
		PathPackage pp;
		(void)path_package_set_dir(&pp, dir_queue.front());
		dir_queue.pop();

		RETURN_ERR(fs->GetDirectoryEntries(pp.path, &files, &subdirectories));

		for(size_t i = 0; i < files.size(); i++)
		{
			const FileInfo fileInfo = files[i];
			if(!match_wildcard(fileInfo.Name(), pattern))
				continue;

			// build complete path (FileInfo only stores name)
			(void)path_package_append_file(&pp, fileInfo.Name());
			cb(pp.path, fileInfo, cbData);
		}

		if((flags & VFS_DIR_RECURSIVE) == 0)
			break;

		for(size_t i = 0; i < subdirectories.size(); i++)
		{
			(void)path_package_append_file(&pp, subdirectories[i]);
			dir_queue.push(path_Pool()->UniqueCopy(pp.path));
		}
	}

	return INFO::OK;
}


void dir_NextNumberedFilename(IFilesystem* fs, const char* pathnameFmt, unsigned& nextNumber, char* nextPathname)
{
	// (first call only:) scan directory and set nextNumber according to
	// highest matching filename found. this avoids filling "holes" in
	// the number series due to deleted files, which could be confusing.
	// example: add 1st and 2nd; [exit] delete 1st; [restart]
	// add 3rd -> without this measure it would get number 1, not 3. 
	if(nextNumber == 0)
	{
		const char* path; const char* nameFmt;
		path_split(pathnameFmt, &path, &nameFmt);

		unsigned maxNumber = 0;
		FileInfos files;
		fs->GetDirectoryEntries(path, &files, 0);
		for(size_t i = 0; i < files.size(); i++)
		{
			unsigned number;
			if(sscanf(files[i].Name(), nameFmt, &number) == 1)
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
		snprintf(nextPathname, PATH_MAX, pathnameFmt, nextNumber++);
	while(dir_FileExists(fs, nextPathname));
}
