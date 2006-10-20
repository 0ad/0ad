/**
 * =========================================================================
 * File        : vfs.cpp
 * Project     : 0 A.D.
 * Description : Handle-based wrapper on top of the vfs_mount API.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"

#include <string.h>
#include <time.h>
#include <math.h>

#include <map>
#include <list>
#include <deque>
#include <vector>
#include <string>
#include <algorithm>

#include "lib/lib.h"
#include "lib/adts.h"
#include "lib/timer.h"
#include "lib/res/res.h"
#include "lib/sysdep/dir_watch.h"
#include "file_internal.h"

// not safe to call before main!


// pathnames are case-insensitive.
// implementation:
//   when mounting, we get the exact filenames as reported by the OS;
//   we allow open requests with mixed case to match those,
//   but still use the correct case when passing to other libraries
//   (e.g. the actual open() syscall, called via file_open).
// rationale:
//   necessary, because some exporters output .EXT uppercase extensions
//   and it's unreasonable to expect that users will always get it right.


// rationale for no forcibly-close support:
// issue:
// we might want to edit files while the game has them open.
// usual case: edit file, notify engine that it should be reloaded.
// here: need to tell the engine to stop what it's doing and close the file;
// only then can the artist write to the file, and trigger a reload.
//
// work involved:
// since closing a file with pending aios results in undefined
// behavior on Win32, we would have to keep track of all aios from each file,
// and cancel them. we'd also need to notify the higher level resource user
// that its read was cancelled, as opposed to failing due to read errors
// (which might cause the game to terminate).
//
// this is just more work than benefit. cases where the game holds on to files
// are rare:
// - streaming music (artist can use regular commands to stop the current
//   track, or all music)
// - if the engine happens to be reading that file at the moment (expected
//   to happen only during loading, and these are usually one-shot anway,
//   i.e. it'll be done soon)
// - bug (someone didn't close a file - tough luck, and should be fixed
//   instead of hacking around it).
// - archives (these remain open. allowing reload would mean we'd have to keep
//   track of all files from an archive, and reload them all. another hassle.
//   anyway, if files are to be changed in-game, then change the plain-file
//   version - that's what they're for).


///////////////////////////////////////////////////////////////////////////////
//
// directory
//
///////////////////////////////////////////////////////////////////////////////


struct VDir
{
	DirIterator di;
	uint di_valid : 1;	// <di> will be closed iff == 1
};

H_TYPE_DEFINE(VDir);

static void VDir_init(VDir* UNUSED(vd), va_list UNUSED(args))
{
}

static void VDir_dtor(VDir* vd)
{
	// note: DirIterator has no way of checking if it's valid;
	// we must therefore only free it if reload() succeeded.
	if(vd->di_valid)
	{
		xdir_close(&vd->di);
		vd->di_valid = 0;
	}
}

static LibError VDir_reload(VDir* vd, const char* V_dir_path, Handle UNUSED(hvd))
{
	debug_assert(VFS_PATH_IS_DIR(V_dir_path));

	RETURN_ERR(xdir_open(V_dir_path, &vd->di));
	vd->di_valid = 1;
	return INFO::OK;
}

static LibError VDir_validate(const VDir* vd)
{
	// note: <di> is mostly opaque and cannot be validated.
	if(vd->di.filter && !isprint(vd->di.filter[0]))
		WARN_RETURN(ERR::_1);
	return INFO::OK;
}

static LibError VDir_to_string(const VDir* vd, char* buf)
{
	const char* filter = vd->di.filter;
	if(!vd->di.filter_latched)
		filter = "?";
	if(!filter)
		filter = "*";
	snprintf(buf, H_STRING_LEN, "(\"%s\")", filter);
	return INFO::OK;
}


// open a directory for reading its entries via vfs_next_dirent.
// V_dir must end in '/' to indicate it's a directory path.
Handle vfs_dir_open(const char* V_dir_path)
{
	// must disallow handle caching because this object is not
	// copy-equivalent (since the iterator is advanced by each user).
	return h_alloc(H_VDir, V_dir_path, RES_NO_CACHE);
}


// close the handle to a directory.
LibError vfs_dir_close(Handle& hd)
{
	return h_free(hd, H_VDir);
}


// retrieve the next (order is unspecified) dir entry matching <filter>.
// return 0 on success, ERR::DIR_END if no matching entry was found,
// or a negative error code on failure.
// filter values:
// - 0: anything;
// - "/": any subdirectory;
// - "/|<pattern>": any subdirectory, or as below with <pattern>;
// - <pattern>: any file whose name matches; ? and * wildcards are allowed.
//
// note that the directory entries are only scanned once; after the
// end is reached (-> ERR::DIR_END returned), no further entries can
// be retrieved, even if filter changes (which shouldn't happen - see impl).
//
// rationale: we do not sort directory entries alphabetically here.
// most callers don't need it and the overhead is considerable
// (we'd have to store all entries in a vector). it is left up to
// higher-level code such as VfsUtil.
LibError vfs_dir_next_ent(const Handle hd, DirEnt* ent, const char* filter)
{
	H_DEREF(hd, VDir, vd);
	return dir_filtered_next_ent(&vd->di, ent, filter);
}



///////////////////////////////////////////////////////////////////////////////
//
// file
//
///////////////////////////////////////////////////////////////////////////////


// return actual path to the specified file:
// "<real_directory>/fn" or "<archive_name>/fn".
LibError vfs_realpath(const char* V_path, char* realpath)
{
	TFile* tf;
	CHECK_ERR(tree_lookup(V_path, &tf));

	const char* atom_fn = tfile_get_atom_fn(tf);
	const Mount* m = tfile_get_mount(tf);
	return mount_realpath(atom_fn, m, realpath);
}



// does the specified file exist? return false on error.
// useful because a "file not found" warning is not raised, unlike vfs_stat.
bool vfs_exists(const char* V_fn)
{
	TFile* tf;
	return (tree_lookup(V_fn, &tf) == 0);
}


// get file status (mode, size, mtime). output param is zeroed on error.
LibError vfs_stat(const char* V_path, struct stat* s)
{
	memset(s, 0, sizeof(*s));

	TFile* tf;
	CHECK_ERR(tree_lookup(V_path, &tf));

	return tree_stat(tf, s);
}


//-----------------------------------------------------------------------------

struct VFile
{
	File f;

	// current file pointer. this is necessary because file.cpp's interface
	// requires passing an offset for every VIo; see file_io_issue.
	off_t ofs;

	// pointer to VFS file info storage; used to update size/mtime
	// after a newly written file is closed.
	TFile* tf;

	uint is_valid : 1;

	// be aware when adding fields that this struct is quite large,
	// and may require increasing the control block size limit.
};

H_TYPE_DEFINE(VFile);

static void VFile_init(VFile* vf, va_list args)
{
	vf->f.flags = va_arg(args, int);
}

static void VFile_dtor(VFile* vf)
{
	// note: checking if reload() succeeded is unnecessary because
	// xfile_close and mem_free_h safely handle 0-initialized data.
	WARN_ERR(xfile_close(&vf->f));

	// update file state in VFS tree
	// (must be done after close, since that calculates the size)
	if(vf->f.flags & FILE_WRITE)
		tree_update_file(vf->tf, vf->f.size, time(0));	// can't fail

	if(vf->is_valid)
		stats_close();
}

static LibError VFile_reload(VFile* vf, const char* V_path, Handle)
{
	const uint flags = vf->f.flags;

	// we're done if file is already open. need to check this because
	// reload order (e.g. if resource opens a file) is unspecified.
	if(xfile_is_open(&vf->f))
		return INFO::OK;

	TFile* tf;
	uint lf = (flags & FILE_WRITE)? LF_CREATE_MISSING : 0;
	LibError err = tree_lookup(V_path, &tf, lf);
	if(err < 0)
	{
		// don't CHECK_ERR - this happens often and the dialog is annoying
		debug_printf("lookup failed for %s\n", V_path);
		return err;
	}

	// careful! FILE_WRITE_TO_TARGET consists of 2 bits; they must both be
	// set (one of them is FILE_WRITE, which can be set independently).
	// this is a bit ugly but better than requiring users to write
	// FILE_WRITE|FILE_WRITE_TO_TARGET.
	if((flags & FILE_WRITE_TO_TARGET) == FILE_WRITE_TO_TARGET)
		RETURN_ERR(set_mount_to_write_target(tf));

	RETURN_ERR(xfile_open(V_path, flags, tf, &vf->f));

	stats_open(vf->f.atom_fn, vf->f.size);
	vf->is_valid = 1;
	vf->tf = tf;

	return INFO::OK;
}

static LibError VFile_validate(const VFile* vf)
{
	// <ofs> doesn't have any invariant we can check.
	RETURN_ERR(xfile_validate(&vf->f));
	return INFO::OK;
}

static LibError VFile_to_string(const VFile* UNUSED(vf), char* buf)
{
	strcpy(buf, "");	// safe
	return INFO::OK;
}


// return the size of an already opened file, or a negative error code.
ssize_t vfs_size(Handle hf)
{
	H_DEREF(hf, VFile, vf);
	return vf->f.size;
}


// open the file for synchronous or asynchronous VIo. write access is
// requested via FILE_WRITE flag, and is not possible for files in archives.
// file_flags: default 0
//
// on failure, a debug_warn is generated and a negative error code returned.
Handle vfs_open(const char* V_fn, uint file_flags)
{
	// keeping files open doesn't make sense in most cases (because the
	// file is used to load resources, which are cached at a higher level).
	uint res_flags = RES_NO_CACHE;

	// res_flags is for h_alloc and file_flags goes to VFile_init.
	// h_alloc already complains on error.
	return h_alloc(H_VFile, V_fn, res_flags, file_flags);
}


// close the handle to a file.
LibError vfs_close(Handle& hf)
{
	// h_free already complains on error.
	return h_free(hf, H_VFile);
}


// transfer the next <size> bytes to/from the given file.
// (read or write access was chosen at file-open time).
//
// if non-NULL, <cb> is called for each block transferred, passing <ctx>.
// it returns how much data was actually transferred, or a negative error
// code (in which case we abort the transfer and return that value).
// the callback mechanism is useful for user progress notification or
// processing data while waiting for the next I/O to complete
// (quasi-parallel, without the complexity of threads).
//
// p (value-return) indicates the buffer mode:
// - *p == 0: read into buffer we allocate; set *p.
//   caller should mem_free it when no longer needed.
// - *p != 0: read into or write into the buffer *p.
// - p == 0: only read into temp buffers. useful if the callback
//   is responsible for processing/copying the transferred blocks.
//   since only temp buffers can be added to the cache,
//   this is the preferred read method.
//
// return number of bytes transferred (see above), or a negative error code.
ssize_t vfs_io(const Handle hf, const size_t size, FileIOBuf* pbuf,
	FileIOCB cb, uintptr_t cb_ctx)
{
	debug_printf("VFS| io: size=%d\n", size);

	H_DEREF(hf, VFile, vf);
	File* f = &vf->f;

	stats_io_user_request(size);
	trace_notify_io(f->atom_fn, size, f->flags);

	off_t ofs = vf->ofs;
	vf->ofs += (off_t)size;

	ssize_t nbytes = xfile_io(&vf->f, ofs, size, pbuf, cb, cb_ctx);
	RETURN_ERR(nbytes);
	return nbytes;
}


// load the entire file <fn> into memory.
// p and size are filled with address/size of buffer (0 on failure).
// flags influences IO mode and is typically 0.
// when the file contents are no longer needed, call file_buf_free(buf).
LibError vfs_load(const char* V_fn, FileIOBuf& buf, size_t& size,
	uint file_flags, FileIOCB cb, uintptr_t cb_ctx)	// all default 0
{
	debug_printf("VFS| load: V_fn=%s\n", V_fn);

	const char* atom_fn = file_make_unique_fn_copy(V_fn);
	const uint fb_flags = (file_flags & FILE_LONG_LIVED)? FB_LONG_LIVED : 0;
	buf = file_cache_retrieve(atom_fn, &size, fb_flags);
	if(buf)
	{
		// we want to skip the below code (especially vfs_open) for
		// efficiency. that includes stats/trace accounting, though,
		// so duplicate that here:
		stats_cache(CR_HIT, size, atom_fn);
		stats_io_user_request(size);
		trace_notify_io(atom_fn, size, file_flags);

		size_t actual_size;
		LibError ret = file_io_call_back(buf, size, cb, cb_ctx, actual_size);
		if(ret < 0)
			file_buf_free(buf);
		// we don't care if the cb has "had enough" or whether it would
		// accept more data - this is all it gets and we need to
		// translate return value to avoid confusing callers.
		if(ret == INFO::CB_CONTINUE)
			ret = INFO::OK;
		size = actual_size;
		return ret;
	}

	buf = 0; size = 0;	// initialize in case something below fails

	Handle hf = vfs_open(atom_fn, file_flags);
	H_DEREF(hf, VFile, vf);

	size = vf->f.size;

	buf = FILE_BUF_ALLOC;
	ssize_t nread = vfs_io(hf, size, &buf, cb, cb_ctx);
	// IO failed
	if(nread < 0)
	{
		file_buf_free(buf);
		(void)vfs_close(hf);
		buf = 0, size = 0;	// make sure they are zeroed
		return (LibError)nread;
	}

	debug_assert(nread == (ssize_t)size);

	(void)file_cache_add(buf, size, atom_fn, file_flags);
	stats_cache(CR_MISS, size, atom_fn);

	(void)vfs_close(hf);
	return INFO::OK;
}


// caveat: pads file to next max(4kb, sector_size) boundary
// (due to limitation of Win32 FILE_FLAG_NO_BUFFERING I/O).
// if that's a problem, specify FILE_NO_AIO when opening.
ssize_t vfs_store(const char* V_fn, const void* p, const size_t size, uint flags /* default 0 */)
{
	Handle hf = vfs_open(V_fn, flags|FILE_WRITE);
	H_DEREF(hf, VFile, vf);
	FileIOBuf buf = (FileIOBuf)p;
	const ssize_t ret = vfs_io(hf, size, &buf);
	WARN_ERR(vfs_close(hf));
	return ret;
}


