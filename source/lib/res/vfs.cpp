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
#include "hotload.h"	// see NO_DIR_WATCH

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
//#define NO_DIR_WATCH


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
// p_*: posix (e.g. mount object name or for open())
// v_*: vfs (e.g. mount point)
// fn : filename only (e.g. from readdir)
// dir_name: directory only, no path (subdir name)
//
// all paths must be relative (no leading '/'); components are separated
// by '/'; no ':', '\\', "." or ".." allowed; root dir is "".


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
		// - "./" (security hole when mounting and not supported on Windows).
		// allow "/.", because CVS backup files include it.
		if(last_c == '.' && (c == '.' || c == '/'))
		{
			msg = "contains '..' or './'";
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
	debug_out("path_validate at line %d failed: %s (error code %d)\n", line, msg, err);
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

	const std::string v_mount_point;
	const std::string p_dir;

	uint pri;

	Loc(Handle _archive, const char* _v_mount_point, const char* _p_dir, uint _pri)
		: archive(_archive), v_mount_point(_v_mount_point), p_dir(_p_dir), pri(_pri) {}
};


// case-independent string for comparing file/directory names.
// modified from GotW #29.
struct ci_char_traits : public std::char_traits<char>
	// inherit all the other functions that we don't need to override.
{
	static bool eq(char c1, char c2)
	{ return tolower(c1) == tolower(c2); }

	static bool ne(char c1, char c2)
	{ return tolower(c1) != tolower(c2); }

	static bool lt( char c1, char c2 )
	{ return tolower(c1) < tolower(c2); }

	static int compare(const char* s1, const char* s2, size_t n)
	{ return stricmp(s1, s2); }

	static const char* find(const char* s, int n, char a)
	{
		while(n-- > 0 && tolower(*s) != tolower(a))
			++s;
		return s;
	}
};

typedef std::basic_string<char, ci_char_traits> ci_string;



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

typedef std::pair<const ci_string, const Loc*> FileVal;
typedef std::map<const ci_string, const Loc*> Files;
typedef Files::iterator FileIt;
	// notes:
	// - Loc is allocated and owned by caller (the mount code)
	// - priority is accessed by following the Loc pointer.
	//   keeping a copy in the map would lead to better cache coherency,
	//   but it's a bit more clumsy (map filename to struct {pri, Loc*}).
	//   revisit if file lookup open is too slow (unlikely).

struct Dir;
typedef std::pair<const ci_string, Dir> SubDir;
typedef std::map<const ci_string, Dir> SubDirs;
typedef SubDirs::iterator SubDirIt;

struct Dir
{
#ifndef NO_DIR_WATCH
	intptr_t watch;
#endif

	int add_file(const char* fn, const Loc* loc);
	const Loc* find_file(const char* fn);

	int add_subdir(const char* d_name);
	Dir* find_subdir(const char* d_name);

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

#ifndef NO_DIR_WATCH
		watch = -1;
#endif
	}
};


int Dir::add_subdir(const char* const dir_name)
{
	if(find_file(dir_name))
	{
		debug_warn("add_subdir: file of same name already exists");
		return -1;
	}

	subdirs[dir_name];
		// side effect: maps <d_dir> to a newly constructed Dir()
		// non-const => cannot be optimized away.

	return 0;
}


Dir* Dir::find_subdir(const char* const dir_name)
{
	SubDirIt it = subdirs.find(dir_name);
	if(it == subdirs.end())
		return 0;
	return &it->second;
}


int Dir::add_file(const char* const fn, const Loc* const loc)
{
	if(find_subdir(fn))
	{
		debug_warn("add_file: subdir of same name already exists");
		return -1;
	}

	FileVal val = std::make_pair(fn, loc);
	std::pair<FileIt, bool> ret = files.insert(val);
	// if fn wasn't already in the list, old_loc = loc
	FileIt it = ret.first;
	const Loc*& old_loc = it->second;

	// older Loc is higher priority - keep it
	if(old_loc->pri > loc->pri)
		return 1;

	// new loc is greater or equal priority - replace old loc.
	// note: need to also replace if priority is the same, to allow multiple
	// patch archives; the one with the "largest" filename trumps the others.
	old_loc = loc;
	return 0;
}


