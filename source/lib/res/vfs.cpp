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
// a key to identify the watch (obviates separate Dir -> watch mapping).
//
// define this to strip out that code - removes .watch from struct Dir,
// and calls to res_watch_dir / res_cancel_watch.
//
// *: the add_watch code would need to iterate through subdirs and watch
//    each one, because the monitor API (e.g. FAM) may only be able to
//    watch single directories, instead of a whole subdirectory tree.
#undef NO_DIR_WATCH


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


//////////////////////////////////////////////////////////////////////////////
//
// path
//
//////////////////////////////////////////////////////////////////////////////

// path types:
// fn  : filename only, no path at all.
// f_* : path intended directly for underlying file layer.
//       component separator is '/'; no absolute paths, or ':', '\\' allowed.
// *   : as above, but path within the VFS.
//       "" is root dir; no absolute path allowed.
//
// . and .. directory entries aren't supported!


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

	int c = 0, last_c;

	// disallow absolute path for safety, in case of *nix systems.
	if(path[0] == '/')
	{
		msg = "absolute path";
		goto fail;
	}

	// scan each char in path string; count length.
	for(;;)
	{
		last_c = c;
		c = path[path_len++];

		// whole path is too long
		if(path_len >= VFS_MAX_PATH)
		{
			msg = "path too long";
			goto fail;
		}

		// disallow:
		// - ".." (prevent going above the VFS root dir)
		// - "/." and "./" (security whole when mounting,
		//   and not supported on Windows).
		// - "//" (makes no sense)
		if((c == '.' || c == '/') && (last_c == '.' || last_c == '/'))
		{
			msg = "contains '..', '/.', './', or '//'";
			goto fail;
		}

		// disallow OS-specific dir separators
		if(c == '\\' || c == ':')
		{
			msg = "contains OS-specific dir separator (e.g. '\\', ':')";
			goto fail;
		}

		// end of string, all is well.
		if(c == '\0')
			goto ok;
	}

	// failed somewhere - err is the error code,
	// or -1 if not set specifically above.
fail:
	debug_out("path_validate at line %d failed: %s (error code %d)", line, msg, err);
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
// one Loc is allocated for each archive or directory mounted.
// therefore, files only /point/ to a (possibly shared) Loc.
// if a file's location changes (e.g. after mounting a higher-priority
// directory), the VFS entry will point to the new Loc; the priority
// of both locations is unchanged.
//
// allocate via mnt_create, passing the location. do not free!
// we keep track of all Locs allocated; they are freed at exit,
// and by mnt_free_all (useful when rebuilding the VFS).
// this is much easier and safer than walking the VFS tree and
// freeing every location we find.


// not many instances => don't worry about struct size / alignment.
struct Loc
{
	Handle archive;

	const std::string mount_point;
	const std::string dir;

	uint pri;

	Loc(Handle _archive, const char* _mount_point, const char* _dir, uint _pri)
		: archive(_archive), mount_point(_mount_point), dir(_dir), pri(_pri) {}
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

typedef std::map<const std::string, const Loc*> Files;
typedef Files::iterator FileIt;
	// notes:
	// - Loc is allocated and owned by caller (the mount code)
	// - priority is accessed by following the Loc pointer.
	//   keeping a copy in the map would lead to better cache coherency,
	//   but it's a bit more clumsy (map filename to struct {pri, Loc*}).
	//   revisit if file lookup open is too slow (unlikely).

struct Dir;
typedef std::pair<const std::string, Dir> SubDir;
typedef std::map<const std::string, Dir> SubDirs;
typedef SubDirs::iterator SubDirIt;

struct Dir
{
#ifndef NO_DIR_WATCH
	intptr_t watch;
#endif

	int add_file(const char* name, const Loc* loc);
	const Loc* find_file(const char* name);

	int add_subdir(const char* name);
	Dir* find_subdir(const char* name);

	void clearR();

	SubDirs subdirs;
	Files files;

	uint mounts;

	// if exactly one real directory is mounted into this virtual dir,
	// this points to its location. used to add files to VFS when writing.
	// the Loc is actually in the mount info and is invalid when removed,
	// but the VFS will be rebuild in that case.
	//
	// valid iff mounts == 1
	const Loc* loc;

