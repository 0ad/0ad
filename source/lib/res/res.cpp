#include "precompiled.h"

#include "res.h"
#include "file.h"
#include "timer.h"

#ifdef _WIN32
#include "sysdep/win/wfam.h"
#else
#include <fam.h>
#endif

#include <string.h>

int res_reload(const char* const fn)
{
	return h_reload(fn);
}


static FAMConnection fc;
static bool initialized;



// path: portable and relative, must add current directory and convert to native
// better to use a cached string from rel_chdir - secure
int res_watch_dir(const char* const path, uintptr_t* const watch)
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

	*watch = req.reqnum;
	return 0;
}


int res_cancel_watch(const uint watch)
{
	if(!initialized)
	{
		debug_warn("res_cancel_watch before res_watch_dir");
		return -1;
	}

	FAMRequest req;
	req.reqnum = watch;
	return FAMCancelMonitor(&fc, &req);
}


// purpose of this routine (intended to be called once a frame):
// file notification may come at any time. by forcing the reloads
// to take place here, we don't require everything to be thread-safe.
int res_reload_changed_files()
{
	if(!initialized)
		return -1;

	static double last_time;
	static char last_fn[VFS_MAX_PATH];

	FAMEvent e;
	while(FAMPending(&fc) > 0)
	{
		if(FAMNextEvent(&fc, &e) < 0)
			continue;

		char path[PATH_MAX];
		char vfs_path[VFS_MAX_PATH];
		CHECK_ERR(file_make_portable_path(e.filename, path));
		CHECK_ERR(vfs_get_path(path, vfs_path));

		const char* fn = vfs_path;

		// many apps save by creating a temp file, deleting the original,
		// and renaming the temp file. that leads to 2 reloads, which is slow.
		// so:
		// .. ignore temp files,
		char* ext = strrchr(fn, '.');
		if(ext && !strcmp(ext, ".tmp"))
			continue;
		// .. directory change (more info is upcoming anyway)
		if(!ext && e.code == FAMChanged)	// dir changed
			continue;
		// .. and reloads for the same file within a small timeframe.
		double cur_time = get_time();
		if(cur_time - last_time < 50e-3 && !strcmp(last_fn, fn))
			continue;

		res_reload(fn);
debug_out("%s\n\n", fn);

		last_time = get_time();
		strcpy(last_fn, fn);
	}

	return 0;
}