///////////////////////////////////////////////////////////////////////////////
//
// asynchronous I/O
//
///////////////////////////////////////////////////////////////////////////////

// we don't support forcibly closing files => don't need to keep track of
// all IOs pending for each file. too much work, little benefit.
struct VIo
{
	Handle hf;
	size_t size;
	void* buf;

	FileIo io;
};

H_TYPE_DEFINE(VIo);

static void VIo_init(VIo* vio, va_list args)
{
	vio->hf   = va_arg(args, Handle);
	vio->size = va_arg(args, size_t);
	vio->buf  = va_arg(args, void*);
}

static void VIo_dtor(VIo* vio)
{
	// note: checking if reload() succeeded is unnecessary because
	// xfile_io_discard safely handles 0-initialized data.
	WARN_ERR(xfile_io_discard(&vio->io));
}

// we don't support transparent read resume after file invalidation.
// if the file has changed, we'd risk returning inconsistent data.
// doesn't look possible without controlling the AIO implementation:
// when we cancel, we can't prevent the app from calling
// aio_result, which would terminate the read.
static LibError VIo_reload(VIo* vio, const char* UNUSED(fn), Handle UNUSED(h))
{
	size_t size = vio->size;
	void* buf   = vio->buf;

	H_DEREF(vio->hf, VFile, vf);
	off_t ofs = vf->ofs;
	vf->ofs += (off_t)size;

	return xfile_io_issue(&vf->f, ofs, size, buf, &vio->io);
}

