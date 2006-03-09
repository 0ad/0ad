#include "precompiled.h"

#include "VFSUtil.h"
#include "lib/res/file/vfs.h"

#include "CLogger.h"
#define LOG_CATEGORY "vfs"

#include <deque>

using namespace VFSUtil;

// Because I'm lazy, and it saves a few lines of code in other places:
bool VFSUtil::FindFiles (const CStr& dirname, const char* filter, FileList& files)
{
	files.clear();

	Handle dir = vfs_dir_open(dirname);
	if (dir <= 0)
	{
		LOG(ERROR, LOG_CATEGORY, "Error opening directory '%s' (%lld)", dirname.c_str(), dir);
		return false;
	}

	int err;
	DirEnt entry;
	while ((err = vfs_dir_next_ent(dir, &entry, filter)) == 0)
	{
		files.push_back(dirname+"/"+entry.name);
	}

	if (err != ERR_DIR_END)
	{
		LOG(ERROR, LOG_CATEGORY, "Error reading files from directory '%s' (%d)", dirname.c_str(), err);
		return false;
	}

	vfs_dir_close(dir);

	return true;

}


// call <cb> for each entry matching <user_filter> (see vfs_next_dirent) in
// directory <path>; if <recursive>, entries in subdirectories are
// also returned.
//
// note: EnumDirEntsCB path and ent are only valid during the callback.
LibError VFSUtil::EnumDirEnts(const CStr start_path, int flags, const char* user_filter,
	EnumDirEntsCB cb, void* context)
{
	debug_assert((flags & ~(RECURSIVE)) == 0);
	const bool recursive = (flags & RECURSIVE) != 0;

	char filter_buf[VFS_MAX_PATH];
	const char* filter = user_filter;
	bool want_dir = true;
	if(user_filter)
	{
		if(user_filter[0] != '/')
			want_dir = false;

		// we need subdirectories and the caller hasn't already requested them
		if(recursive && !want_dir)
		{
			snprintf(filter_buf, sizeof(filter_buf), "/|%s", user_filter);
			filter = filter_buf;
		}
	}


	// note: FIFO queue instead of recursion is much more efficient
	// (less stack usage; avoids seeks by reading all entries in a
	// directory consecutively)

	std::queue<const char*> dir_queue;
	dir_queue.push(file_make_unique_fn_copy(start_path.c_str()));

	// for each directory:
	do
	{
		// get current directory path from queue
		// note: can't refer to the queue contents - those are invalidated
		// as soon as a directory is pushed onto it.
		PathPackage pp;
		(void)pp_set_dir(&pp, dir_queue.front());
		dir_queue.pop();

		Handle hdir = vfs_dir_open(pp.path);
		if(hdir <= 0)
		{
			debug_warn("vfs_open_dir failed");
			continue;
		}

		// for each entry (file, subdir) in directory:
		DirEnt ent;
		while(vfs_dir_next_ent(hdir, &ent, filter) == 0)
		{
			// build complete path (DirEnt only stores entry name)
			(void)pp_append_file(&pp, ent.name);
			const char* atom_path = file_make_unique_fn_copy(pp.path);

			if(DIRENT_IS_DIR(&ent))
			{
				if(recursive)
					dir_queue.push(atom_path);

				if(want_dir)
					cb(atom_path, &ent, context);
			}
			else
				cb(atom_path, &ent, context);
		}

		vfs_dir_close(hdir);
	}
	while(!dir_queue.empty());

	return ERR_OK;
}
