#include "precompiled.h"

#include "VFSUtil.h"
#include "lib/res/vfs.h"

#include "CLogger.h"
#define LOG_CATEGORY "vfs"

using namespace VFSUtil;

// Because I'm lazy, and it saves a few lines of code in other places:
bool VFSUtil::FindFiles (CStr dirname, const char* filter, FileList& files)
{
	files.clear();

	Handle dir = vfs_open_dir(dirname.c_str());
	if (dir <= 0)
	{
		LOG(ERROR, LOG_CATEGORY, "Error opening directory '%s' (%lld)", dirname.c_str(), dir);
		return false;
	}

	int err;
	vfsDirEnt entry;
	while ((err = vfs_next_dirent(dir, &entry, filter)) == 0)
	{
		files.push_back(dirname+"/"+entry.name);
	}

	if (err != ERR_DIR_END)
	{
		LOG(ERROR, LOG_CATEGORY, "Error reading files from directory '%s' (%d)", dirname.c_str(), err);
		return false;
	}

	vfs_close_dir(dir);

	return true;

}
