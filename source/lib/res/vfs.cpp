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
#include "adts.h"

#include <string.h>

#include <map>
#include <list>
#include <vector>
#include <string>
#include <algorithm>


// currently not thread safe. will have to change that if
// a prefetch thread is to be used.
// not safe to call before main!


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
// path
//
///////////////////////////////////////////////////////////////////////////////

// path types:
// fn  : filename only, no path at all.
// f_* : path intended directly for underlying file layer.
//       component separator is '/'; no absolute paths, or ':', '\\' allowed.
// *   : as above, but path within the VFS.
//       "" is root dir; no absolute path allowed.


// path1 and path2 may be empty, filenames, or full paths.
static int path_append(char* dst, const char* path1, const char* path2)
{
	const size_t path1_len = strlen(path1);
	const size_t path2_len = strlen(path2);

	bool need_separator = false;

	size_t total_len = path1_len + path2_len + 1;	// includes '\0'
	if(path1_len > 0 && path1[path1_len-1] != '/')
	{
		total_len++;	// for '/'
		need_separator = true;
	}

	if(total_len+1 > VFS_MAX_PATH)
		return ERR_VFS_PATH_LENGTH;

	char* p = dst;

	strcpy(p, path1);
	p += path1_len;
	if(need_separator)
		*p++ = '/';
	strcpy(p, path2);
	return 0;
}


static int path_validate(const uint line, const char* const path)
{
	size_t path_len = 0;

	const char* msg = 0;	// error occurred <==> != 0
	int err = -1;			// pass error code to caller

	// disallow absolute path for safety, in case of *nix systems.
	if(path[0] == '/')
	{
		msg = "absolute path";
		goto fail;
	}

	// scan each char in path string; count length.
	for(;;)
	{
		const int c = path[path_len++];

		// whole path is too long
		if(path_len >= VFS_MAX_PATH)
		{
			msg = "path too long";
			goto fail;
		}

		// disallow ".." to prevent going above the VFS root dir
		static bool last_was_dot;
		if(c == '.')
		{
			if(last_was_dot)
			{
				msg = "contains \"..\"";
///				goto fail;
			}
			last_was_dot = true;
		}
		else
			last_was_dot = false;

		// disallow OS-specific dir separators
		if(c == '\\' || c == ':')
		{
			msg = "contains OS-specific dir separator (e.g. '\\', ':')";
///			goto fail;
		}

		// end of string, all is well.
		if(c == '\0')
			goto ok;
	}

	// failed somewhere - err is the error code,
	// or -1 if not set specifically above.
fail:
	debug_out("path_validate at line %d failed: %s", err);
	debug_warn("path_validate failed");
	return err;

ok:
	return 0;
}


#define CHECK_PATH(path) CHECK_ERR(path_validate(__LINE__, path))


///////////////////////////////////////////////////////////////////////////////
//
// "file system" (tree structure; stores location of each file)
//
///////////////////////////////////////////////////////////////////////////////


// the VFS stores the location (archive or directory) of each file;
// this allows multiple search paths without having to check each one
// when opening a file (slow).
//
// one FileLoc is allocated for each archive or directory mounted.
// therefore, files only /point/ to a (possibly shared) FileLoc.
// if a file's location changes (e.g. after mounting a higher-priority
// directory), the VFS entry will point to the new FileLoc; the priority
// of both locations is unchanged.
//
// allocate via mnt_create, passing the location. do not free!
// we keep track of all Locs allocated; they are freed at exit,
// and by mnt_free_all (useful when rebuilding the VFS).
// this is much easier and safer than walking the VFS tree and
// freeing every location we find.


// not many instances => don't worry about struct size / alignment.
struct FileLoc
{
	Handle archive;
	std::string dir;
	uint pri;

	FileLoc() {}
	FileLoc(Handle _archive, const char* _dir, uint _pri)
		: archive(_archive), dir(_dir), pri(_pri) {}
};


// rationale for separate file / subdir containers:
// problems:
// - more code for insertion (oh well);
// - makes ordered output of all dirents difficult
//   (but dirs and files are usually displayed separately)
// advantages:
// - simplifies lookup code: it can just check if a path is there,
//   no need to check if the entry is actually a directory
// - storing Dir objects directly in the map means less
//   memory allocations / no need to free them.
//
// add_* aborts if a subdir or file of the same name already exists.

