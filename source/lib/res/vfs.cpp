// virtual file system - transparent access to files in archives;
// allows multiple mount points
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

#include "lib.h"
#include "res.h"
#include "zip.h"
#include "file.h"
#include "adts.h"
#include "timer.h"
#include "vfs_path.h"
#include "vfs_tree.h"

#include <string.h>

#include <map>
#include <list>
#include <vector>
#include <string>
#include <algorithm>


// currently not thread safe. will have to change that if
// a prefetch thread is to be used.
// not safe to call before main!


// we add/cancel directory watches from the VFS mount code for convenience -
// it iterates through all subdirectories anyway (*) and provides storage for
// a key to identify the watch (obviates separate TDir -> watch mapping).
//
// define this to strip out that code - removes .watch from struct TDir,
// and calls to res_watch_dir / res_cancel_watch.
//
// *: the add_watch code would need to iterate through subdirs and watch
//    each one, because the monitor API (e.g. FAM) may only be able to
//    watch single directories, instead of a whole subdirectory tree.
//#define NO_DIR_WATCH


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









// the VFS stores the location (archive or directory) of each file;
// this allows multiple search paths without having to check each one
// when opening a file (slow).
//
// one TMountPoint is allocated for each archive or directory mounted.
// therefore, files only /point/ to a (possibly shared) TMountPoint.
// if a file's location changes (e.g. after mounting a higher-priority
// directory), the VFS entry will point to the new TMountPoint; the priority
// of both locations is unchanged.
//
// allocate via mnt_create, passing the location. do not free!
// we keep track of all Locs allocated; they are freed at exit,
// and by mnt_free_all (useful when rebuilding the VFS).
// this is much easier and safer than walking the VFS tree and
// freeing every location we find.


// location of a file: either archive or a real directory.
// not many instances => don't worry about efficiency.
struct TMountPoint
{
	Handle archive;
	// not freed in dtor, so that users don't have to avoid
	// TMountPoint temporary objects (that would free the archive)

	const std::string v_mount_point;
	const std::string p_real_path;

	uint pri;

	TMountPoint(Handle _archive, const char* _v_mount_point, const char* _p_real_path, uint _pri)
		: v_mount_point(_v_mount_point), p_real_path(_p_real_path)
	{
		archive = _archive;
		pri = _pri;
	}

	// no copy ctor, since some members are const
private:
	TMountPoint& operator=(const TMountPoint&);
};


// container must not invalidate iterators after insertion!
// (we keep and pass around pointers to Mount.archives elements)
// see below.
typedef std::list<TMountPoint> TMountPoints;
typedef TMountPoints::iterator TMountPointIt;


///////////////////////////////////////////////////////////////////////////////
//
// populate the directory being mounted with files from real subdirectories
// and archives.
//
///////////////////////////////////////////////////////////////////////////////

// attempt to add <fn> to <dir>, storing its status <s> and location <mount_point>.
// overrides previously existing files of the same name if the new one
// is more important, determined via priority and file location.
// called by zip_cb and dirent_cb.
//
// note: if "priority" is the same, replace!
// this makes sure mods/patches etc. actually replace files.
//
// xxx [total time 27ms, with ~2000 files and up-to-date archive]
static int add_file(TDir* dir, const char* fn, const struct stat* s, const TMountPoint* mount_point)
{
	TFile* file = tree_add_file(dir, fn);
	if(!file)
		return ERR_NO_MEM;

	const uint pri        = mount_point->pri;
	const bool in_archive = mount_point->archive > 0;
	const off_t size   = s->st_size;
	const time_t mtime = s->st_mtime;

	// was already added: check if we need to override
	if(file->mount_point)
	{
		// older is higher priority - keep.
		if(file->mount_point->pri > pri)
			return 0;

		// assume they're the same if size and last-modified time match.
		const bool is_same = (file->size == size) &&
			fabs(difftime(file->mtime, mtime)) <= 2.0;
		// (FAT timestamp has 2 second resolution)

		// strategy: always replace unless: both are the same,
		// old is archived (fast), and new is loose (slow).
		if(is_same && file->mount_point->archive > 0 && !in_archive)
			return 0;
	}

	file->mount_point = mount_point;
	file->in_archive  = in_archive;
	file->pri         = pri;
	file->mtime       = mtime;
	file->size        = size;
	return 0;
}


// passed through dirent_cb's zip_enum to zip_cb
struct ZipCBParams
{
	// tree directory into which we are adding the archive's files
	TDir* const dir;

	// archive's location; assigned to all files added from here
	const TMountPoint* const mount_point;

