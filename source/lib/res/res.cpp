#include "precompiled.h"

#include "res.h"
#include "file.h"		// file_make_native_path, file_invalidate_cache
#include "timer.h"
#include "hotload.h"	// we implement that interface

#include "sysdep/dir_watch.h"

#include <string.h>


// called by res_reload and res_reload_changed_files (which will already
// have rebuilt the VFS - doing so more than once a frame is unnecessary).
static int reload_without_rebuild(const char* fn)
{
	// invalidate this file's cached blocks to make sure its contents are
	// loaded anew.
	CHECK_ERR(file_invalidate_cache(fn));

	CHECK_ERR(h_reload(fn));

	return 0;
}


// called via console command.
int res_reload(const char* fn)
{
	// if <fn> currently maps to an archive, the VFS must switch
	// over to using the loose file (that was presumably changed).
	CHECK_ERR(vfs_rebuild());

	return reload_without_rebuild(fn);
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
	// array of reloads requested this frame (see 'do we really need to
	// reload' below). go through gyrations to avoid heap allocs.
	const size_t MAX_RELOADS_PER_FRAME = 12;
	typedef char Path[VFS_MAX_PATH];
	typedef Path PathList[MAX_RELOADS_PER_FRAME];
	PathList pending_reloads;

	uint num_pending = 0;
	// process only as many notifications as we have room for; the others
	// will be handled next frame. it's not imagineable that they'll pile up.
	while(num_pending < MAX_RELOADS_PER_FRAME)
	{
		// get next notification
		char n_path[PATH_MAX];
		int ret = dir_get_changed_file(n_path);
		CHECK_ERR(ret);	// error? (doesn't cover 'none available')
		if(ret != 0)	// none available; done.
			break;

		// convert to VFS path
		char p_path[PATH_MAX];
		CHECK_ERR(file_make_full_portable_path(n_path, p_path));
		char* vfs_path = pending_reloads[num_pending];
		CHECK_ERR(vfs_make_vfs_path(p_path, vfs_path));

		// do we really need to reload? try to avoid the considerable cost of
		// rebuilding VFS and scanning all Handles.
		//
		// note: be careful to avoid 'race conditions' depending on the
		// timeframe in which notifications reach us.
		// example: editor deletes a.tga; we are notified; reload is
		// triggered but fails since the file isn't found; further
		// notifications (e.g. renamed a.tmp to a.tga) come within x [ms] and
		// are ignored due to a time limit.
		// therefore, we can only check for multiple reload requests a frame;
		// to that purpose, an array is built and duplicates ignored.
		const char* ext = strrchr(vfs_path, '.');
		// .. directory change notification; ignore because we get
		//    per-file notifications anyway. (note: assume no extension =>
		//    it's a directory). this also protects the strcmp calls below.
		if(!ext)
			continue;
		// .. compiled XML files the engine writes out by the hundreds;
		//    skipping them is a big performance gain.
		if(!strcmp(ext, ".xmb"))
			continue;
		// .. temp files, usually created when an editor saves a file
		//    (delete, create temp, rename temp); no need to reload those.
		if(!strcmp(ext, ".tmp"))
			continue;
		// .. more than one notification for a file; only reload once.
		//    note: this doesn't suffer from the 'reloaded too early'
		//    problem described above; if there's more than one
		//    request in the array, the file has since been written.
		for(uint i = 0; i < num_pending; i++)
			if(!strcmp(pending_reloads[i], vfs_path))
				continue;

		// path has already been written to pending_reloads,
		// so just mark it valid.
		num_pending++;		
	}

	// rebuild VFS, in case a file that has been changed is currently
	// mounted from an archive (reloading would just grab the unchanged
	// version in the archive). the rebuild sees differing mtimes and
	// always choses the loose file version. only do this once
	// (instead of per reload request) because it's slow (> 1s)!
	if(num_pending != 0)
		CHECK_ERR(vfs_rebuild());

	// now actually reload all files in the array we built
	for(uint i = 0; i < num_pending; i++)
		CHECK_ERR(reload_without_rebuild(pending_reloads[i]));

	return 0;
}
