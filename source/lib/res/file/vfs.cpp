// virtual file system - transparent access to files in archives;
// allows multiple mount points.
// this file provides a Handle-based interface on top of the vfs_tree
// API and implements I/O for the various MountType file sources.
//
// Copyright (c) 2004 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

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

#include "lib.h"
#include "adts.h"
#include "timer.h"
#include "../res.h"
#include "zip.h"
#include "file.h"
#include "file_cache.h"
#include "file_internal.h"
#include "sysdep/dir_watch.h"
#include "vfs_path.h"
#include "vfs_tree.h"
#include "vfs_mount.h"
#include "vfs_optimizer.h"

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
	TreeDirIterator it;
	uint it_valid : 1;	// <it> will be closed iff == 1

	// safety check
	const char* filter;
	// has filter been assigned? this flag is necessary because there are no
	// "invalid" filter values we can use.
	uint filter_latched : 1;
};

H_TYPE_DEFINE(VDir);

static void VDir_init(VDir* UNUSED(vd), va_list UNUSED(args))
{
}

static void VDir_dtor(VDir* vd)
{
	// note: TreeDirIterator has no way of checking if it's valid;
	// we must therefore only free it if reload() succeeded.
	if(vd->it_valid)
	{
		tree_dir_close(&vd->it);
		vd->it_valid = 0;
	}
}

static LibError VDir_reload(VDir* vd, const char* path, Handle UNUSED(hvd))
{
	// add required trailing slash if not already present to make
	// caller's life easier.
	char V_path_slash[PATH_MAX];
	RETURN_ERR(vfs_path_append(V_path_slash, path, ""));

	RETURN_ERR(tree_dir_open(V_path_slash, &vd->it));
	vd->it_valid = 1;
	return ERR_OK;
}

static LibError VDir_validate(const VDir* vd)
{
	// note: <it> is opaque and cannot be validated.
	if(vd->filter && !isprint(vd->filter[0]))
		return ERR_1;
	return ERR_OK;
}

static LibError VDir_to_string(const VDir* d, char* buf)
{
	const char* filter = d->filter;
	if(!d->filter_latched)
		filter = "?";
	if(!filter)
		filter = "*";
	snprintf(buf, H_STRING_LEN, "(\"%s\")", filter);
	return ERR_OK;
}


// open a directory for reading its entries via vfs_next_dirent.
// <v_dir> need not end in '/'; we add it if not present.
Handle vfs_dir_open(const char* v_dir)
{
	// must disallow handle caching because this object is not
	// copy-equivalent (since the iterator is advanced by each user).
	return h_alloc(H_VDir, v_dir, RES_NO_CACHE);
}


// close the handle to a directory.
LibError vfs_dir_close(Handle& hd)
{
	return h_free(hd, H_VDir);
}


// retrieve the next (order is unspecified) dir entry matching <filter>.
// return 0 on success, ERR_DIR_END if no matching entry was found,
// or a negative error code on failure.
// filter values:
// - 0: anything;
// - "/": any subdirectory;
// - "/|<pattern>": any subdirectory, or as below with <pattern>;
// - <pattern>: any file whose name matches; ? and * wildcards are allowed.
//
// note that the directory entries are only scanned once; after the
// end is reached (-> ERR_DIR_END returned), no further entries can
// be retrieved, even if filter changes (which shouldn't happen - see impl).
//
// rationale: we do not sort directory entries alphabetically here.
// most callers don't need it and the overhead is considerable
// (we'd have to store all entries in a vector). it is left up to
// higher-level code such as VfsUtil.
LibError vfs_dir_next_ent(const Handle hd, DirEnt* ent, const char* filter)
{
	H_DEREF(hd, VDir, vd);

	// warn if scanning the directory twice with different filters
	// (this used to work with dir/file because they were stored separately).
	// it is imaginable that someone will want to change it, but until
	// there's a good reason, leave this check in. note: only comparing
	// pointers isn't 100% certain, but it's safe enough and easy.
	if(!vd->filter_latched)
	{
		vd->filter = filter;
		vd->filter_latched = 1;
	}
	if(vd->filter != filter)
		debug_warn("filter has changed for this directory. are you scanning it twice?");

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

	// loop until ent matches what is requested, or end of directory.
	for(;;)
	{
		RETURN_ERR(tree_dir_next_ent(&vd->it, ent));

		if(DIRENT_IS_DIR(ent))
		{
			if(want_dir)
				break;
		}
		else
		{
			// (note: filter = 0 matches anything)
			if(match_wildcard(ent->name, filter))
				break;
		}
	}

	return ERR_OK;
}



///////////////////////////////////////////////////////////////////////////////
//
// file
//
///////////////////////////////////////////////////////////////////////////////


// return actual path to the specified file:
// "<real_directory>/fn" or "<archive_name>/fn".
LibError vfs_realpath(const char* v_path, char* realpath)
{
	TFile* tf;
	CHECK_ERR(tree_lookup(v_path, &tf));

	const Mount* m = tfile_get_mount(tf);
	const char* V_path = tfile_get_atom_fn(tf);
	return x_realpath(m, V_path, realpath);
}