	// storage for directory lookup optimization (see below).
	// held across one zip_enum's zip_cb calls.
	char last_path[VFS_MAX_PATH];
	size_t last_path_len;
	TDir* last_dir;

	ZipCBParams(TDir* dir_, const TMountPoint* loc_)
		: dir(dir_), mount_point(loc_)
	{
		last_path[0] = '\0';
		last_path_len = 0;
		last_dir = 0;
	}

	// no copy ctor, since some members are const
private:
	ZipCBParams& operator=(const ZipCBParams&);
};

// called by dirent_cb's zip_enum for each file in the archive.
// we get the full path, since that's what is stored in Zip archives.
//
// [total time 21ms, with ~2000 file's (includes add_file cost)]
static int zip_cb(const char* path, const struct stat* s, uintptr_t user)
{
	CHECK_PATH(path);

	ZipCBParams* params = (ZipCBParams*)user;
	TDir* dir                      = params->dir;
	const TMountPoint* mount_point = params->mount_point;
	char* last_path                = params->last_path;
	size_t& last_path_len          = params->last_path_len;
	TDir*& last_dir                = params->last_dir;

	// extract file name (needed for add_file)
	const char* fn = path;
	const char* slash = strrchr(path, '/');
	if(slash)
		fn = slash+1;
	// else: there is no path - it's in the archive's root dir.

	// into which directory should the file be inserted?
	// naive approach: tree_lookup_dir the path (slow!)
	// optimization: store the last file's path; if it's the same,
	//   use the directory we looked up last time (much faster!)
	const size_t path_len = fn-path;
	// .. same as last time
	if(last_dir && path_len == last_path_len &&
	   strnicmp(path, last_path, path_len) == 0)
		dir = last_dir;
	// .. last != current: need to do lookup
	else
	{
		path_copy(last_path, path);
		last_path_len = path_len;
		last_path[last_path_len] = '\0';
			// strip filename (tree_lookup_dir requirement)

		CHECK_ERR(tree_lookup_dir(last_path, &dir, LF_CREATE_MISSING|LF_START_DIR));
			// we have to create them if missing, since we can't rely on the
			// archiver placing directories before subdirs or files that
			// reference them (WinZip doesn't always).
			// we also need to start at the mount point (dir).

		last_dir = dir;
	}

	return add_file(dir, fn, s, mount_point);
}


struct DirAndPath
{
	TDir* dir;
	std::string path;
	DirAndPath(TDir* d, const char* p)
		: dir(d), path(p) {}
};

typedef std::deque<const DirAndPath> DirQueue;


// passed through TDir::addR's file_enum to dirent_cb
struct DirentCBParams
{
	// tree dir into which the dirent is to be added
	TDir* const dir;

	// real dir's location; assigned to all files added from this mounting
	const TMountPoint* const mount_point;

	const char* const p_path;

	DirQueue* const dir_queue;

	// if the dirent is an archive, its TMountPoint is added here.
	TMountPoints* const archives;

	DirentCBParams(TDir* d, const TMountPoint* mp, const char* p, DirQueue* dq, TMountPoints* a)
		: dir(d), mount_point(mp), p_path(p), dir_queue(dq), archives(a) {}

	// no copy ctor, since members are const
private:
	DirentCBParams& operator=(const DirentCBParams&);
};

// called by TDir::addR's file_enum for each entry in a real directory.
//
// if called for a real directory, it is added to VFS.
// else if called for a loose file that is a valid archive (*),
//   it is mounted (all of its files are added)
// else the file is added to VFS.
//
// * we only perform this check in the directory being mounted,
// i.e. passed in by tree_add_dir. to determine if a file is an archive,
// we have to open it and read the header, which is slow.
// can't just check extension, because it might not be .zip (e.g. Quake3 .pk3).
//
// [total time 61ms, with ~2000 files (includes zip_cb and add_file cost)]
static int dirent_cb(const char* name, const struct stat* s, uintptr_t user)
{
	const DirentCBParams* params = (const DirentCBParams*)user;
	TDir* dir                      = params->dir;
	const TMountPoint* mount_point = params->mount_point;
	const char* const p_path       = params->p_path;
	DirQueue* dir_queue            = params->dir_queue;
	TMountPoints* archives         = params->archives;
		// = 0 <==> this is the directory being added by tree_add_dir

	// directory: add it.
	if(S_ISDIR(s->st_mode) && dir_queue)
	{
		// don't clutter the tree with versioning system dirs.
		// only applicable for normal dirs; the archive builder
		// takes care of removing these there.
		if(!strcmp(name, "CVS") || !strcmp(name, ".svn"))
			return 0;

		char new_path[VFS_MAX_PATH];
		CHECK_ERR(path_append(new_path, p_path, name));
		TDir* new_dir = tree_add_dir(dir, new_path, mount_point);
		dir_queue->push_back(DirAndPath(new_dir, new_path));
		return 0;
	}

	// caller is requesting we look for archives
	// (only happens in tree_add_dir's dir, not subdirectories. see below)
	if(archives)
	{
		char path[PATH_MAX];
		path_append(path, mount_point->p_real_path.c_str(), name);
			// HACK: only works for tree_add_dir's dir

		// note: don't bother checking extension -
		// archives won't necessarily be called .zip (e.g. Quake III .pk3).
		// we just try and open the file.
		Handle archive = zip_archive_open(path);
		if(archive > 0)
		{
			archives->push_back(TMountPoint(archive, "", "", mount_point->pri));
			const TMountPoint* archive_loc = &archives->back();

			ZipCBParams params(dir, archive_loc);
			return zip_enum(archive, zip_cb, (uintptr_t)&params);
				// bail, so that the archive file isn't added below.
		}
	}

	return add_file(dir, name, s, mount_point);
}