typedef std::map<const std::string, const FileLoc*> Files;
typedef Files::iterator FileIt;
	// notes:
	// - FileLoc is allocated and owned by caller (the mount code)
	// - priority is accessed by following the FileLoc pointer.
	//   keeping a copy in the map would lead to better cache coherency,
	//   but it's a bit more clumsy (map filename to struct {pri, FileLoc*}).
	//   revisit if file lookup open is too slow (unlikely).

struct Dir;
typedef std::pair<const std::string, Dir> SubDir;
typedef std::map<const std::string, Dir> SubDirs;
typedef SubDirs::iterator SubDirIt;

struct Dir
{
	int add_file(const char* name, const FileLoc* loc);
	const FileLoc* find_file(const char* name);

	int add_subdir(const char* name);
	Dir* find_subdir(const char* name);

	void clearR();

	SubDirs subdirs;
	Files files;
};


int Dir::add_subdir(const char* const fn)
{
	if(find_file(fn) || find_subdir(fn))
	{
		debug_warn("dir_add: file or subdirectory of same name already exists");
		return -1;
	}

	subdirs[fn];
		// side effect: maps <fn> to a newly constructed Dir()
		// non-const => cannot be optimized away.
	return 0;
}


Dir* Dir::find_subdir(const char* const fn)
{
	SubDirIt it = subdirs.find(fn);
	if(it == subdirs.end())
		return 0;
	return &it->second;
}


int Dir::add_file(const char* const fn, const FileLoc* const loc)
{
	if(find_subdir(fn))
	{
		debug_warn("dir_add: file of same name already exists");
		return -1;
	}

	// default pointer ctor sets it to 0 =>
	// if fn wasn't already in the container, old_loc is 0.
	typedef const FileLoc* Data;
		// for absolute clarity; the container holds const FileLoc* objects.
		// operator[] returns a reference to that.
		// need this typedef to work around a GCC bug?
	Data& old_loc = files[fn];
	// old loc exists and is higher priority - keep it.
	if(old_loc && old_loc->pri > loc->pri)
		return 1;

	old_loc = loc;
	return 0;
}


const FileLoc* Dir::find_file(const char* const fn)
{
	FileIt it = files.find(fn);
	if(it == files.end())
		return 0;
	return it->second;
}


void Dir::clearR()
{
	SubDirIt it;
	for(it = subdirs.begin(); it != subdirs.end(); ++it)
	{
		Dir& subdir = it->second;
		subdir.clearR();
	}

	subdirs.clear();
	files.clear();
}




static Dir vfs_root;


enum LookupFlags
{
	LF_DEFAULT                   = 0,

	LF_CREATE_MISSING_COMPONENTS = 1,

	LF_LAST                      = 2
};


// starts in VFS root directory (path = "").
// path may specify a file or directory; it need not end in '/'.
static int tree_lookup(const char* path, const FileLoc** const loc = 0, Dir** const dir = 0, LookupFlags flags = LF_DEFAULT)
{
	CHECK_PATH(path);
	assert(loc != 0 || dir != 0);
	assert(flags < LF_LAST);

	const bool create_missing_components = flags & LF_CREATE_MISSING_COMPONENTS;

	// copy into (writeable) buffer so we can 'tokenize' path components
	// by replacing '/' with '\0'. length check done by CHECK_PATH.
	char buf[VFS_MAX_PATH];
	strcpy(buf, path);

	Dir* cur_dir = &vfs_root;
	const char* cur_component = buf;

	// subdirectory traverse logic
	// valid:
	//   "" (root dir),
	//   "*file",
	//   "*dir[/]"
	// invalid:
	//   "/*" (caught by CHECK_PATH),
	//   "*file/*" (subdir switch will fail)
	//
	// a bit tricky: make sure we go through the directory checks for the
	// last component - it may either be a file or directory name.
	// we don't require a trailing '/' for dir names because appending it
	// to a given dir name would be more (unnecessary) work for the caller.

	// successively navigate to the next subdirectory in <path>.
	for(;;)
	{
		// "extract" cur_component string (0-terminate by replacing '/')
		char* slash = strchr(cur_component, '/');
		if(slash)
			*slash = 0;

		// early outs:
		// .. last component and it's a filename
		if(slash == 0 && loc != 0)
			break;
		// .. root dir ("") or trailing '/' in dir name
		if(*cur_component == '\0' && loc == 0)
			break;

		// create <cur_component> subdir (no-op if it already exists)
		if(create_missing_components)
			cur_dir->add_subdir(cur_component);

		// switch to <cur_component>
		cur_dir = cur_dir->find_subdir(cur_component);
		if(!cur_dir)
			return -ENOENT;

		// next component
		if(!slash)	// done, no more components left
			break;
		cur_component = slash+1;
	}

	// caller wants a file (possibly in addition to its dir)
	if(loc)
	{
		*loc = cur_dir->find_file(cur_component);
		// .. but the file doesn't exist
		if(!*loc)
			return -ENOENT;
	}

	// if loc != 0, we know cur_dir is correct, because it contains
	// the desired file (otherwise, the find_file above will have failed).
	// if loc == 0, path is a dir name, and we have successfully traversed
	// all path components (subdirectories) in the loop.
	if(dir)
		*dir = cur_dir;

	return 0;
}