	Dir()
	{
		mounts = 0;
	}
};


int Dir::add_subdir(const char* const fn)
{
	if(find_file(fn) || find_subdir(fn))
	{
		debug_warn("add_subdir: file or subdirectory of same name already exists");
		return -1;
	}

	const std::string fn_s(fn);
	subdirs[fn_s];
		// side effect: maps <fn> to a newly constructed Dir()
		// non-const => cannot be optimized away.
	return 0;
}


Dir* Dir::find_subdir(const char* const fn)
{
	const std::string fn_s(fn);
	SubDirIt it = subdirs.find(fn_s);
	if(it == subdirs.end())
		return 0;
	return &it->second;
}


int Dir::add_file(const char* const fn, const Loc* const loc)
{
	if(find_subdir(fn))
	{
		debug_warn("add_file: file of same name already exists");
		return -1;
	}

	const std::string fn_s(fn);

	typedef const Loc* Data;
		// for absolute clarity; the container holds const Loc* objects.
		// operator[] returns a reference to that.
		// need this typedef to work around a GCC bug?
	Data& old_loc = files[fn_s];
		// default pointer ctor sets it to 0 =>
		// if fn wasn't already in the container, old_loc is 0.

	// old loc exists and is higher priority - keep it.
	if(old_loc && old_loc->pri > loc->pri)
		return 1;

	// new loc is greater or equal priority - replace old loc.
	// note: need to also replace if priority is the same, to allow multiple
	// patch archives; the one with the "largest" filename trumps the others.
	old_loc = loc;
	return 0;
}


const Loc* Dir::find_file(const char* const fn)
{
	const std::string fn_s(fn);
	FileIt it = files.find(fn_s);
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

#ifndef NO_DIR_WATCH
	res_cancel_watch(watch);
	watch = 0;
#endif
}




static Dir vfs_root;


// tree_lookup flags
enum
{
	LF_DEFAULT                   = 0,

	LF_CREATE_MISSING_DIRS       = 1,

	LF_CREATE_MISSING_FILE       = 2,