const Loc* Dir::find_file(const char* const fn)
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

#ifndef NO_DIR_WATCH
	res_cancel_watch(watch);
	watch = 0;
#endif
}


///////////////////////////////////////////////////////////////////////////////


static Dir vfs_root;


// tree_lookup flags
enum LookupFlags
{
	LF_DEFAULT         = 0,

	LF_CREATE_MISSING  = 1,

	// only valid with LF_CREATE_MISSING.
	// *loc specifies the new file's loc
	LF_HAVE_LOC        = 2,

	LF_START_DIR       = 4,

	LF_LAST            = 8
};


// starts in VFS root directory (path = "").
// path may specify a file or directory; it need not end in '/'.
static int tree_lookup(const char* _c_path, const Loc** const loc = 0, Dir** const dir = 0, uint flags = LF_DEFAULT)
{
	CHECK_PATH(_c_path);
	assert(loc != 0 || dir != 0);
	assert(flags < LF_LAST);

	const bool create_missing = !!(flags & LF_CREATE_MISSING);
	const bool start_dir      = !!(flags & LF_START_DIR);
	const bool have_loc       = !!(flags & LF_HAVE_LOC);

	// copy into (writeable) buffer so we can 'tokenize' path components
	// by replacing '/' with '\0'. length check done by CHECK_PATH.
	char v_path[VFS_MAX_PATH];
	strcpy(v_path, _c_path);

	Dir* cur_dir = &vfs_root;
	if(start_dir)
		cur_dir = *dir;
	char* cur_component = v_path;

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
		char* slash = (char*)strchr(cur_component, '/');
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
		if(create_missing)
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

		if(create_missing)
		{
			const Loc* new_loc;

			if(have_loc)
				new_loc = *loc;
			else
			{
				// dir wasn't populated via tree_add_dirR => don't know
				// the dir's Loc => cannot add this file.
				if(cur_dir->mounts != 1)
					return -1;

				new_loc = cur_dir->loc;
			}

			CHECK_ERR(cur_dir->add_file(fn, new_loc));
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

// called for each dirent in OS dirs or archives.
// add each file and directory to the VFS dir.
//
// note:
// we don't mount archives here for performance reasons.
// that means archives in subdirectories of mount points aren't added!
// rationale: can't determine if file is an archive via extension -
// they might be called .pk3 or whatnot. for every file in the tree,
// we'd have to try to open it as an archive - not good.
// this restriction also simplifies the code a bit, but if it's a problem,
// just generate a list of archives here and mount them from the caller.
static int add_dirent_cb(const char* const n_name, const ssize_t size, const uintptr_t user)
{
	const FileCBParams* const params = (FileCBParams*)user;
	Dir* cur_dir       = params->dir;
	const Loc* cur_loc = params->loc;

	const Loc** ploc = &cur_loc;
		// default; set to 0 if a directory.
	Dir** pdir = &cur_dir;
		// ignored unless LF_START_DIR is set.
	uint lf_flags = LF_CREATE_MISSING;
		// note: WinZip sometimes outputs files before their dir name =>
		// we have to create missing components for each filename/path.

	// it's a dir.
	// in lookup, need to make sure last component is treated as a dir.
	if(size < 0)
        ploc = 0;

	// it's in an archive.
	// when adding a file, set its Loc to that of the archive.
	// if it's a dir, ploc has been zeroed (it's not needed).
	if(cur_loc->archive > 0)
		lf_flags |= LF_HAVE_LOC;
	// it's a normal OS file or dir.
	// start path lookup at cur_dir (set above), since n_name is
	// the dirent name only (i.e. not full path).
	else
		lf_flags |= LF_START_DIR;

	CHECK_ERR(tree_lookup(n_name, ploc, pdir, lf_flags));
	return 0;
}


static int tree_add_dirR(Dir* const dir, const char* const p_path, const Loc* const loc)
{
	dir->mounts++;
	dir->loc = loc;

	// add files and subdirs to vdir
	const FileCBParams params(dir, loc);
	file_enum(p_path, add_dirent_cb, (uintptr_t)&params);

#ifndef NO_DIR_WATCH
	res_watch_dir(p_path, &dir->watch);
#endif

	// recurse over all subdirs
	for(SubDirIt it = dir->subdirs.begin(); it != dir->subdirs.end(); ++it)
	{
		Dir* const subdir = &it->second;
		const char* const d_subdir_name = (it->first).c_str();

		// don't clutter the tree with CVS dirs.
		// only applicable for normal dirs, since archives don't include CVS.
		if(!strcmp(d_subdir_name, "CVS"))
			continue;

		char p_subdir_path[PATH_MAX];
		CHECK_ERR(path_append(p_subdir_path, p_path, d_subdir_name));

		tree_add_dirR(subdir, p_subdir_path, loc);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// mount directories into the VFS
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
	const std::string v_mount_point;

	// name of directory being mounted
	const std::string p_dir;

	uint pri;

	// storage for all Locs ensuing from this mounting.
	// it's safe to store pointers to them: the Mount and Locs containers
	// are std::lists; all pointers are reset after unmounting something.
	// the VFS stores Loc* pointers for each file and for dirs with exactly
	// 1 associated real dir, so that newly written files may be added.
	Loc dir_loc;
	Locs archive_locs;
		// contains one Loc for every archive in the directory
		// (but not its children - see remount()).

	Mount(const char* _v_mount_point, const char* _p_dir, uint _pri)
		: v_mount_point(_v_mount_point), p_dir(_p_dir), pri(_pri),
		  dir_loc(0, _v_mount_point, _p_dir, pri), archive_locs() {}
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
	const char* p_dir;

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
static int archive_cb(const char* const fn, const ssize_t size, const uintptr_t user)
{
	// not interested in directories
	if(size < 0)
		return 0;

	const ArchiveCBParams* const params = (ArchiveCBParams*)user;
	const char* const p_dir  = params->p_dir;
	const uint pri           = params->pri;
	Locs* const archive_locs = params->archive_locs;

	// get full path (fn is filename only)
	char p_path[PATH_MAX];
	CHECK_ERR(path_append(p_path, p_dir, fn));

	// don't check filename extension - archives won't necessarily
	// be called .zip (example: Quake III .pk3).
	// just try to open the file.
	const Handle archive = zip_archive_open(p_path);
	if(archive > 0)
		archive_locs->push_back(Loc(archive, "", "", pri));

	// only add archive to list; don't add its files into the VFS yet,
	// to simplify debugging (we see which files are in which archive)

	return 0;
}


// actually mount the specified entry. split out of vfs_mount,
// because when invalidating (reloading) the VFS, we need to
// be able to mount without changing the mount list.
static int remount(Mount& m)
{
	const char* v_mount_point = m.v_mount_point.c_str();
	const char* p_dir         = m.p_dir.c_str();
	const uint pri            = m.pri;
	const Loc* dir_loc        = &m.dir_loc;
	Locs& archive_locs        = m.archive_locs;

	Dir* dir;
	CHECK_ERR(tree_lookup(v_mount_point, 0, &dir, LF_CREATE_MISSING));

	// add all loose files and subdirectories in subtree
	// (before adding archives, so that it doesn't try to add subdirs
	// that are only in the archive).
	if(tree_add_dirR(dir, p_dir, dir_loc) < 0)
		debug_warn("remount: adding files failed");

	// enumerate all archives in dir (but not its subdirs! see above.)
	ArchiveCBParams params = { p_dir, pri, &archive_locs };
	file_enum(p_dir, archive_cb, (uintptr_t)&params);

	// .. and add them
	for(LocIt it = archive_locs.begin(); it != archive_locs.end(); ++it)
	{
		const Loc* const loc = &*it;
		const FileCBParams params(dir, loc);
		if(zip_enum(loc->archive, add_dirent_cb, (uintptr_t)&params) < 0)
			debug_warn("remount: adding archive failed");
			// don't CHECK_ERR, because we should try to mount the remaining
			// archives anyway.
	}

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
int vfs_mount(const char* const v_mount_point, const char* const p_dir, const uint pri)
{
	ONCE(atexit2(vfs_shutdown));

	// make sure it's not already mounted, i.e. in mounts.
	// also prevents mounting a parent directory of a previously mounted
	// directory, or vice versa. example: mount $install/data and then
	// $install/data/mods/official - mods/official would also be accessible
	// from the first mount point - bad.
	// no matter if it's an archive - still shouldn't be a "subpath".
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		if(is_subpath(p_dir, it->p_dir.c_str()))
		{
			debug_warn("vfs_mount: already mounted");
			return -1;
		}

	// disallow "." because "./" isn't supported on Windows.
	// it would also create a loophole for the parent dir check above.
	// "./" and "/." are caught by CHECK_PATH.
	if(!strcmp(p_dir, "."))
	{
		debug_warn("vfs_mount: mounting . not allowed");
		return -1;
	}

	mounts.push_back(Mount(v_mount_point, p_dir, pri));

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
	tree_clear();

	unmount_all();
	remount_all();
	return 0;
}


// unmount a previously mounted item, and rebuild the VFS afterwards.
int vfs_unmount(const char* p_dir)
{
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		// found the corresponding entry
		if(it->p_dir == p_dir)
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

	// get rid of trailing / in src (must not be included in remove)
	const char* start = src+remove_len;
	if(*start == '/' || *start == DIR_SEP)
		start++;

	// prepend replace.
	CHECK_ERR(path_append(dst, replace, start));
	return 0;
}


int vfs_make_vfs_path(const char* const path, char* const vfs_path)
{
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
	{
		const char* remove = it->p_dir.c_str();
		const char* replace = it->v_mount_point.c_str();

		if(path_replace(vfs_path, path, remove, replace) == 0)
			return 0;
	}

	return -1;
}


static int make_file_path(const char* vfs_path, const Loc* loc, char* const path)
{
	assert(loc->archive == 0);

	const char* remove = loc->v_mount_point.c_str();
	const char* replace = loc->p_dir.c_str();
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
Handle vfs_open_dir(const char* const v_dir)
{
	return h_alloc(H_VDir, v_dir, RES_NO_CACHE);
		// must not cache, since the position in file array
		// is advanced => not copy-equivalent.
}


// close the handle to a directory.
// all vfsDirEnt.name strings are now invalid.
int vfs_close_dir(Handle& hd)
{
	return h_free(hd, H_VDir);
}


// retrieve the next dir entry (in alphabetical order) matching <filter>.
// return 0 on success, ERR_VFS_DIR_END if no matching entry was found,
// or a negative error code on failure.
// filter values:
// - 0: any file;
// - "/": any subdirectory
// - anything else: pattern for name (may include '?' and '*' wildcards)
//
// rationale: the filename is currently stored internally as
// std::string (=> less manual memory allocation). we don't want to
// return a reference, because that would break C compatibility.
// we're trying to avoid fixed-size buffers, so that is out as well.
// finally, allocating a copy is not so good because it has to be
// freed by the user (won't happen). returning a volatile pointer
// to the string itself via c_str is the only remaining option.
int vfs_next_dirent(const Handle hd, vfsDirEnt* ent, const char* const filter)
{
	H_DEREF(hd, VDir, vd);

	// caller wants a subdirectory; return the next one.
	if(filter && filter[0] == '/' && filter[1] == '\0')
	{
		if(vd->subdir_it == vd->subdirs->end())
			return ERR_VFS_DIR_END;
		ent->name = vd->subdir_it->first.c_str();
		++vd->subdir_it;
		return 0;
	}

	// caller wants a file; loop until one matches or end of list.
	for(;;)
	{
		if(vd->file_it == vd->files->end())
			return ERR_VFS_DIR_END;
		ent->name = vd->file_it->first.c_str();
		++vd->file_it;

		if(!filter || match_wildcard(ent->name, filter))
			return 0;
	}
}


// return actual path to the specified file:
// "<real_directory>/fn" or "<archive_name>/fn".
int vfs_realpath(const char* v_path, char* realpath)
{
	const Loc* loc;
	CHECK_ERR(tree_lookup(v_path, &loc));

	// file is in normal directory
	if(loc->archive <= 0)
		CHECK_ERR(make_file_path(v_path, loc, realpath));
	// file is in archive
	else
	{
		const char* archive_fn = h_filename(loc->archive);
		if(!archive_fn)
			return -1;
		CHECK_ERR(path_append(realpath, archive_fn, v_path));
	}

	return 0;
}


// does the specified file exist? return false on error.
// useful because a "file not found" warning is not raised, unlike vfs_stat.
bool vfs_exists(const char* v_fn)
{
	const Loc* loc;
	return (tree_lookup(v_fn, &loc) == 0);
}


// get file status (size, mtime). output param is zeroed on error.
int vfs_stat(const char* v_fn, struct stat* s)
{
	const Loc* loc;
	CHECK_ERR(tree_lookup(v_fn, &loc));

	const char* z_fn = v_fn;	// Zip is case-insensitive as well
	if(loc->archive > 0)
		CHECK_ERR(zip_stat(loc->archive, z_fn, s));
	else
	{
		char p_fn[PATH_MAX];
		CHECK_ERR(make_file_path(v_fn, loc, p_fn));
		CHECK_ERR(file_stat(p_fn, s));
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// file
//
///////////////////////////////////////////////////////////////////////////////


// logging

static int file_listing_enabled;
	// tristate; -1 = already shut down

static FILE* file_list;


static void file_listing_shutdown()
{
	fclose(file_list);
	file_listing_enabled = -1;
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
		ONCE(atexit(file_listing_shutdown));
			// ONCE necessary to prevent multiple atexits if fopen fails.

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



static int VFile_reload(VFile* vf, const char* v_fn, Handle)
{
	uint& flags = vf_flags(vf);

	// we're done if file is already open. need to check this because
	// reload order (e.g. if resource opens a file) is unspecified.
	if(flags & VF_OPEN)
		return 0;

	file_listing_add(v_fn);

	const Loc* loc;
	uint lf = (flags & FILE_WRITE)? LF_CREATE_MISSING : LF_DEFAULT;
	int err = tree_lookup(v_fn, &loc, 0, lf);
	if(err < 0)
	{
		// don't CHECK_ERR - this happens often and the dialog is annoying
		debug_out("tree_lookup failed for %s\n", v_fn);
		return err;
	}


	// normal file
	if(loc->archive <= 0)
	{
		char p_fn[PATH_MAX];
		CHECK_ERR(make_file_path(v_fn, loc, p_fn));
		CHECK_ERR(file_open(p_fn, flags, &vf->f));
	}
	// archive
	else
	{
		if(flags & FILE_WRITE)
		{
			debug_warn("requesting write access to file in archive");
			return -1;
		}

		CHECK_ERR(zip_open(loc->archive, v_fn, &vf->zf));

		flags |= VF_ZIP;
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
Handle vfs_open(const char* v_fn, uint file_flags /* = 0 */)
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
	debug_out("TOTAL TIME IN VFS_IO: %f\nthroughput: %f MB/s\n\n", dt, totaldata/dt/1e6);
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
Handle vfs_load(const char* const v_fn, void*& p, size_t& size, uint flags /* default 0 */)
{
#ifdef PARANOIA
debug_out("vfs_load v_fn=%s\n", v_fn);
#endif

	p = 0; size = 0;	// zeroed in case vfs_open or H_DEREF fails

	Handle hf = vfs_open(v_fn, flags);
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
		const size_t BLOCK_SIZE = 64*KB;
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
int vfs_store(const char* const v_fn, void* p, const size_t size, uint flags /* default 0 */)
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
static int IO_reload(IO* io, const char*, Handle)
{
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