static inline void tree_clear()
{
	vfs_root.clearR();
}




struct FileCBParams
{
	Dir* const dir;
	const FileLoc* const loc;
	FileCBParams(Dir* _dir, const FileLoc* _loc)
		: dir(_dir), loc(_loc) {}
};

// called for each OS dir ent.
// add each file and directory to the VFS dir.
//
// note:
// we don't mount archives here for performance reasons.
// that means archives in subdirectories of mount points aren't added!
// rationale: can't determine if file is an archive via extension -
// they might be called .pk3 or whatnot. for every file in the tree, we'd have
// to try to open it as an archive - not good.
// this restriction also simplifies the code a bit, but if it's a problem,
// just generate a list of archives here and mount them from the caller.
static int add_dirent_cb(const char* const fn, const uint flags, const ssize_t size, const uintptr_t user)
{
	const FileCBParams* const params = (FileCBParams*)user;
	Dir* const cur_dir           = params->dir;
	const FileLoc* const cur_loc = params->loc;

	int err;

	// directory
	if(flags & LOC_DIR)
	{
		// leave out CVS dirs in debug builds. this makes possible
		// user code that relies on a fixed data directory structure.
#ifndef NDEBUG
		if(!strcmp(fn, "CVS"))
			return 0;
#endif
		err = cur_dir->add_subdir(fn);
	}
	// file
	else
		err = cur_dir->add_file(fn, cur_loc);

	if(err < 0)
		return -EEXIST;
	return 0;
}


static int tree_add_dirR(Dir* const dir, const char* const f_path, const FileLoc* const loc)
{
	CHECK_PATH(f_path);

	// add files and subdirs to vdir
	const FileCBParams params(dir, loc);
	file_enum(f_path, add_dirent_cb, (uintptr_t)&params);

	for(SubDirIt it = dir->subdirs.begin(); it != dir->subdirs.end(); ++it)
	{
		Dir* const subdir = &it->second;
		const char* const subdir_name_c = (it->first).c_str();

		char f_subdir_path[VFS_MAX_PATH];
		CHECK_ERR(path_append(f_subdir_path, f_path, subdir_name_c));

		tree_add_dirR(subdir, f_subdir_path, loc);
	}

	return 0;
}


static int tree_add_loc(Dir* const dir, const FileLoc* const loc)
{
	if(loc->archive > 0)
	{
		const FileCBParams params(dir, loc);
		return zip_enum(loc->archive, add_dirent_cb, (uintptr_t)&params);
	}
	else
	{
		const char* f_path_c = loc->dir.c_str();
		return tree_add_dirR(dir, f_path_c, loc);
	}
}


///////////////////////////////////////////////////////////////////////////////
//
// mount archives and directories into the VFS
//
///////////////////////////////////////////////////////////////////////////////


// container must not invalidate iterators after insertion!
// (we keep and pass around pointers to Mount.archive_locs elements)
// see below.
//
// not const, because we h_free a handle in FileLoc
// (it resets the handle to 0)
typedef std::list<FileLoc> Locs;
typedef Locs::iterator LocIt;


struct Mount
{
	// mounting into this VFS directory ("" for root)
	std::string v_path;

	// what is being mounted; either directory,
	// or archive filename (=> is_single_archive = true)
	std::string f_name;