// add the contents of directory <p_path> to this TDir,
// marking the files' locations as <mount_point>.
// if desired, we recursively add the contents of subdirectories as well.
// if <archives> != 0, all archives found in this directory only
//   (not its subdirs! see below) are opened in alphabetical order,
//   their files added, and a TMountPoint appended to <*archives>.
static int populate_dir(TDir* dir_, const char* p_path_, const TMountPoint* mount_point, bool recursive, TMountPoints* archives)
{
	DirQueue dir_queue;
	dir_queue.push_back(DirAndPath(dir_, p_path_));
	DirQueue* const pdir_queue = recursive? &dir_queue : 0;

	do
	{
		TDir* dir          = dir_queue.front().dir;
		const char* p_path = dir_queue.front().path.c_str();

		// add files and subdirs to this dir;
		// also adds the contents of archives if archives != 0.
		const DirentCBParams params(dir, mount_point, p_path, pdir_queue, archives);
		file_enum(p_path, dirent_cb, (uintptr_t)&params);

		// xxx load all archive_loc archives here instead

		archives = 0;
			// prevent searching for archives in subdirectories (slow!). this
			// is currently required by the dirent_cb implementation anyway.

		dir_queue.pop_front();
			// pop at end of loop, because we hold a c_str() reference.
	}
	while(!dir_queue.empty());

	return 0;
}




///////////////////////////////////////////////////////////////////////////////
//
// mount directories into the VFS
//
///////////////////////////////////////////////////////////////////////////////


struct Mount
{
	// note: we basically duplicate the mount information in mount_point.
	// it's no big deal - there won't be many mountings.
	//
	// reason is, we need this info in Mount when remounting,
	// but also in TMountPoint when getting real file path.
	// accessing everything via TDir's TMountPoint is ugly.

	// mounting into this VFS directory;
	// must end in '/' (unless if root dir, i.e. "")
	const std::string v_mount_point;

	// real directory being mounted
	const std::string p_real_path;

	// see enum VfsMountFlags
	int flags;

	uint pri;

	// storage for all TMountPoints ensuing from this mounting.
	// it's safe to store pointers to them: the Mount and Locs containers
	// are std::lists, and all pointers are reset after unmounting something.
	TMountPoint mount_point;
		// referenced by TDir::mount_point (used when creating files for writing)
	TMountPoints archives;
		// contains one TMountPoint for every archive in this directory that
		// was mounted - in alphabetical order!
		//
		// multiple archives per dir support is required for patches.


	Mount(const char* v_mount_point_, const char* p_real_path_, int flags_, uint pri_)
		: v_mount_point(v_mount_point_), p_real_path(p_real_path_),
		  mount_point(0, v_mount_point_, p_real_path_, pri_), archives()
	{
		flags = flags_;
		pri = pri_;
	}

	// no copy ctor, since some members are const
private:
	Mount& operator=(const Mount&);
};

typedef std::list<Mount> Mounts;
typedef Mounts::iterator MountIt;
static Mounts mounts;


