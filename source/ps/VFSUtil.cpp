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
		CStr path = dirname+entry.name;
		if(DIRENT_IS_DIR(&entry))
			path += '/';
		files.push_back(path);
	}

	if (err != ERR_DIR_END)
	{
		LOG(ERROR, LOG_CATEGORY, "Error reading files from directory '%s' (%d)", dirname.c_str(), err);
		return false;
	}

	vfs_dir_close(dir);

	return true;

}