// does the specified file exist? return false on error.
// useful because a "file not found" warning is not raised, unlike vfs_stat.
bool vfs_exists(const char* V_fn)
{
	TFile* tf;
	return (tree_lookup(V_fn, &tf) == 0);
}


// get file status (mode, size, mtime). output param is zeroed on error.
LibError vfs_stat(const char* v_path, struct stat* s)
{
	memset(s, 0, sizeof(*s));

	TFile* tf;
	CHECK_ERR(tree_lookup(v_path, &tf));

	return tree_stat(tf, s);
}






struct VFile
{
	XFile xf;

	// current file pointer. this is necessary because file.cpp's interface
	// requires passing an offset for every VIo; see file_io_issue.
	off_t ofs;

	uint is_valid : 1;

	// be aware when adding fields that this struct is quite large,
	// and may require increasing the control block size limit.
};

H_TYPE_DEFINE(VFile);

static void VFile_init(VFile* vf, va_list args)
{
	uint flags = va_arg(args, int);
	x_set_flags(&vf->xf, flags);	// can't fail
}

static void VFile_dtor(VFile* vf)
{
	// note: checking if reload() succeeded is unnecessary because
	// x_close and mem_free_h safely handle 0-initialized data.
	WARN_ERR(x_close(&vf->xf));

	if(vf->is_valid)
		FILE_STATS_NOTIFY_CLOSE();
}

static LibError VFile_reload(VFile* vf, const char* V_path, Handle)
{
	const uint flags = x_flags(&vf->xf);

	// we're done if file is already open. need to check this because
	// reload order (e.g. if resource opens a file) is unspecified.
	if(x_is_open(&vf->xf))
		return ERR_OK;

	trace_add(V_path);

	TFile* tf;
	uint lf = (flags & FILE_WRITE)? LF_CREATE_MISSING : 0;
	LibError err = tree_lookup(V_path, &tf, lf);
	if(err < 0)
	{
		// don't CHECK_ERR - this happens often and the dialog is annoying
		debug_printf("lookup failed for %s\n", V_path);
		return err;
	}

	const Mount* m = tfile_get_mount(tf);
if(!m)
return ERR_LOGIC;
	RETURN_ERR(x_open(m, V_path, flags, tf, &vf->xf));

	FileCommon& fc = vf->xf.u.fc;
	FILE_STATS_NOTIFY_OPEN(fc.atom_fn, fc.size);
	vf->is_valid = 1;

	return ERR_OK;
}

static LibError VFile_validate(const VFile* vf)
{
	// <ofs> doesn't have any invariant we can check.
	RETURN_ERR(x_validate(&vf->xf));
	return ERR_OK;
}

static LibError VFile_to_string(const VFile* UNUSED(vf), char* buf)
{
	strcpy(buf, "");	// safe
	return ERR_OK;
}


// return the size of an already opened file, or a negative error code.
ssize_t vfs_size(Handle hf)
{
	H_DEREF(hf, VFile, vf);
	return x_size(&vf->xf);
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
	if(file_flags & FILE_CACHE)
		res_flags = 0;

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

	off_t ofs = vf->ofs;
	vf->ofs += (off_t)size;

	return x_io(&vf->xf, ofs, size, pbuf, cb, cb_ctx);
}


// load the entire file <fn> into memory.
// p and size are filled with address/size of buffer (0 on failure).
// flags influences IO mode and is typically 0.
// when the file contents are no longer needed, call file_buf_free(buf).
LibError vfs_load(const char* V_fn, FileIOBuf& buf, size_t& size, uint flags /* default 0 */)
{
	debug_printf("VFS| load: V_fn=%s\n", V_fn);

	const char* atom_fn = file_make_unique_fn_copy(V_fn, 0);
	buf = file_cache_retrieve(atom_fn, &size);
	if(buf)
		return ERR_OK;

	buf = 0; size = 0;	// initialize in case something below fails

	Handle hf = vfs_open(atom_fn, flags);
	RETURN_ERR(hf);
		// necessary because if we skip this and have H_DEREF report the
		// error, we get "invalid handle" instead of vfs_open's error code.

	H_DEREF(hf, VFile, vf);

	size = x_size(&vf->xf);
	buf = FILE_BUF_ALLOC;
	ssize_t nread = vfs_io(hf, size, &buf);
	// IO failed
	if(nread < 0)
	{
		file_buf_free(buf);
		(void)vfs_close(hf);
		buf = 0, size = 0;	// make sure they are zeroed
		return (LibError)nread;
	}

	debug_assert(nread == (ssize_t)size);
	(void)file_cache_add(buf, size, atom_fn);

	(void)vfs_close(hf);
	return ERR_OK;
}