	uint pri;

	// storage for all Locs ensuing from this mounting.
	// the VFS tree only holds pointers to FileLoc, which is why the
	// Locs container must not invalidate its contents after adding,
	// and also why the VFS tree must be rebuilt after unmounting something.
	FileLoc dir_loc;
	Locs archive_locs;
		// if not is_single_archive, contains one FileLoc for every archive
		// in the directory (but not its children - see remount()).
		// otherwise, contains exactly one FileLoc for the single archive.

	// is f_name an archive filename? if not, it's a directory.
	bool is_single_archive;

	Mount() {}
	Mount(const char* _v_path, const char* _f_name, uint _pri)
		: v_path(_v_path), f_name(_f_name), pri(_pri),
		dir_loc(0, _f_name, 0), archive_locs(), is_single_archive(false)
	{
	}
};

typedef std::vector<Mount> Mounts;
typedef Mounts::iterator MountIt;
static Mounts mounts;


// support for mounting multiple archives in a directory
// (useful for mix-in mods and patches).
// all archives are enumerated, added to a Locs list,
// and mounted (in alphabetical order!)

struct ArchiveCBParams
{
	// we need a full path to open the archive, and only receive
	// the filename, so prepend this (the directory being searched).
	const char* f_dir;

	// priority at which the archive is to be mounted.
	// specify here, instead of when actually adding the archive,
	// because Locs are created const.
	uint pri;

	// will add one FileLoc to this container for
	// every archive successfully opened.
	Locs* archive_locs;
};

// called for each directory entry.
// add each successfully opened archive to list.
static int archive_cb(const char* const fn, const uint flags, const ssize_t size, const uintptr_t user)
{
	// not interested in subdirectories
	if(flags & LOC_DIR)
		return 0;

	const ArchiveCBParams* const params = (ArchiveCBParams*)user;
	const char* const f_dir  = params->f_dir;
	const uint pri           = params->pri;
	Locs* const archive_locs = params->archive_locs;

	// get full path (fn is filename only)
	char f_path[VFS_MAX_PATH];
	CHECK_ERR(path_append(f_path, f_dir, fn));

	// don't check filename extension - archives won't necessarily
	// be called .zip (example: Quake III .pk3).
	// just try to open the file.
	const Handle archive = zip_archive_open(f_path);
	if(archive > 0)
		archive_locs->push_back(FileLoc(archive, "", pri));

	// only add archive to list; don't add its files into the VFS yet,
	// to simplify debugging (we see which files are in which archive)

	return 0;
}


// actually mount the specified entry (either Zip archive or dir).
// split out of vfs_mount because we need to mount without changing the
// mount list, when invalidating (reloading) the VFS.
static int remount(Mount& m)
{
	int err;

	const char* const v_path = m.v_path.c_str();
	const char* const f_name = m.f_name.c_str();
	const uint pri           = m.pri;
	const FileLoc& dir_loc   = m.dir_loc;
	Locs& archive_locs       = m.archive_locs;

	Dir* dir;
	CHECK_ERR(tree_lookup(v_path, 0, &dir, LF_CREATE_MISSING_COMPONENTS));

	// check if target is a single Zip archive
	// order doesn't matter; can't have both an archive and dir
	const Handle archive = zip_archive_open(f_name);
	if(archive > 0)
	{
		m.is_single_archive = true;
		archive_locs.push_back(FileLoc(archive, "", pri));
		const FileLoc* loc = &archive_locs.front();
		return tree_add_loc(dir, loc);
	}

	// enumerate all archives
	ArchiveCBParams params = { f_name, pri, &archive_locs };
	file_enum(f_name, archive_cb, (uintptr_t)&params);

	for(LocIt it = archive_locs.begin(); it != archive_locs.end(); ++it)
	{
		const FileLoc* const loc = &*it;
		tree_add_loc(dir, loc);
	}

	err = tree_add_loc(dir, &dir_loc);
	if(err < 0)
		err = err;

return 0;
}


static int unmount(Mount& m)
{
	for(LocIt it = m.archive_locs.begin(); it != m.archive_locs.end(); ++it)
	{
		FileLoc& loc = *it;
		zip_archive_close(loc.archive);
	}

	m.archive_locs.clear();
	return 0;
}


static inline void unmount_all(void)
	{ std::for_each(mounts.begin(), mounts.end(), unmount); }