// actually mount the specified entry. split out of vfs_mount,
// because when invalidating (reloading) the VFS, we need to
// be able to mount without changing the mount list.
static int remount(Mount& m)
{
	const char* v_mount_point = m.v_mount_point.c_str();
	const char* p_real_path   = m.p_real_path.c_str();
	const int flags           = m.flags;
	const uint pri            = m.pri;
	TMountPoint* mount_point             = &m.mount_point;
	TMountPoints& archives       = m.archives;

	// callers have a tendency to forget required trailing '/';
	// complain if it's not there, unless path = "" (root dir).
#ifndef NDEBUG
	const size_t len = strlen(v_mount_point);
	if(len && v_mount_point[len-1] != '/')
		debug_warn("remount: path doesn't end in '/'");
#endif

	TDir* dir;
	CHECK_ERR(tree_lookup_dir(v_mount_point, &dir, LF_CREATE_MISSING));

	const bool recursive = !!(flags & VFS_MOUNT_RECURSIVE);
	TMountPoints* parchive_locs = (flags & VFS_MOUNT_ARCHIVES)? &archives : 0;

	// add all loose files and subdirectories (recursive).
	// also mounts all archives in p_real_path and adds to archives.
	return populate_dir(dir, p_real_path, mount_point, recursive, parchive_locs);
}


// don't do this in dtor, to allow use of temporary Mount objects.
static int unmount(Mount& m)
{
	for(TMountPointIt it = m.archives.begin(); it != m.archives.end(); ++it)
		zip_archive_close(it->archive);
	m.archives.clear();
	return 0;
}


// trivial, but used by vfs_shutdown and vfs_rebuild
static inline void unmount_all(void)
	{ std::for_each(mounts.begin(), mounts.end(), unmount); }

static inline void remount_all()
	{ std::for_each(mounts.begin(), mounts.end(), remount); }



// mount <p_real_path> into the VFS at <vfs_mount_point>,
//   which is created if it does not yet exist.
// files in that directory override the previous VFS contents if
//   <pri>(ority) is not lower.
// all archives in <p_real_path> are also mounted, in alphabetical order.
//
// flags determines extra actions to perform; see VfsMountFlags.
//
// p_real_path = "." or "./" isn't allowed - see implementation for rationale.
int vfs_mount(const char* v_mount_point, const char* p_real_path, int flags, uint pri)
{
	// make sure it's not already mounted, i.e. in mounts.
	// also prevents mounting a parent directory of a previously mounted
	// directory, or vice versa. example: mount $install/data and then
	// $install/data/mods/official - mods/official would also be accessible
	// from the first mount point - bad.
	// no matter if it's an archive - still shouldn't be a "subpath".
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		if(file_is_subpath(p_real_path, it->p_real_path.c_str()))
		{
			debug_warn("vfs_mount: already mounted");
			return -1;
		}

	// disallow "." because "./" isn't supported on Windows.
	// it would also create a loophole for the parent dir check above.
	// "./" and "/." are caught by CHECK_PATH.
	if(!strcmp(p_real_path, "."))
	{
		debug_warn("vfs_mount: mounting . not allowed");
		return -1;
	}

	CHECK_PATH(v_mount_point);

	const Mount& new_mount = Mount(v_mount_point, p_real_path, flags, pri);
	mounts.push_back(new_mount);

	// actually mount the entry
	Mount& m = mounts.back();
	return remount(m);
}


// rebuild the VFS, i.e. re-mount everything. open files are not affected.
// necessary after loose files or directories change, so that the VFS
// "notices" the changes and updates file locations. res calls this after
// dir_watch reports changes; can also be called from the console after a
// rebuild command. there is no provision for updating single VFS dirs -
// it's not worth the trouble.
int vfs_rebuild()
{
	unmount_all();
	tree_clear();
	tree_init();
	remount_all();
	return 0;
}


// unmount a previously mounted item, and rebuild the VFS afterwards.
int vfs_unmount(const char* p_real_path)
{
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		// found the corresponding entry
		if(it->p_real_path == p_real_path)
		{
			Mount& m = *it;
			unmount(m);

			mounts.erase(it);
			return vfs_rebuild();
		}

	return ERR_PATH_NOT_FOUND;
}


// if <path> or its ancestors are mounted,
// return a VFS path that accesses it.
// used when receiving paths from external code.
int vfs_make_vfs_path(const char* path, char* vfs_path)
{
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
	{
		const char* remove = it->p_real_path.c_str();
		const char* replace = it->v_mount_point.c_str();

		if(path_replace(vfs_path, path, remove, replace) == 0)
			return 0;
	}

	return -1;
}


// given <vfs_path> and the file's location,
// return the actual filename.
// used by vfs_realpath and VFile_reopen.
static int make_file_path(char* path, const char* vfs_path, const TMountPoint* mount_point)
{
	assert(mount_point->archive == 0);

	const char* remove = mount_point->v_mount_point.c_str();
	const char* replace = mount_point->p_real_path.c_str();
	return path_replace(path, vfs_path, remove, replace);
}