// caveat: pads file to next max(4kb, sector_size) boundary
// (due to limitation of Win32 FILE_FLAG_NO_BUFFERING I/O).
// if that's a problem, specify FILE_NO_AIO when opening.
ssize_t vfs_store(const char* V_fn, const void* p, const size_t size, uint flags /* default 0 */)
{
	Handle hf = vfs_open(V_fn, flags|FILE_WRITE);
	RETURN_ERR(hf);
		// necessary because if we skip this and have H_DEREF report the
		// error, we get "invalid handle" instead of vfs_open's error code.
		// don't CHECK_ERR because vfs_open already did.
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

	XIo xio;
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
	// x_io_discard safely handles 0-initialized data.
	WARN_ERR(x_io_discard(&vio->xio));
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

	return x_io_issue(&vf->xf, ofs, size, buf, &vio->xio);
}

static LibError VIo_validate(const VIo* vio)
{
	if(vio->hf < 0)
		return ERR_21;
	// <size> doesn't have any invariant we can check.
	if(debug_is_pointer_bogus(vio->buf))
		return ERR_22;
	return x_io_validate(&vio->xio);
}

static LibError VIo_to_string(const VIo* vio, char* buf)
{
	snprintf(buf, H_STRING_LEN, "buf=%p size=%d", vio->buf, vio->size);
	return ERR_OK;
}



// begin transferring <size> bytes, starting at <ofs>. get result
// with vfs_io_wait; when no longer needed, free via vfs_io_discard.
Handle vfs_io_issue(Handle hf, size_t size, void* buf)
{
	const char* fn = 0;
	int flags = 0;
	return h_alloc(H_VIo, fn, flags, hf, size, buf);
}


// finished with transfer <hio> - free its buffer (returned by vfs_io_wait)
LibError vfs_io_discard(Handle& hio)
{
	return h_free(hio, H_VIo);
}


// indicates if the VIo referenced by <xio> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
int vfs_io_has_completed(Handle hio)
{
	H_DEREF(hio, VIo, vio);
	return x_io_has_completed(&vio->xio);
}


// wait until the transfer <hio> completes, and return its buffer.
// output parameters are zeroed on error.
LibError vfs_io_wait(Handle hio, void*& p, size_t& size)
{
	H_DEREF(hio, VIo, vio);
	return x_io_wait(&vio->xio, p, size);
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
	return x_map(&vf->xf, p, size);
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
	return x_unmap(&vf->xf);
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

	return ERR_OK;
}


// called via console command.
LibError vfs_reload(const char* fn)
{
	// if <fn> currently maps to an archive, the VFS must switch
	// over to using the loose file (that was presumably changed).
	CHECK_ERR(mount_rebuild());

	return reload_without_rebuild(fn);
}


// get directory change notifications, and reload all affected files.
// must be called regularly (e.g. once a frame). this is much simpler
// than asynchronous notifications: everything would need to be thread-safe.
LibError vfs_reload_changed_files()
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
		char N_path[PATH_MAX];
		LibError ret = dir_get_changed_file(N_path);
		if(ret == ERR_AGAIN)	// none available; done.
			break;
		CHECK_ERR(ret);

		// convert to VFS path
		char P_path[PATH_MAX];
		CHECK_ERR(file_make_full_portable_path(N_path, P_path));
		char* V_path = pending_reloads[num_pending];
		CHECK_ERR(mount_make_vfs_path(P_path, V_path));

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
		const char* ext = strrchr(V_path, '.');
		// .. directory change notification; ignore because we get
		//    per-file notifications anyway. (note: assume no extension =>
		//    it's a directory). this also protects the strcmp calls below.
		if(!ext)
			continue;
		// .. compiled XML files the engine writes out by the hundreds;
		//    skipping them is a big performance gain.
		if(!stricmp(ext, ".xmb"))
			continue;
		// .. temp files, usually created when an editor saves a file
		//    (delete, create temp, rename temp); no need to reload those.
		if(!stricmp(ext, ".tmp"))
			continue;
		// .. more than one notification for a file; only reload once.
		//    note: this doesn't suffer from the 'reloaded too early'
		//    problem described above; if there's more than one
		//    request in the array, the file has since been written.
		for(uint i = 0; i < num_pending; i++)
			if(!strcmp(pending_reloads[i], V_path))
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
		CHECK_ERR(mount_rebuild());

	// now actually reload all files in the array we built
	for(uint i = 0; i < num_pending; i++)
		CHECK_ERR(reload_without_rebuild(pending_reloads[i]));

	return ERR_OK;
}


//-----------------------------------------------------------------------------

inline void vfs_display()
{
	tree_display();
}


// allow init more complicated than merely calling mount_init by
// splitting this into a separate function.
static void vfs_init_once(void)
{
	tree_init();
	mount_init();
}

// make the VFS tree ready for use. must be called before all other
// functions below, barring explicit mentions to the contrary.
//
// rationale: initialization could be done implicitly by calling this
// from all VFS APIs. we refrain from that and require the user to
// call this because a central point of initialization (file_set_root_dir)
// is necessary anyway and this way is simpler/easier to maintain.
void vfs_init()
{
	static pthread_once_t once = PTHREAD_ONCE_INIT;
	WARN_ERR(pthread_once(&once, vfs_init_once));
}

void vfs_shutdown()
{
	trace_shutdown();
	mount_shutdown();
	tree_shutdown();
}