static inline void remount_all()
	{ std::for_each(mounts.begin(), mounts.end(), remount); }


void vfs_shutdown(void)
{
	tree_clear();
	unmount_all();
}


// mount either a single archive or a directory into the VFS at
// <vfs_mount_point>, which is created if it does not yet exist.
// new files override the previous VFS contents if pri(ority) is higher.
// if <name> is a directory, all archives in that directory (but not
// its subdirs - see add_dirent_cb) are also mounted in alphabetical order.
// name = "." or "./" isn't allowed - see implementation for rationale.
int vfs_mount(const char* const vfs_mount_point, const char* const name, const uint pri)
{
	ONCE(atexit2(vfs_shutdown));

	// make sure it's not already mounted, i.e. in mounts
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		if(it->f_name == name)
		{
			debug_warn("vfs_mount: already mounted");
			return -1;
		}

	CHECK_PATH(name);

	// TODO: disallow mounting parent directory of a previous mounting

	// disallow . because "./" isn't supported on Windows.
	// the more important reason is that mount points must not overlap
	// (i.e. mount $install/data and then $install/data/mods/official -
	// mods/official would also be accessible from the first mount point).
	if(!strcmp(name, ".") || !strcmp(name, "./"))
	{
		debug_warn("vfs_mount: mounting . not allowed");
		return -1;
	}

	mounts.push_back(Mount(vfs_mount_point, name, pri));

	// actually mount the entry
	Mount& m = mounts.back();
	return remount(m);
}


// rebuild the VFS, i.e. re-mount everything. open files are not affected.
// necessary after loose files or directories change, so that the VFS
// "notices" the changes and updates file locations. res calls this after
// FAM reports changes; can also be called from the console after a
// rebuild command. there is no provision for updating single VFS dirs -
// it's not worth the trouble.
int vfs_rebuild()
{
	tree_clear();

	unmount_all();
	remount_all();
	return 0;
}


// unmount a previously mounted item, and rebuild the VFS afterwards.
int vfs_unmount(const char* name)
{
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		// found the corresponding entry
		if(it->f_name == name)
		{
			Mount& m = *it;
			unmount(m);

			mounts.erase(it);
			return vfs_rebuild();
		}

	return ERR_PATH_NOT_FOUND;
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
	SubDirs* subdirs;
	SubDirIt subdir_it;
	Files* files;
	FileIt file_it;
};

H_TYPE_DEFINE(VDir);

static void VDir_init(VDir* vd, va_list args)
{
}

static void VDir_dtor(VDir* vd)
{
	delete vd->subdirs;
	delete vd->files;
}

static int VDir_reload(VDir* vd, const char* path)
{
	// check if actually reloaded, and why it happened?

	Dir* dir;
	CHECK_ERR(tree_lookup(path, 0, &dir));

	// rationale for copy: see VDir definition
	vd->subdirs = new SubDirs(dir->subdirs);
	vd->subdir_it = vd->subdirs->begin();
	vd->files = new Files(dir->files);
	vd->file_it = vd->files->begin();
	return 0;
}


// open a directory for reading its entries via vfs_next_dirent.
// directory contents are cached here; subsequent changes to the dir
// are not returned by this handle. rationale: see VDir definition.
Handle vfs_open_dir(const char* const dir)
{
	return h_alloc(H_VDir, dir, 0);
}


// close the handle to a directory.
// all vfsDirEnt.name strings are now invalid.
int vfs_close_dir(Handle& hd)
{
	return h_free(hd, H_VDir);
}