///////////////////////////////////////////////////////////////////////////////
//
// directory
//
///////////////////////////////////////////////////////////////////////////////


struct VDir
{
	// we need to cache the complete contents of the directory:
	// if we reference the real directory and it changes,
	// the c_str pointers may become invalid, and some files
	// may be returned out of order / not at all.
	// we copy the directory's subdirectory and file containers.
	void* latch;
};

H_TYPE_DEFINE(VDir);

static void VDir_init(VDir* vd, va_list args)
{
	UNUSED(vd);
	UNUSED(args);
}

static void VDir_dtor(VDir* vd)
{
	tree_close_dir(vd->latch);
}

static int VDir_reload(VDir* vd, const char* path, Handle)
{
	if(vd->latch)
	{
		debug_warn("VDir_reload called when already loaded - why?");
		return 0;
	}

	// add required trailing slash if not already present,
	// to make caller's life easier.
	char path_slash[PATH_MAX];
	CHECK_ERR(path_append(path_slash, path, ""));

	CHECK_ERR(tree_open_dir(path_slash, &vd->latch));
	return 0;
}


// open a directory for reading its entries via vfs_next_dirent.
// <v_dir> need not end in '/'; we add it if not present.
// directory contents are cached here; subsequent changes to the dir
// are not returned by this handle. rationale: see VDir definition.
Handle vfs_open_dir(const char* v_dir)
{
	return h_alloc(H_VDir, v_dir, RES_NO_CACHE);
		// must not cache, since the position in file array
		// is advanced => not copy-equivalent.
}


// close the handle to a directory.
int vfs_close_dir(Handle& hd)
{
	return h_free(hd, H_VDir);
}

// make sure we can assign directly from TDirent to vfsDirEnt
// (more efficient)
cassert(offsetof(vfsDirEnt, name)  == offsetof(TDirent, name));
cassert(offsetof(vfsDirEnt, size)  == offsetof(TDirent, size));
cassert(offsetof(vfsDirEnt, mtime) == offsetof(TDirent, mtime));

// retrieve the next dir entry (in alphabetical order) matching <filter>.
// return 0 on success, ERR_DIR_END if no matching entry was found,
// or a negative error code on failure.
// filter values:
// - 0: any file;
// - "/": any subdirectory
// - anything else: pattern for name (may include '?' and '*' wildcards)
//
// xxx rationale: the filename is currently stored internally as
// std::string (=> less manual memory allocation). we don't want to
// return a reference, because that would break C compatibility.
// we're trying to avoid fixed-size buffers, so that is out as well.
// finally, allocating a copy is not so good because it has to be
// freed by the user (won't happen). returning a volatile pointer
// to the string itself via c_str is the only remaining option.
int vfs_next_dirent(const Handle hd, vfsDirEnt* ent, const char* filter)
{
	H_DEREF(hd, VDir, vd);
	return tree_next_dirent(vd->latch, filter, (TDirent*)ent);
}


// return actual path to the specified file:
// "<real_directory>/fn" or "<archive_name>/fn".
int vfs_realpath(const char* v_path, char* realpath)
{
	TFile* file;
	char v_exact_path[VFS_MAX_PATH];
	CHECK_ERR(tree_lookup(v_path, &file, 0, v_exact_path));

	if(file->in_archive)
	{
		const char* archive_fn = h_filename(file->mount_point->archive);
		if(!archive_fn)
			return -1;
		CHECK_ERR(path_append(realpath, archive_fn, v_exact_path));
	}
	// file is in normal directory
	else
		CHECK_ERR(make_file_path(realpath, v_exact_path, file->mount_point));

	return 0;
}


// does the specified file exist? return false on error.
// useful because a "file not found" warning is not raised, unlike vfs_stat.
bool vfs_exists(const char* v_fn)
{
	TFile* file;
	return (tree_lookup(v_fn, &file) == 0);
}


