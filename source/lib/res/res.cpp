#include "precompiled.h"

#include "res.h"
#include "file.h"

#ifdef _WIN32
#include "sysdep/win/wfam.h"
#else
#include <fam.h>
#endif


int res_reload(const char* const fn)
{
	return h_reload(fn);
}


static FAMConnection fc;
static bool initialized;



// path: portable and relative, must add current directory and convert to native
// better to use a cached string from rel_chdir - secure
int res_watch_dir(const char* const path, uint* const reqnum)
{
	if(!initialized)
	{
		CHECK_ERR(FAMOpen2(&fc, "lib_res"));
		atexit2((void *)FAMClose, (uintptr_t)&fc);
		initialized = true;
	}

	char n_full_path[PATH_MAX];
	CHECK_ERR(file_make_native_path(path, n_full_path));

	FAMRequest req;
	if(FAMMonitorDirectory(&fc, n_full_path, &req, (void*)0) < 0)
		return -1;	// no way of getting error?

	*reqnum = req.reqnum;
	return 0;
}


int res_cancel_watch(const uint reqnum)
{
	if(!initialized)
	{
		debug_warn("res_cancel_watch before res_watch_dir");
		return -1;
	}

	FAMRequest req;
	req.reqnum = reqnum;
	return FAMCancelMonitor(&fc, &req);
}


// purpose of this routine (intended to be called once a frame):
// file notification may come at any time. by forcing the reloads
// to take place here, we don't require everything to be thread-safe.
int res_reload_changed_files()
{
	if(!initialized)
		return -1;

	FAMEvent e;
	while(FAMPending(&fc) > 0)
		if(FAMNextEvent(&fc, &e) == 0)
		{
			char path[PATH_MAX];
			char vfs_path[VFS_MAX_PATH];
			CHECK_ERR(file_make_portable_path(e.filename, path));
			CHECK_ERR(vfs_get_path(path, vfs_path));
			res_reload(vfs_path);
		}

	return 0;
}