// get the next directory entry (in alphabetical order) that matches filter.
// return 0 on success. filter values:
// - 0: any file;
// - ".": any file without extension (filename doesn't contain '.');
// - ".ext": any file with extension ".ext" (which must not contain '.');
// - "/": any subdirectory
int vfs_next_dirent(const Handle hd, vfsDirEnt* ent, const char* const filter)
{
	H_DEREF(hd, VDir, vd);

	// interpret filter (paranoid)
	bool filter_dir = false;
	bool filter_no_ext = false;
	if(filter)
	{
		if(filter[0] == '/')
		{
			if(filter[1] != '\0')
				goto invalid_filter;
			filter_dir = true;
		}
		else if(filter[0] == '.')
		{
			if(strchr(filter+1, '.'))
				goto invalid_filter;
			filter_no_ext = filter[1] == '\0';
		}
		else
		{
		invalid_filter:
			debug_warn("vfs_next_dirent: invalid filter");
			return -1;
		}
	}

	// rationale: the filename is currently stored internally as
	// std::string (=> less manual memory allocation). we don't want to
	// return a reference, because that would break C compatibility.
	// we're trying to avoid fixed-size buffers, so that is out as well.
	// finally, allocating a copy is not so good because it has to be
	// freed by the user (won't happen). returning a volatile pointer
	// to the string itself via c_str is the only remaining option.
	const char* fn;

	// caller wants a subdirectory; return the next one.
	if(filter_dir)
	{
		if(vd->subdir_it == vd->subdirs->end())
			return -1;
		fn = vd->subdir_it->first.c_str();
		++vd->subdir_it;
		goto have_match;
	}

	// caller wants a file; loop until one matches or end of list.
	for(;;)
	{
		if(vd->file_it == vd->files->end())
			return -1;
		fn = vd->file_it->first.c_str();
		++vd->file_it;

		char* const ext = strrchr(fn, '.');
		// file has an extension
		if(ext)
		{
			// not filtering, or filter matches extension: match
			if(!filter || strcmp(ext, filter) == 0)
				goto have_match;
		}
		// file has no extension
		else
		{
			// filter accepts files without an extension: match
			if(filter_no_ext)
				goto have_match;
		}
	}

have_match:
	ent->name = fn;
	return 0;
}


// return actual path to the specified file:
// "<real_directory>/fn" or "<archive_name>/fn".
int vfs_realpath(const char* fn, char* full_path)
{
	const FileLoc* loc;
	CHECK_ERR(tree_lookup(fn, &loc));

	const char* dir;

	// file is in normal directory
	if(loc->archive <= 0)
		dir = loc->dir.c_str();
	// file is in archive
	{
		// "dir" is the archive filename
		dir = h_filename(loc->archive);
		if(!dir)
			return -1;
	}

	CHECK_ERR(path_append(full_path, dir, fn));
	return 0;
}


// return information about the specified file as in stat(2),
// most notably size. stat buffer is undefined on error.
int vfs_stat(const char* fn, struct stat* s)
{
	const FileLoc* loc;
	CHECK_ERR(tree_lookup(fn, &loc));

	if(loc->archive > 0)
		return zip_stat(loc->archive, fn, s);
	else
	{
		// similar to realpath, but don't bother splitting it out.
		char path[VFS_MAX_PATH];
		path_append(path, loc->dir.c_str(), fn);
		return file_stat(path, s);
	}
}


///////////////////////////////////////////////////////////////////////////////
//
// file
//
///////////////////////////////////////////////////////////////////////////////


enum
{
	// internal file state flags
	// make sure these don't conflict with vfs.h flags
	VF_OPEN = 0x100,
	VF_ZIP  = 0x200,

};

struct VFile
{
	// cached contents of file from vfs_load
	// (can't just use pointer - may be freed behind our back)
	Handle hm;

	union
	{
		File f;
		ZFile zf;
	};

	// be aware when adding fields that we're already pushing the size limit
	// (especially in PARANOIA builds, which add a member!)
};

H_TYPE_DEFINE(VFile)


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



static int VFile_reload(VFile* vf, const char* path)
{
	uint& flags = vf_flags(vf);

	// we're done if file is already open. need to check this because reload order
	// (e.g. if resource opens a file) is unspecified.
	if(flags & VF_OPEN)
		return 0;

	int err = -1;


	const FileLoc* loc;
	CHECK_ERR(tree_lookup(path, &loc));

	if(loc->archive <= 0)
	{
		char f_path[PATH_MAX];
		const char* dir = loc->dir.c_str();
		CHECK_ERR(path_append(f_path, dir, path));
		CHECK_ERR(file_open(f_path, vf_flags(vf), &vf->f));
	}
	else
	{
		if(flags & VFS_WRITE)
		{
			debug_warn("requesting write access to file in archive");
			return -1;
		}

		CHECK_ERR(zip_open(loc->archive, path, &vf->zf));

		flags |= VF_ZIP;
	}


	// success
	flags |= VF_OPEN;
	return 0;
}