// get file status (mode, size, mtime). output param is zeroed on error.
int vfs_stat(const char* v_path, struct stat* s)
{
	TFile* file;
	CHECK_ERR(tree_lookup(v_path, &file));

	// all stat members currently supported are stored in TFile,
	// so we can return that without having to call file_stat().
	s->st_mode  = S_IFREG;
	s->st_size  = file->size;
	s->st_mtime = file->mtime;

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// file
//
///////////////////////////////////////////////////////////////////////////////


//
// logging
//

static int file_listing_enabled;
	// tristate; -1 = already shut down

static FILE* file_list;


static void file_listing_shutdown()
{
	if(file_listing_enabled == 1)
	{
		fclose(file_list);
		file_listing_enabled = -1;
	}
}


static void file_listing_add(const char* v_fn)
{
	// we've already shut down - complain.
	if(file_listing_enabled == -1)
	{
		debug_warn("file_listing_add: called after file_listing_shutdown atexit");
		return;
	}

	// listing disabled.
	if(file_listing_enabled == 0)
		return;

	if(!file_list)
	{
		file_list = fopen("../logs/filelist.txt", "w");
		if(!file_list)
			return;
	}

	fputs(v_fn, file_list);
	fputc('\n', file_list);
}


void vfs_enable_file_listing(bool want_enabled)
{
	// already shut down - don't allow enabling
	if(file_listing_enabled == -1 && want_enabled)
	{
		debug_warn("vfs_enable_file_listing: enabling after shutdown");
		return;
	}

	file_listing_enabled = (int)want_enabled;
}


///////////////////////////////////////////////////////////////////////////////



enum
{
	// internal file state flags
	// make sure these don't conflict with vfs.h flags
	VF_OPEN = 0x100,
	VF_ZIP  = 0x200

};

struct VFile
{
	// cached contents of file from vfs_load
	// (can't just use pointer - may be freed behind our back)
	Handle hm;

	off_t ofs;

	union
	{
		File f;
		ZFile zf;
	};

	// be aware when adding fields that this struct is quite large,
	// and may require increasing the control block size limit.
	// (especially in PARANOIA builds, which add a member!)
};

H_TYPE_DEFINE(VFile);


// with #define PARANOIA, File and ZFile get an additional member,
// and VFile was exceeding HDATA_USER_SIZE. flags and size (required
// in File as well as VFile) are now moved into the union.
// use the functions below to insulate against change a bit.

static off_t& vf_size(VFile* vf)
{
	assert(offsetof(struct File, size) == offsetof(struct ZFile, ucsize));
	return vf->f.size;
}


static uint& vf_flags(VFile* vf)
{
	assert(offsetof(struct File, flags) == offsetof(struct ZFile, flags));
	return vf->f.flags;
}



static void VFile_init(VFile* vf, va_list args)
{
	uint flags = va_arg(args, int);
	vf_flags(vf) = flags;
}


static void VFile_dtor(VFile* vf)
{
	uint& flags = vf_flags(vf);

	if(flags & VF_OPEN)
	{
		if(flags & VF_ZIP)
			zip_close(&vf->zf);
		else
			file_close(&vf->f);

		flags &= ~(VF_OPEN);
	}

	mem_free_h(vf->hm);
}



static int VFile_reload(VFile* vf, const char* v_path, Handle)
{
	uint& flags = vf_flags(vf);
		// note: no matter if flags are assigned and the function later
		// fails: the Handle will be closed anyway.

	// we're done if file is already open. need to check this because
	// reload order (e.g. if resource opens a file) is unspecified.
	if(flags & VF_OPEN)
		return 0;

	file_listing_add(v_path);

	TFile* file;
	char v_exact_path[VFS_MAX_PATH];
	uint lf = (flags & FILE_WRITE)? LF_CREATE_MISSING : 0;
	int err = tree_lookup(v_path, &file, lf, v_exact_path);
	if(err < 0)
	{
		// don't CHECK_ERR - this happens often and the dialog is annoying
		debug_out("lookup failed for %s\n", v_path);
		return err;
	}

	if(file->in_archive)
	{
		if(flags & FILE_WRITE)
		{
			debug_warn("requesting write access to file in archive");
			return -1;
		}

		CHECK_ERR(zip_open(file->mount_point->archive, v_exact_path, &vf->zf));

		flags |= VF_ZIP;
	}
	// normal file
	else
	{
		char p_path[PATH_MAX];
		CHECK_ERR(make_file_path(p_path, v_exact_path, file->mount_point));
		CHECK_ERR(file_open(p_path, flags, &vf->f));
	}

	// success
	flags |= VF_OPEN;
	return 0;
}


// return the size of an already opened file, or a negative error code.
ssize_t vfs_size(Handle hf)
{
	H_DEREF(hf, VFile, vf);
	return vf_size(vf);
}


// open the file for synchronous or asynchronous IO. write access is
// requested via FILE_WRITE flag, and is not possible for files in archives.
// file_flags: default 0
Handle vfs_open(const char* v_fn, uint file_flags)
{
	// keeping files open doesn't make sense in most cases (because the
	// file is used to load resources, which are cached at a higher level).
	uint res_flags = RES_NO_CACHE;
	if(file_flags & FILE_CACHE)
		res_flags = 0;

	Handle h = h_alloc(H_VFile, v_fn, res_flags, file_flags);
		// pass file flags to init

#ifdef PARANOIA
debug_out("vfs_open fn=%s %llx\n", v_fn, h);
#endif

	return h;
}


// close the handle to a file.
int vfs_close(Handle& h)
{
#ifdef PARANOIA
debug_out("vfs_close %llx\n", h);
#endif

	return h_free(h, H_VFile);
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
ssize_t vfs_io(const Handle hf, const size_t size, void** p, FileIOCB cb, uintptr_t ctx)
{
#ifdef PARANOIA
debug_out("vfs_io size=%d\n", size);
#endif

	H_DEREF(hf, VFile, vf);

	off_t ofs = vf->ofs;
	vf->ofs += (off_t)size;

	void* buf = 0;	// assume temp buffer (p == 0)
	if(p)
		// user-specified buffer
		if(*p)
			buf = *p;
		// we allocate
		else
		{
			buf = mem_alloc(round_up(size, 4096), 4096);
			if(!buf)
				return ERR_NO_MEM;
			*p = buf;
		}

	// (vfs_open makes sure it's not opened for writing if zip)
	if(vf_flags(vf) & VF_ZIP)
		return zip_read(&vf->zf, ofs, size, buf, cb, ctx);

	// normal file:
	// let file_io alloc the buffer if the caller didn't (i.e. p = 0),
	// because it knows about alignment / padding requirements
	return file_io(&vf->f, ofs, size, buf, cb, ctx);
}


#include "timer.h"
static double dt;
static double totaldata;
void dump()
{
	debug_out("TOTAL TIME IN VFS_IO: %f\nthroughput: %f MiB/s\n\n", dt, totaldata/dt/1e6);
}

static ssize_t vfs_timed_io(const Handle hf, const size_t size, void** p, FileIOCB cb = 0, uintptr_t ctx = 0)
{
	ONCE(atexit(dump));

	double t1=get_time();
	totaldata += size;

	ssize_t nread = vfs_io(hf, size, p, cb, ctx);

	double t2=get_time();
	if(t2-t1 < 1.0)
		dt += t2-t1;

	return nread;
}


// load the entire file <fn> into memory; return a handle to the memory
// and the buffer address/size. output parameters are zeroed on failure.
// in addition to the regular file cache, the entire buffer is kept in memory
// if flags & FILE_CACHE.
//
// note: we need the Handle return value for Tex.hm - the data pointer
// must be protected against being accidentally free-d in that case.
Handle vfs_load(const char* v_fn, void*& p, size_t& size, uint flags /* default 0 */)
{
#ifdef PARANOIA
debug_out("vfs_load v_fn=%s\n", v_fn);
#endif

	p = 0; size = 0;	// zeroed in case vfs_open or H_DEREF fails

	Handle hf = vfs_open(v_fn, flags);
	CHECK_ERR(hf);
		// note: if we skip this and have H_DEREF report the error,
		// we get "invalid handle" instead of vfs_open's error code.

	H_DEREF(hf, VFile, vf);

	Handle hm = 0;	// return value - handle to memory or error code
	size = vf_size(vf);

	// already read into mem - return existing mem handle
	// TODO: what if mapped?
	if(vf->hm > 0)
	{
		p = mem_get_ptr(vf->hm, &size);
		if(p)
		{
			assert(vf_size(vf) == (off_t)size && "vfs_load: mismatch between File and Mem size");
			hm = vf->hm;
			goto ret;
		}
		else
			debug_warn("vfs_load: invalid MEM attached to vfile (0 pointer)");
			// happens if someone frees the pointer. not an error!
	}

	// allocate memory. does expose implementation details of File
	// (padding), but it greatly simplifies returning the Handle
	// (if we allow File to alloc, have to make sure the Handle references
	// the actual data address, not that of the padding).
	{
		const size_t BLOCK_SIZE = 64*KiB;
		p = mem_alloc(size, BLOCK_SIZE, 0, &hm);
		if(!p)
			goto ret;
	}

	{
		ssize_t nread = vfs_timed_io(hf, size, &p);
		// failed
		if(nread < 0)
		{
			mem_free_h(hm);
			hm = nread;	// error code
		}
		else
		{
			if(flags & FILE_CACHE)
				vf->hm = hm;
		}
	}
ret:

	vfs_close(hf);
		// if FILE_CACHE, it's kept open

	// if we fail, make sure these are set to 0
	// (they may have been assigned values above)
	if(hm <= 0)
		p = 0, size = 0;

	if (hm == 0)
		debug_out("hm == 0!!\n");

	return hm;
}


// caveat: pads file to next max(4kb, sector_size) boundary
// (due to limitation of Win32 FILE_FLAG_NO_BUFFERING I/O).
// if that's a problem, specify FILE_NO_AIO when opening.
int vfs_store(const char* v_fn, void* p, const size_t size, uint flags /* default 0 */)
{
	Handle hf = vfs_open(v_fn, flags|FILE_WRITE);
	if(hf <= 0)
		return (int)hf;	// error code
	H_DEREF(hf, VFile, vf);
	const int ret = vfs_io(hf, size, &p);
	vfs_close(hf);
	return ret;
}


///////////////////////////////////////////////////////////////////////////////
//
// asynchronous I/O
//
///////////////////////////////////////////////////////////////////////////////


struct IO
{
	union
	{
		FileIO fio;
		ZipIO zio;
	};

	bool is_zip;	// necessary if we have separate File and Zip IO structures
		// default is false, since control block is 0-initialized
};

H_TYPE_DEFINE(IO);


// don't support forcibly closing files => don't need to keep track of
// all IOs pending for each file. too much work, little benefit.


static void IO_init(IO*, va_list)
{
}

static void IO_dtor(IO* io)
{
	if(io->is_zip)
		zip_discard_io(&io->zio);
	else
		file_discard_io(&io->fio);
}


// we don't support transparent read resume after file invalidation.
// if the file has changed, we'd risk returning inconsistent data.
// doesn't look possible without controlling the AIO implementation:
// when we cancel, we can't prevent the app from calling
// aio_result, which would terminate the read.
static int IO_reload(IO* io, const char* fn, Handle h)
{
	UNUSED(io);
	UNUSED(fn);
	UNUSED(h);
	return 0;
}


// begin transferring <size> bytes, starting at <ofs>. get result
// with vfs_wait_io; when no longer needed, free via vfs_discard_io.
Handle vfs_start_io(Handle hf, size_t size, void* buf)
{
	Handle hio = h_alloc(H_IO, 0);
	H_DEREF(hio, IO, io);

	H_DEREF(hf, VFile, vf);
	off_t ofs = vf->ofs;
	vf->ofs += (off_t)size;

	if(vf_flags(vf) & VF_ZIP)
	{
		io->is_zip = true;
		return zip_start_io(&vf->zf, ofs, size, buf, &io->zio);
	}
	return file_start_io(&vf->f, ofs, size, buf, &io->fio);
}


// indicates if the IO referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
int vfs_io_complete(Handle hio)
{
	H_DEREF(hio, IO, io);
	if(io->is_zip)
		return zip_io_complete(&io->zio);
	else
		return file_io_complete(&io->fio);
}


// wait until the transfer <hio> completes, and return its buffer.
// output parameters are zeroed on error.
int vfs_wait_io(Handle hio, void*& p, size_t& size)
{
	H_DEREF(hio, IO, io);
	if(io->is_zip)
		return zip_wait_io(&io->zio, p, size);
	else
		return file_wait_io(&io->fio, p, size);
}


// finished with transfer <hio> - free its buffer (returned by vfs_wait_io)
int vfs_discard_io(Handle& hio)
{
	return h_free(hio, H_IO);
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
int vfs_map(const Handle hf, const uint flags, void*& p, size_t& size)
{
	UNUSED(flags);

	p = 0;
	size = 0;
	// need to zero these here in case H_DEREF fails

	H_DEREF(hf, VFile, vf);
	if(vf_flags(vf) & VF_ZIP)
		return zip_map(&vf->zf, p, size);
	else
		return file_map(&vf->f, p, size);
}


// decrement the reference count for the mapping belonging to file <f>.
// fail if there are no references; remove the mapping if the count reaches 0.
//
// the mapping will be removed (if still open) when its file is closed.
// however, map/unmap calls should still be paired so that the mapping
// may be removed when no longer needed.
int vfs_unmap(const Handle hf)
{
	H_DEREF(hf, VFile, vf);
	if(vf_flags(vf) & VF_ZIP)
		return zip_unmap(&vf->zf);
	else
		return file_unmap(&vf->f);
}


void vfs_init()
{
	tree_init();
}


// write a representation of the VFS tree to stdout.
void vfs_display()
{
	tree_display();
}

void vfs_shutdown()
{
	file_listing_shutdown();
	tree_clear();
	unmount_all();
}