static LibError VIo_validate(const VIo* vio)
{
	if(vio->hf < 0)
		WARN_RETURN(ERR::_21);
	// <size> doesn't have any invariant we can check.
	if(debug_is_pointer_bogus(vio->buf))
		WARN_RETURN(ERR::_22);
	return xfile_io_validate(&vio->io);
}

static LibError VIo_to_string(const VIo* vio, char* buf)
{
	snprintf(buf, H_STRING_LEN, "buf=%p size=%d", vio->buf, vio->size);
	return INFO::OK;
}



// begin transferring <size> bytes, starting at <ofs>. get result
// with vfs_io_wait; when no longer needed, free via vfs_io_discard.
Handle vfs_io_issue(Handle hf, size_t size, void* buf)
{
	const char* fn = 0;
	uint flags = 0;
	return h_alloc(H_VIo, fn, flags, hf, size, buf);
}


// finished with transfer <hio> - free its buffer (returned by vfs_io_wait)
LibError vfs_io_discard(Handle& hio)
{
	return h_free(hio, H_VIo);
}


// indicates if the VIo referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
int vfs_io_has_completed(Handle hio)
{
	H_DEREF(hio, VIo, vio);
	return xfile_io_has_completed(&vio->io);
}


// wait until the transfer <hio> completes, and return its buffer.
// output parameters are zeroed on error.
LibError vfs_io_wait(Handle hio, void*& p, size_t& size)
{
	H_DEREF(hio, VIo, vio);
	return xfile_io_wait(&vio->io, p, size);
}