// open the file for synchronous or asynchronous IO. write access is
// requested via VFS_WRITE flag, and is not possible for files in archives.
Handle vfs_open(const char* fn, uint flags /* = 0 */)
{
	Handle h = h_alloc(H_VFile, fn, 0, flags);
		// pass file flags to init

#ifdef PARANOIA
debug_out("vfs_open fn=%s %I64x\n", fn, h);
#endif

	return h;
}


// close the handle to a file.
inline int vfs_close(Handle& h)
{
#ifdef PARANOIA
debug_out("vfs_close %I64x\n", h);
#endif

	return h_free(h, H_VFile);
}


// try to transfer <size> bytes, starting at <ofs>, to/from the given file.
// (read or write access was chosen at file-open time).
// return bytes of actual data transferred, or a negative error code.
// TODO: buffer types
ssize_t vfs_io(const Handle hf, const off_t ofs, const size_t size, void*& p)
{
#ifdef PARANOIA
debug_out("vfs_io ofs=%d size=%d\n", ofs, size);
#endif

	H_DEREF(hf, VFile, vf);

	// (vfs_open makes sure it's not opened for writing if zip)
	if(vf_flags(vf) & VF_ZIP)
		return zip_read(&vf->zf, ofs, size, p);

	// normal file:
	// let file_io alloc the buffer if the caller didn't (i.e. p = 0),
	// because it knows about alignment / padding requirements
	return file_io(&vf->f, ofs, size, &p);
}


// load the entire file <fn> into memory; return a handle to the memory
// and the buffer address/size. output parameters are zeroed on failure.
Handle vfs_load(const char* const fn, void*& p, size_t& size)
{
#ifdef PARANOIA
debug_out("vfs_load fn=%s\n", fn);
#endif

	p = 0;		// vfs_io needs initial 0 value
	size = 0;	// in case open or deref fails

	Handle hf = vfs_open(fn);
	if(hf <= 0)
		return hf;	// error code
	H_DEREF(hf, VFile, vf);

	Handle hm = 0;
	size = vf_size(vf);

	// already read into mem - return existing mem handle
	// TODO: what if mapped?
	if(vf->hm > 0)
	{
		p = mem_get_ptr(vf->hm, &size);
		if(p)
		{
			assert(vf_size(vf) == size && "vfs_load: mismatch between File and Mem size");
			hm = vf->hm;
			goto skip_read;
		}
		else
			debug_warn("vfs_load: invalid MEM attached to vfile (0 pointer)");
			// happens if someone frees the pointer. not an error!
	}

	{	// VC6 goto fix
	ssize_t nread = vfs_io(hf, 0, size, p);
	if(nread > 0)
		hm = mem_assign(p, size);
	}

skip_read:

	vfs_close(hf);

	// if we fail, make sure these are set to 0
	// (they may have been assigned values above)
	if(hm <= 0)
		p = 0, size = 0;

	return hm;
}


int vfs_store(const char* const fn, void* p, const size_t size)
{
	Handle hf = vfs_open(fn, VFS_WRITE);
	if(hf <= 0)
		return (int)hf;	// error code
	H_DEREF(hf, VFile, vf);
	const int ret = vfs_io(hf, 0, size, p);
	vfs_close(hf);
	return ret;
}


///////////////////////////////////////////////////////////////////////////////
//
// memory mapping
//
///////////////////////////////////////////////////////////////////////////////


// map the entire file <hf> into memory. if already currently mapped,
// return the previous mapping (reference-counted).
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


///////////////////////////////////////////////////////////////////////////////
//
// asynchronous I/O
//
///////////////////////////////////////////////////////////////////////////////


// begin transferring <size> bytes, starting at <ofs>. get result
// with vfs_wait_read; when no longer needed, free via vfs_discard_io.
Handle vfs_start_io(Handle hf, off_t ofs, size_t size, void* buf)
{
	H_DEREF(hf, VFile, vf);
	if(vf_flags(vf) & VF_ZIP)
		;

	return 0;
}


// wait until the transfer <hio> completes, and return its buffer.
// output parameters are zeroed on error.
int vfs_wait_io(Handle hio, void*& p, size_t& size)
{
	p = 0;
	size = 0;

	return 0;
}


// finished with transfer <hio> - free its buffer (returned by vfs_wait_read)
int vfs_discard_io(Handle& hio)
{
	return 0;
}