	LF_LAST                      = 2
};


// starts in VFS root directory (path = "").
// path may specify a file or directory; it need not end in '/'.
static int tree_lookup(const char* path, const Loc** const loc = 0, Dir** const dir = 0, uint flags = LF_DEFAULT)
{
	CHECK_PATH(path);
	assert(loc != 0 || dir != 0);
	assert(flags <= LF_LAST);

	const bool create_missing_components = !!(flags & LF_CREATE_MISSING_DIRS);
	const bool create_missing_files      = !!(flags & LF_CREATE_MISSING_FILE);

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
		const char* fn = cur_component;

		if(create_missing_files)
		{
			// dir wasn't populated via tree_add_dirR => don't know
			// the dir's Loc => cannot add this file.
			if(cur_dir->mounts != 1)
				return -1;

			CHECK_ERR(cur_dir->add_file(fn, cur_dir->loc));
		}

		*loc = cur_dir->find_file(fn);
		// the file (still) doesn't exist
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
	const Loc* const loc;
	FileCBParams(Dir* _dir, const Loc* _loc)
		: dir(_dir), loc(_loc) {}

private:
	FileCBParams& operator=(const FileCBParams&);
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
	UNUSED(size);

	const FileCBParams* const params = (FileCBParams*)user;
	Dir* const cur_dir       = params->dir;
	const Loc* const cur_loc = params->loc;

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


static int tree_add_dirR(Dir* const dir, const char* const f_path, const Loc* const loc)
{
	CHECK_PATH(f_path);

	dir->mounts++;
	dir->loc = loc;

	// add files and subdirs to vdir
	const FileCBParams params(dir, loc);
	file_enum(f_path, add_dirent_cb, (uintptr_t)&params);

#ifndef NO_DIR_WATCH
	res_watch_dir(f_path, &dir->watch);
#endif

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


static int tree_add_loc(Dir* const dir, const Loc* const loc)
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
// not const, because we h_free a handle in Loc
// (it resets the handle to 0)
typedef std::list<Loc> Locs;
typedef Locs::iterator LocIt;


struct Mount
{
	// note: we basically duplicate the mount information in dir_loc.
	// this is because it's needed in Mount when remounting, but also
	// in Loc when adding files. accessing everything via Loc is ugly.
	// it's no big deal - there won't be many mountings.

	// mounting into this VFS directory ("" for root)
	const std::string mount_point;

	// what is being mounted; either directory, or archive filename 
	const std::string f_name;

	uint pri;

	// storage for all Locs ensuing from this mounting.
	// it's safe to store pointers to them: the Mount and Locs containers
	// are std::lists; all pointers are reset after unmounting something.
	// the VFS stores Loc* pointers for each file and for dirs with exactly
	// 1 associated real dir, so that newly written files may be added.
	Loc dir_loc;
	Locs archive_locs;
		// if f_name is a single archive, this stores its Loc.
		// otherwise, there's one Loc for every archive in the directory
		// (but not its children - see remount()).

	Mount(const char* _mount_point, const char* _f_name, uint _pri)
		: mount_point(_mount_point), f_name(_f_name), pri(_pri),
		  dir_loc(0, _mount_point, _f_name, pri), archive_locs() {}
};

typedef std::list<Mount> Mounts;
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

	// will add one Loc to this container for
	// every archive successfully opened.
	Locs* archive_locs;
};

// called for each directory entry.
// add each successfully opened archive to list.
static int archive_cb(const char* const fn, const uint flags, const ssize_t size, const uintptr_t user)
{
	UNUSED(size);

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
		archive_locs->push_back(Loc(archive, "", "", pri));

	// only add archive to list; don't add its files into the VFS yet,
	// to simplify debugging (we see which files are in which archive)

	return 0;
}


// actually mount the specified entry (either Zip archive or dir).
// split out of vfs_mount because we need to mount without changing the
// mount list, when invalidating (reloading) the VFS.
static int remount(Mount& m)
{
	const char* mount_point = m.mount_point.c_str();
	const char* f_name      = m.f_name.c_str();
	const uint pri          = m.pri;
	const Loc* dir_loc      = &m.dir_loc;
	Locs& archive_locs      = m.archive_locs;

	Dir* dir;
	CHECK_ERR(tree_lookup(mount_point, 0, &dir, LF_CREATE_MISSING_DIRS));

	// check if target is a single Zip archive
	// (it can't also be a directory - prevented by OS FS)
	const Handle archive = zip_archive_open(f_name);
	if(archive > 0)
	{
		archive_locs.push_back(Loc(archive, "", "", pri));
		const Loc* loc = &archive_locs.front();
		return tree_add_loc(dir, loc);
	}

	// f_name is a directory (not Zip file - would have been opened above)

	// enumerate all archives in dir (but not its subdirs! see above.)
	ArchiveCBParams params = { f_name, pri, &archive_locs };
	file_enum(f_name, archive_cb, (uintptr_t)&params);

	// .. and add them
	for(LocIt it = archive_locs.begin(); it != archive_locs.end(); ++it)
	{
		const Loc* const loc = &*it;
		if(tree_add_loc(dir, loc) < 0)
			debug_warn("adding archive failed");
	}

	// add all loose files and subdirectories in subtree
	CHECK_ERR(tree_add_loc(dir, dir_loc));

	return 0;
}


static int unmount(Mount& m)
{
	for(LocIt it = m.archive_locs.begin(); it != m.archive_locs.end(); ++it)
	{
		Loc& loc = *it;
		zip_archive_close(loc.archive);
	}

	m.archive_locs.clear();
	return 0;
}


static inline void unmount_all(void)
	{ std::for_each(mounts.begin(), mounts.end(), unmount); }

static inline void remount_all()
	{ std::for_each(mounts.begin(), mounts.end(), remount); }


static void vfs_shutdown(void)
{
	tree_clear();
	unmount_all();
}


static bool is_subpath(const char* s1, const char* s2)
{
	// make sure s1 is the shorter string
	if(strlen(s1) > strlen(s2))
		std::swap(s1, s2);

	int c1 = 0, last_c1, c2;
	for(;;)
	{
		last_c1 = c1;
		c1 = *s1++, c2 = *s2++;

		// end of s1 reached
		if(c1 == '\0')
		{
			// s1 matched s2 up until:
			if(c2 == '\0' ||	// its end (i.e. they're equal length)
			   c2 == '/' ||		// start of next component
			   last_c1 == '/')	// ", but both have a trailing slash
				return true;
		}

		if(c1 != c2)
			return false;
	}
}


// mount either a single archive or a directory into the VFS at
// <vfs_mount_point>, which is created if it does not yet exist.
// new files override the previous VFS contents if pri(ority) is not lower.
// if <name> is a directory, all archives in that directory (but not
// its subdirs - see add_dirent_cb) are also mounted in alphabetical order.
// name = "." or "./" isn't allowed - see implementation for rationale.
int vfs_mount(const char* const mount_point, const char* const name, const uint pri)
{
	ONCE(atexit2(vfs_shutdown));

	CHECK_PATH(name);

	// make sure it's not already mounted, i.e. in mounts.
	// also prevents mounting a parent directory of a previously mounted
	// directory, or vice versa. example: mount $install/data and then
	// $install/data/mods/official - mods/official would also be accessible
	// from the first mount point - bad.
	// no matter if it's an archive - still shouldn't be a "subpath".
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		if(is_subpath(name, it->f_name.c_str()))
		{
			debug_warn("vfs_mount: already mounted");
			return -1;
		}

	// disallow "." because "./" isn't supported on Windows.
	// it would also create a loophole for the parent dir check above.
	// "./" and "/." are caught by CHECK_PATH.
	if(!strcmp(name, "."))
	{
		debug_warn("vfs_mount: mounting . not allowed");
		return -1;
	}

	mounts.push_back(Mount(mount_point, name, pri));

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




static int path_replace(char* dst, const char* src, const char* remove, const char* replace)
{
	// remove doesn't match start of <src>
	const size_t remove_len = strlen(remove);
	if(strncmp(src, remove, remove_len) != 0)
		return -1;

	// prepend replace.
	CHECK_ERR(path_append(dst, replace, src+remove_len));
	return 0;
}


int vfs_make_vfs_path(const char* const path, char* const vfs_path)
{
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
	{
		const char* remove = it->f_name.c_str();
		const char* replace = it->mount_point.c_str();

		if(path_replace(vfs_path, path, remove, replace) == 0)
			return 0;
	}

	return -1;
}


static int make_file_path(const char* vfs_path, const Loc* loc, char* const path)
{
	assert(loc->archive == 0);

	const char* remove = loc->mount_point.c_str();
	const char* replace = loc->dir.c_str();
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
	SubDirs* subdirs;
	SubDirIt subdir_it;
	Files* files;
	FileIt file_it;
};

H_TYPE_DEFINE(VDir);

static void VDir_init(VDir* vd, va_list args)
{
	UNUSED(vd);
	UNUSED(args);
}

static void VDir_dtor(VDir* vd)
{
	delete vd->subdirs;
	delete vd->files;
}

static int VDir_reload(VDir* vd, const char* path, Handle)
{
	if(vd->subdirs)
	{
		debug_warn("VDir_reload called when already loaded - why?");
		return 0;
	}

	Dir* dir;
	CHECK_ERR(tree_lookup(path, 0, &dir));

	// rationale for copying the dir's data: see VDir definition
	// note: bad_alloc exception handled by h_alloc.
	vd->subdirs   = new SubDirs(dir->subdirs);
	vd->subdir_it = vd->subdirs->begin();
	vd->files   = new Files(dir->files);
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


// return the next remaining directory entry (in alphabetical order) matching
// filter, or a negative error code on error (e.g. end of directory reached).
// filter values:
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
int vfs_realpath(const char* fn, char* realpath)
{
	const Loc* loc;
	CHECK_ERR(tree_lookup(fn, &loc));

	// file is in normal directory
	if(loc->archive <= 0)
		CHECK_ERR(make_file_path(fn, loc, realpath));
	// file is in archive
	else
	{
		const char* archive_fn = h_filename(loc->archive);
		if(!archive_fn)
			return -1;
		CHECK_ERR(path_append(realpath, archive_fn, fn));
	}

	return 0;
}


// does the specified file exist? return false on error.
// useful because a "file not found" warning is not raised, unlike vfs_stat.
bool vfs_exists(const char* fn)
{
	const Loc* loc;
	return (tree_lookup(fn, &loc) == 0);
}


// return information about the specified file as in stat(2),
// most notably size. stat buffer is undefined on error.
int vfs_stat(const char* fn, struct stat* s)
{
	const Loc* loc;
	CHECK_ERR(tree_lookup(fn, &loc));

	if(loc->archive > 0)
		CHECK_ERR(zip_stat(loc->archive, fn, s));
	else
	{
		char path[PATH_MAX];
		CHECK_ERR(make_file_path(fn, loc, path));
		CHECK_ERR(file_stat(path, s));
	}

	return 0;
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

	// be aware when adding fields that we're already pushing the size limit
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



static int VFile_reload(VFile* vf, const char* path, Handle)
{
	uint& flags = vf_flags(vf);

	// we're done if file is already open. need to check this because
	// reload order (e.g. if resource opens a file) is unspecified.
	if(flags & VF_OPEN)
		return 0;

	const Loc* loc;
	uint lf = (flags & FILE_WRITE)? LF_CREATE_MISSING_FILE : LF_DEFAULT;
	CHECK_ERR(tree_lookup(path, &loc, 0, lf));

	// normal file
	if(loc->archive <= 0)
	{
		char f_path[PATH_MAX];
		CHECK_ERR(make_file_path(path, loc, f_path));
		CHECK_ERR(file_open(f_path, flags, &vf->f));
	}
	// archive
	else
	{
		if(flags & FILE_WRITE)
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
// requested via FILE_WRITE flag, and is not possible for files in archives.
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


// try to transfer the next <size> bytes to/from the given file.
// (read or write access was chosen at file-open time).
// return bytes of actual data transferred, or a negative error code.
// TODO: buffer types
ssize_t vfs_io(const Handle hf, const size_t size, void** p)
{
#ifdef PARANOIA
debug_out("vfs_io size=%d\n", size);
#endif

	H_DEREF(hf, VFile, vf);

	off_t ofs = vf->ofs;
	vf->ofs += (off_t)size;

	// (vfs_open makes sure it's not opened for writing if zip)
	if(vf_flags(vf) & VF_ZIP)
		return zip_read(&vf->zf, ofs, size, p);

	// normal file:
	// let file_io alloc the buffer if the caller didn't (i.e. p = 0),
	// because it knows about alignment / padding requirements
	return file_io(&vf->f, ofs, size, p);
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
			assert(vf_size(vf) == (off_t)size && "vfs_load: mismatch between File and Mem size");
			hm = vf->hm;
			goto skip_read;
		}
		else
			debug_warn("vfs_load: invalid MEM attached to vfile (0 pointer)");
			// happens if someone frees the pointer. not an error!
	}

	{	// VC6 goto fix
	ssize_t nread = vfs_io(hf, size, &p);
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


// caveat: pads file to next max(4kb, sector_size) boundary
// (due to limitation of Win32 FILE_FLAG_NO_BUFFERING I/O).
// if that's a problem, specify FILE_NO_AIO when opening.
int vfs_store(const char* const fn, void* p, const size_t size, uint flags)
{
	Handle hf = vfs_open(fn, flags|FILE_WRITE);
	if(hf <= 0)
		return (int)hf;	// error code
	H_DEREF(hf, VFile, vf);
	const int ret = vfs_io(hf, size, &p);
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


// note: VFS and zip return the file I/O handle.
// there's no way for zip to do any extra processing, but that's unnecessary
// because aio is currently only supported for uncompressed files.


// begin transferring <size> bytes, starting at <ofs>. get result
// with vfs_wait_io; when no longer needed, free via vfs_discard_io.
Handle vfs_start_io(Handle hf, off_t ofs, size_t size, void* buf)
{
	H_DEREF(hf, VFile, vf);
	if(vf_flags(vf) & VF_ZIP)
		return zip_start_io(&vf->zf, ofs, size, buf);
	return file_start_io(&vf->f, ofs, size, buf);
}


// wait until the transfer <hio> completes, and return its buffer.
// output parameters are zeroed on error.
inline int vfs_wait_io(Handle hio, void*& p, size_t& size)
{
	return file_wait_io(hio, p, size);
}


// finished with transfer <hio> - free its buffer (returned by vfs_wait_io)
inline int vfs_discard_io(Handle& hio)
{
	return file_discard_io(hio);
}
