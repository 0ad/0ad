/**
 * =========================================================================
 * File        : dir_util.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "dir_util.h"

#include <queue>

#include "filesystem.h"
#include "path.h"
#include "lib/path_util.h"
#include "lib/regex.h"


bool dir_FileExists(IFilesystem* fs, const char* pathname)
{
	FilesystemEntry fsEntry;
	if(fs->GetEntry(pathname, fsEntry) < 0)
		return false;
	debug_assert(!fsEntry.IsDirectory());
	return true;
}


bool dir_DirectoryExists(IFilesystem* fs, const char* dirPath)
{
	FilesystemEntry fsEntry;
	if(fs->GetEntry(dirPath, fsEntry) < 0)
		return false;
	debug_assert(fsEntry.IsDirectory());
	return true;
}



struct FsEntryNameLess : public std::binary_function<const FilesystemEntry, const FilesystemEntry, bool>
{
	bool operator()(const FilesystemEntry& fsEntry1, const FilesystemEntry& fsEntry2) const
	{
		return strcmp(fsEntry1.name, fsEntry2.name) < 0;
	}
};

LibError dir_GatherSortedEntries(IFilesystem* fs, const char* dirPath, FilesystemEntries& fsEntries)
{
	DirectoryIterator di(fs, dirPath);
	fsEntries.reserve(50);	// preallocate for efficiency

	FilesystemEntry fsEntry;
	for(;;)
	{
		LibError ret = di.NextEntry(fsEntry);
		if(ret == ERR::DIR_END)
			break;
		RETURN_ERR(ret);
		fsEntries.push_back(fsEntry);
	}

	std::sort(fsEntries.begin(), fsEntries.end(), FsEntryNameLess());

	return INFO::OK;
}


LibError dir_ForEachSortedEntry(IFilesystem* fs, const char* dirPath, const DirCallback cb, const uintptr_t cbData)
{
	PathPackage pp;
	RETURN_ERR(path_package_set_dir(&pp, dirPath));

	FilesystemEntries fsEntries;
	RETURN_ERR(dir_GatherSortedEntries(fs, dirPath, fsEntries));

	for(FilesystemEntries::const_iterator it = fsEntries.begin(); it != fsEntries.end(); ++it)
	{
		const FilesystemEntry& fsEntry = *it;
		path_package_append_file(&pp, fsEntry.name);

		LibError ret = cb(pp.path, fsEntry, cbData);
		if(ret != INFO::CB_CONTINUE)
			return ret;
	}

	return INFO::OK;
}


LibError dir_filtered_next_ent(DirectoryIterator& di, FilesystemEntry& fsEntry, const char* filter)
{
	bool want_dir = true;
	if(filter)
	{
		// directory
		if(filter[0] == '/')
		{
			// .. and also files
			if(filter[1] == '|')
				filter += 2;
		}
		// file only
		else
			want_dir = false;
	}

	// loop until fsEntry matches what is requested, or end of directory.
	for(;;)
	{
		RETURN_ERR(di.NextEntry(fsEntry));

		if(fsEntry.IsDirectory())
		{
			if(want_dir)
				break;
		}
		else
		{
			// (note: filter = 0 matches anything)
			if(match_wildcard(fsEntry.name, filter))
				break;
		}
	}

	return INFO::OK;
}


// call <cb> for each fsEntry matching <user_filter> (see vfs_next_dirent) in
// directory <path>; if flags & VFS_DIR_RECURSIVE, entries in
// subdirectories are also returned.
//
// note: EnumDirEntsCB path and fsEntry are only valid during the callback.
LibError dir_FilteredForEachEntry(IFilesystem* fs, const char* dirPath, uint flags, const char* user_filter, DirCallback cb, uintptr_t cbData)
{
	debug_assert((flags & ~(VFS_DIR_RECURSIVE)) == 0);
	const bool recursive = (flags & VFS_DIR_RECURSIVE) != 0;

	char filter_buf[PATH_MAX];
	const char* filter = user_filter;
	bool user_filter_wants_dirs = true;
	if(user_filter)
	{
		if(user_filter[0] != '/')
			user_filter_wants_dirs = false;

		// we need subdirectories and the caller hasn't already requested them
		if(recursive && !user_filter_wants_dirs)
		{
			snprintf(filter_buf, sizeof(filter_buf), "/|%s", user_filter);
			filter = filter_buf;
		}
	}


	// note: FIFO queue instead of recursion is much more efficient
	// (less stack usage; avoids seeks by reading all entries in a
	// directory consecutively)

	std::queue<const char*> dir_queue;
	dir_queue.push(path_UniqueCopy(dirPath));

	// for each directory:
	do
	{
		// get current directory path from queue
		// note: can't refer to the queue contents - those are invalidated
		// as soon as a directory is pushed onto it.
		PathPackage pp;
		(void)path_package_set_dir(&pp, dir_queue.front());
		dir_queue.pop();

		DirectoryIterator di(fs, pp.path);

		// for each fsEntry (file, subdir) in directory:
		FilesystemEntry fsEntry;
		while(dir_filtered_next_ent(di, fsEntry, filter) == 0)
		{
			// build complete path (FilesystemEntry only stores fsEntry name)
			(void)path_package_append_file(&pp, fsEntry.name);
			const char* atom_path = path_UniqueCopy(pp.path);

			if(fsEntry.IsDirectory())
			{
				if(recursive)
					dir_queue.push(atom_path);

				if(user_filter_wants_dirs)
					cb(atom_path, fsEntry, cbData);
			}
			else
				cb(atom_path, fsEntry, cbData);
		}
	}
	while(!dir_queue.empty());

	return INFO::OK;
}



// fill V_next_fn (which must be big enough for PATH_MAX chars) with
// the next numbered filename according to the pattern defined by V_fn_fmt.
// <state> must be initially zeroed (e.g. by defining as static) and passed
// each time.
// if <use_vfs> (default), the paths are treated as VFS paths; otherwise,
// file.cpp's functions are used. this is necessary because one of
// our callers needs a filename for VFS archive files.
//
// this function is useful when creating new files which are not to
// overwrite the previous ones, e.g. screenshots.
// example for V_fn_fmt: "screenshots/screenshot%04d.png".
void dir_NextNumberedFilename(IFilesystem* fs, const char* fn_fmt, NextNumberedFilenameState* state, char* next_fn)
{
	// (first call only:) scan directory and set next_num according to
	// highest matching filename found. this avoids filling "holes" in
	// the number series due to deleted files, which could be confusing.
	// example: add 1st and 2nd; [exit] delete 1st; [restart]
	// add 3rd -> without this measure it would get number 1, not 3. 
	if(state->next_num == 0)
	{
		char dirPath[PATH_MAX];
		path_dir_only(fn_fmt, dirPath);
		const char* name_fmt = path_name_only(fn_fmt);

		int max_num = -1; int num;
		DirectoryIterator di(fs, dirPath);
		FilesystemEntry fsEntry;
		while(di.NextEntry(fsEntry) == INFO::OK)
		{
			if(!fsEntry.IsDirectory() && sscanf(fsEntry.name, name_fmt, &num) == 1)
				max_num = std::max(num, max_num);
		}

		state->next_num = max_num+1;
	}

	// now increment number until that file doesn't yet exist.
	// this is fairly slow, but typically only happens once due
	// to scan loop above. (we still need to provide for looping since
	// someone may have added files in the meantime)
	// binary search isn't expected to improve things.
	do
		snprintf(next_fn, PATH_MAX, fn_fmt, state->next_num++);
	while(dir_FileExists(fs, next_fn));
}