///////////////////////////////////////////////////////////////////////////////
//
// memory mapping
//
///////////////////////////////////////////////////////////////////////////////


// map the entire (uncompressed!) file <hf> into memory. if currently
// already mapped, return the previous mapping (reference-counted).
// output parameters are zeroed on failure.
//
// the mapping will be removed (if still open) when its file is closed.
// however, map/unmap calls should still be paired so that the mapping
// may be removed when no longer needed.
LibError vfs_map(const Handle hf, const uint UNUSED(flags), void*& p, size_t& size)
{
	p = 0;
	size = 0;
	// need to zero these here in case H_DEREF fails

	H_DEREF(hf, VFile, vf);
	return xfile_map(&vf->f, p, size);
}


// decrement the reference count for the mapping belonging to file <f>.
// fail if there are no references; remove the mapping if the count reaches 0.
//
// the mapping will be removed (if still open) when its file is closed.
// however, map/unmap calls should still be paired so that the mapping
// may be removed when no longer needed.
LibError vfs_unmap(const Handle hf)
{
	H_DEREF(hf, VFile, vf);
	return xfile_unmap(&vf->f);
}


//-----------------------------------------------------------------------------
// hotloading
//-----------------------------------------------------------------------------

// called by vfs_reload and vfs_reload_changed_files (which will already
// have rebuilt the VFS - doing so more than once a frame is unnecessary).
static LibError reload_without_rebuild(const char* fn)
{
	// invalidate this file's cached blocks to make sure its contents are
	// loaded anew.
	RETURN_ERR(file_cache_invalidate(fn));

	RETURN_ERR(h_reload(fn));

	return INFO::OK;
}


