#include "precompiled.h"

#include "res.h"
#include "file.h"
#include "timer.h"

#include "sysdep/dir_watch.h"

#include <string.h>

int res_reload(const char* const fn)
{
	return h_reload(fn);
}


int res_watch_dir(const char* const path, intptr_t* const watch)
{
	char n_path[PATH_MAX];
	CHECK_ERR(file_make_native_path(path, n_path));
	return dir_add_watch(n_path, watch);
}


int res_cancel_watch(const intptr_t watch)
{
	return dir_cancel_watch(watch);
}


// purpose of this routine (intended to be called once a frame):
// file notification may come at any time. by forcing the reloads
// to take place here, we don't require everything to be thread-safe.
int res_reload_changed_files()
{
	static double last_time;
	static char last_fn[VFS_MAX_PATH];

	char path[PATH_MAX];
	while(dir_get_changed_file(path) == 0)
	{
		char vfs_path[VFS_MAX_PATH];
		CHECK_ERR(vfs_make_vfs_path(path, vfs_path));

		const char* fn = vfs_path;

		char* ext = strrchr(fn, '.');

		// slight optimization (and reduces output clutter):
		// don't reload XMB output files
		if(ext && !strcmp(ext, ".xmb"))
			continue;

		// many apps save by creating a temp file, deleting the original,
		// and renaming the temp file. that leads to 2 reloads, which is slow.
		// so:
		// .. ignore temp files,
		if(ext && !strcmp(ext, ".tmp"))
				continue;
		// .. and directory change (more info is upcoming anyway)
		if(!ext)	// dir changed
			continue;
		// .. and reloads for the same file within a small timeframe.
		double cur_time = get_time();
		if(cur_time - last_time < 50e-3 && !strcmp(last_fn, fn))
			continue;

debug_out("res_reload %s\n\n", fn);
		res_reload(fn);

		last_time = get_time();
		strcpy(last_fn, fn);
	}

	return 0;
}
