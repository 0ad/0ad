#include "precompiled.h"

#include "res.h"
#include "file.h"		// file_make_native_path, file_invalidate_cache
#include "timer.h"
#include "hotload.h"	// we implement that interface

#include "sysdep/dir_watch.h"

#include <string.h>


// called from res_reload_changed_files and via console.
int res_reload(const char* fn)
{
	// if <fn> currently maps to an archive, the VFS must switch
	// over to using the loose file (that was presumably changed).
	vfs_rebuild();

	// invalidate this file's cached blocks to make sure its contents are
	// loaded anew.
	file_invalidate_cache(fn);

	return h_reload(fn);
}


int res_watch_dir(const char* path, intptr_t* watch)
{
	char n_path[PATH_MAX];
	CHECK_ERR(file_make_full_native_path(path, n_path));
	return dir_add_watch(n_path, watch);
}


int res_cancel_watch(const intptr_t watch)
{
	return dir_cancel_watch(watch);
}


// get directory change notifications, and reload all affected files.
// must be called regularly (e.g. once a frame). this is much simpler
// than asynchronous notifications: everything would need to be thread-safe.
int res_reload_changed_files()
{
	char n_path[PATH_MAX];
	while(dir_get_changed_file(n_path) == 0)
	{
		// convert to VFS path
		char p_path[PATH_MAX];
		CHECK_ERR(file_make_full_portable_path(n_path, p_path));
		char vfs_path[VFS_MAX_PATH];
		CHECK_ERR(vfs_make_vfs_path(p_path, vfs_path));

		// various early-out checks that reduce debug output clutter,
		// avoid the overhead of searching through all Handles, and
		// try to eliminate repeated reloads:
		const char* ext = strrchr(vfs_path, '.');
		// .. ignore directory change notifications, because we get
		// per-file notifications anyway. (note: assume no extension =>
		// it's a directory). this also protects the strcmp calls below.
		if(!ext)
			continue;
		// .. ignore files that can't be reloaded anyway.
		if(!strcmp(ext, ".xmb"))
			continue;
		// .. skip temp files, because many apps save by creating a temp
		// file, deleting the original, and renaming the temp file.
		// => avoids 2 redundant reloads.
		if(!strcmp(ext, ".tmp"))
			continue;

		int ret = res_reload(vfs_path);
		assert(ret == 0);
	}

	return 0;
}