// called via console command.
LibError vfs_reload(const char* fn)
{
	// if <fn> currently maps to an archive, the VFS must switch
	// over to using the loose file (that was presumably changed).
	RETURN_ERR(mount_rebuild());

	return reload_without_rebuild(fn);
}

// array of reloads requested this frame (see 'do we really need to
// reload' below). go through gyrations to avoid heap allocs.
const size_t MAX_RELOADS_PER_FRAME = 12;
typedef char Path[PATH_MAX];
typedef Path PathList[MAX_RELOADS_PER_FRAME];

// do we really need to reload? try to avoid the considerable cost of
// rebuilding VFS and scanning all Handles.
static bool can_ignore_reload(const char* V_path, PathList pending_reloads, uint num_pending)
{
	// note: be careful to avoid 'race conditions' depending on the
	// timeframe in which notifications reach us.
	// example: editor deletes a.tga; we are notified; reload is
	// triggered but fails since the file isn't found; further
	// notifications (e.g. renamed a.tmp to a.tga) come within x [ms] and
	// are ignored due to a time limit.
	// therefore, we can only check for multiple reload requests a frame;
	// to that purpose, an array is built and duplicates ignored.
	const char* ext = path_extension(V_path);
	// .. directory change notification; ignore because we get
	//    per-file notifications anyway. (note: assume no extension =>
	//    it's a directory).
	if(ext[0] == '\0')
		return true;
	// .. compiled XML files the engine writes out by the hundreds;
	//    skipping them is a big performance gain.
	if(!stricmp(ext, "xmb"))
		return true;
	// .. temp files, usually created when an editor saves a file
	//    (delete, create temp, rename temp); no need to reload those.
	if(!stricmp(ext, "tmp"))
		return true;
	// .. more than one notification for a file; only reload once.
	//    note: this doesn't suffer from the 'reloaded too early'
	//    problem described above; if there's more than one
	//    request in the array, the file has since been written.
	for(uint i = 0; i < num_pending; i++)
	{
		if(!strcmp(pending_reloads[i], V_path))
			return true;
	}

	return false;
}


// get directory change notifications, and reload all affected files.
// must be called regularly (e.g. once a frame). this is much simpler
// than asynchronous notifications: everything would need to be thread-safe.
LibError vfs_reload_changed_files()
{
	PathList pending_reloads;

	uint num_pending = 0;
	// process only as many notifications as we have room for; the others
	// will be handled next frame. it's not imagineable that they'll pile up.
	while(num_pending < MAX_RELOADS_PER_FRAME)
	{
		// get next notification
		char N_path[PATH_MAX];
		LibError ret = dir_get_changed_file(N_path);
		if(ret == ERR::AGAIN)	// none available; done.
			break;
		RETURN_ERR(ret);

		// convert to VFS path
		char P_path[PATH_MAX];
		RETURN_ERR(file_make_full_portable_path(N_path, P_path));
		char* V_path = pending_reloads[num_pending];
		RETURN_ERR(mount_make_vfs_path(P_path, V_path));

		if(can_ignore_reload(V_path, pending_reloads, num_pending))
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
		RETURN_ERR(mount_rebuild());

	// now actually reload all files in the array we built
	for(uint i = 0; i < num_pending; i++)
		RETURN_ERR(reload_without_rebuild(pending_reloads[i]));

	return INFO::OK;
}


//-----------------------------------------------------------------------------

void vfs_display()
{
	tree_display();
}


static enum VfsInitState
{
	VFS_BEFORE_INIT,
	VFS_INITIALIZED,
	VFS_SHUTDOWN
}
vfs_init_state = VFS_BEFORE_INIT;

// make the VFS tree ready for use. must be called before all other
// functions below, barring explicit mentions to the contrary.
//
// rationale: initialization could be done implicitly by calling this
// from all VFS APIs. we refrain from that and require the user to
// call this because a central point of initialization (file_set_root_dir)
// is necessary anyway and this way is simpler/easier to maintain.
void vfs_init()
{
	debug_assert(vfs_init_state == VFS_BEFORE_INIT || vfs_init_state == VFS_SHUTDOWN);

	stats_vfs_init_start();
	mount_init();
	stats_vfs_init_finish();

	vfs_init_state = VFS_INITIALIZED;
}

void vfs_shutdown()
{
	debug_assert(vfs_init_state == VFS_INITIALIZED);

	trace_shutdown();
	mount_shutdown();

	vfs_init_state = VFS_SHUTDOWN;
}
