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
// a key to identify the watch (obviates separate TDir -> watch mapping).
//
// define this to strip out that code - removes .watch from struct TDir,
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
// grammar:
// path ::= dir*file?
// dir  ::= name/
// file ::= name
// name ::= [^/]



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
		return ERR_PATH_LENGTH;

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



// not many instances => don't worry about struct size / alignment.
struct TLoc
{
	Handle archive;
		// not freed in dtor, so that users don't have to avoid
		// TLoc temporary objects (that would free the archive)

	const std::string v_mount_point;
	const std::string p_dir_name;

	uint pri;

	TLoc(Handle _archive, const char* _v_mount_point, const char* _p_dir_name, uint _pri)
		: v_mount_point(_v_mount_point), p_dir_name(_p_dir_name)
	{
		archive = _archive;
		pri = _pri;
	}
};


// container must not invalidate iterators after insertion!
// (we keep and pass around pointers to Mount.archive_locs elements)
// see below.
//
// not const, because we h_free a handle in Loc
// (it resets the handle to 0)
typedef std::list<TLoc> TLocs;
typedef TLocs::iterator TLocIt;




// rationale for separate file / subdir containers:
// problems:
// - more code for insertion (oh well);
// - makes ordered output of all dirents difficult
//   (but dirs and files are usually displayed separately)
// advantages:
// - simplifies lookup code: it can just check if a path is there,
//   no need to check if the entry is actually a directory
// - storing TDir objects directly in the map means less
//   memory allocations / no need to free them.
// - sharing a TNode object means every function has to check
//   if it's really a directory
// - less memory use (TDir+bool is_dir are somewhat bigger)
//
// add_* aborts if a subdir or file of the same name already exists.

struct TFile
{
	// required:
	const TLoc* loc;
		// allocated and owned by caller (mount code)

	// set by ctor if struct stat is available:
	time_t mtime;
	off_t size;

	// these can also be extracted from TLoc,
	// but better cache coherency when accessing them here.
	u16 pri;
	u32 in_archive : 1;

	TFile(const TLoc* _loc = 0, const struct stat* s = 0)
	{
		loc = _loc;

		if(s)
			mtime = s->st_mtime, size = s->st_size;
		else
			mtime = 0, size = 0;

		pri        = _loc->pri;
		in_archive = _loc->archive > 0;
	}
};


typedef std::pair<const ci_string, TFile> TFilePair;
typedef std:: map<const ci_string, TFile> TFiles;
typedef TFiles::iterator TFileIt;

struct TDir;
typedef std::pair<const ci_string, TDir> TDirPair;
typedef std:: map<const ci_string, TDir> TDirs;
typedef TDirs::iterator TDirIt;

struct TDir
{
#ifndef NO_DIR_WATCH
	intptr_t watch;
#endif

	TDirs subdirs;
	TFiles files;

	// if exactly one real directory is mounted into this virtual dir,
	// this points to its location. used to add files to VFS when writing.
	// the Loc is actually in the mount info and is invalid when removed,
	// but the VFS will be rebuilt in that case.
	const TLoc* loc;


	int add_file(const char* fn, const TLoc* loc, const struct stat* s = 0);
	TFile* find_file(const char* fn);

	int add_subdir(const char* d_name);
	TDir* find_subdir(const char* d_name);

	void clearR();


	TDir()
	{
#ifndef NO_DIR_WATCH
		watch = -1;
#endif
		loc = 0;
	}
};


int TDir::add_subdir(const char* const dir_name)
{
	if(find_file(dir_name))
	{
		debug_warn("TDir::add_subdir: file of same name already exists");
		return -1;
	}

	TDirPair pair = std::make_pair(dir_name, TDir());
	std::pair<TDirIt, bool> ret = subdirs.insert(pair);
	return ret.second? 0 : 1;
}


TDir* TDir::find_subdir(const char* dir_name)
{
	TDirIt it = subdirs.find(dir_name);
	if(it == subdirs.end())
		return 0;
	return &it->second;
}


int TDir::add_file(const char* fn, const TLoc* loc, const struct stat* s)
{
	if(find_subdir(fn))
	{
		debug_warn("TDir::add_file: subdir of same name already exists");
		return -1;
	}

	TFilePair pair = std::make_pair(fn, TFile(loc, s));
	std::pair<TFileIt, bool> ret = files.insert(pair);
	return ret.second? 0 : 1;
}


TFile* TDir::find_file(const char* fn)
{
	TFileIt it = files.find(fn);
	if(it == files.end())
		return 0;
	return &it->second;
}


void TDir::clearR()
{
	TDirIt it;
	for(it = subdirs.begin(); it != subdirs.end(); ++it)
	{
		TDir& subdir = it->second;
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


static TDir vfs_root;


// tree_lookup* flags
enum LookupFlags
{
	LF_DEFAULT         = 0,
	LF_CREATE_MISSING  = 1
};



// starts in VFS root directory (path = ""), or in *dir if flags & LF_START_DIR.
// path may specify a file or directory and must end in '/' if specifying a dir!
static int tree_lookup_dir(const char* path, TDir** pdir, uint flags = LF_DEFAULT)
{
	CHECK_PATH(path);
	assert(!(flags & ~LF_CREATE_MISSING));	// no undefined bits set
	// can't check if path ends in '/' - we're called via tree_lookup also.

	const bool create_missing = !!(flags & LF_CREATE_MISSING);

	// copy into (writeable) buffer so we can 'tokenize' path components
	// by replacing '/' with '\0'. length check done by CHECK_PATH.
	char v_path[VFS_MAX_PATH];
	strcpy(v_path, path);
	char* cur_component = v_path;

	TDir* cur_dir = &vfs_root;

	// grammar:
	// path ::= dir*file?
	// dir  ::= name/
	// file ::= name
	// name ::= [^/]

	// successively navigate to the next subdirectory in <path>.
	for(;;)
	{
		// "extract" cur_component string (0-terminate by replacing '/')
		char* slash = (char*)strchr(cur_component, '/');
		if(!slash)
			break;
		*slash = '\0';

		// create <cur_component> subdir (no-op if it already exists)
		if(create_missing)
			CHECK_ERR(cur_dir->add_subdir(cur_component));

		cur_dir = cur_dir->find_subdir(cur_component);
		if(!cur_dir)
			return ERR_PATH_NOT_FOUND;

		cur_component = slash+1;
	}

	// success.
	*pdir = cur_dir;
	return 0;
}



// if flags & LF_CREATE_MISSING, add the file to the VFS tree,
// unless a higher-priority file of the same name already exists.
// (used by VFile_reload when opening for writing).
static int tree_lookup(const char* path, TFile** pfile, uint flags = LF_DEFAULT)
{
	// path and flags checked by tree_lookup_dir

	TDir* dir;
	CHECK_ERR(tree_lookup_dir(path, &dir, flags));

	const bool create_missing = !!(flags & LF_CREATE_MISSING);

	const char* fn = path;
	const char* slash = strrchr(path, '/');
	if(slash)
		fn = slash+1;

	// empty filename ("" or "dir/" etc.)
	if(fn[0] == '\0')
		return -1;

	if(create_missing) 
	{
		// dir wasn't populated via tree_add_dirR (i.e. we don't know its loc) OR
		// more than one phys dir mounted here => can't add
		if(!dir->loc || dir->loc == (TLoc*)-1)
			return -1;

		CHECK_ERR(dir->add_file(fn, dir->loc));
	}

	*pfile = dir->find_file(fn);
	// the file (still) doesn't exist
	if(!*pfile)
		return ERR_FILE_NOT_FOUND;

	return 0;
}


static inline void tree_clear()
{
	vfs_root.clearR();
}







struct FileCBParams
{
	TDir* const dir;
	const TLoc* cur_loc;
	TLocs* archive_locs;
	FileCBParams(TDir* _dir, TLoc* _cur_loc, TLocs* _archive_locs)
		: dir(_dir), cur_loc(_cur_loc), archive_locs(_archive_locs) {}

	// no copy ctor, since some members are const
private:
	FileCBParams& operator=(const FileCBParams&);
};


static int override(TFile* old_file, TFile* new_file)
{
	// older is higher priority - keep it.
	// note: if priority is the same, replace!
	//   this allows multiple patch archives; the one with the "largest"
	//   filename trumps the others.
	if(old_file->pri > new_file->pri)
		return 0;

	bool is_same = (old_file->size == new_file->size);
	is_same &= fabs(difftime(old_file->mtime, new_file->mtime)) <= 2.0;
	// FAT timestamp has 2 second resolution

	// not more important - keep current file.
	// base case is if old_file is in an archive.
	// this way, no need to flip them if new_file is an archive.
	if(((int)is_same ^ (int)old_file->in_archive) == 0)
		return 0;

	// new_file is more important -> override old_file.
	return 1;
}

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
static int add_dirent_cb(const char* const n_name, const struct stat* s, const uintptr_t user)
{
	const FileCBParams* const params = (FileCBParams*)user;
	TDir* cur_dir       = params->dir;
	const TLoc* cur_loc = params->cur_loc;
	TLocs* archive_locs   = params->archive_locs;

	// loose (i.e. not in archive) directory: add it.
	// note: when called by zip_enum, we only get files in the archive.
	if(S_ISDIR(s->st_mode))
		return cur_dir->add_subdir(n_name);


	const char* fn = n_name;

	// loose file
	if(cur_loc->archive <= 0)
	{
		// at first level of nesting (i.e. dir being mounted):
		// test if file is an archive; if so, mount it
		// (we only do this at the first nesting level
		// to avoid testing every single file - slow)
		if(archive_locs)
		{
			char path[PATH_MAX];
			path_append(path, cur_loc->p_dir_name.c_str(), n_name);
			// don't check filename extension - archives won't necessarily
			// be called .zip (example: Quake III .pk3).
			// just try to open the file.
			Handle archive = zip_archive_open(path);
			if(archive > 0)
			{
				archive_locs->push_back(TLoc(archive, "", "", cur_loc->pri));
				FileCBParams params(cur_dir, &archive_locs->back(), 0);
				zip_enum(archive, add_dirent_cb, (uintptr_t)&params);
				return 0;
					// don't add as a file
			}
		}
	}
	// archived file
	else
	{
		// ignore directories (virtual dirs are created when adding files).
		// can't rely on seeing dirs first to create dirs: WinZip sometimes
		// adds files before their directory entry =>
		// we have to create missing components for each filename.

		CHECK_ERR(tree_lookup_dir(n_name, &cur_dir, LF_CREATE_MISSING));
		const char* slash = strrchr(n_name, '/');
		if(slash)
			fn = slash+1;
	}

	int ret = cur_dir->add_file(fn, cur_loc, s);
	// wasn't in dir yet - added and done
	if(ret == 0)
		return 0;

	TFile new_file(cur_loc, s);
	TFile* old_file = cur_dir->find_file(fn);
	if(override(old_file, &new_file) == 1)
		*old_file = new_file;

	return 0;
}


static int tree_add_dirR(TDir* const dir, const char* const p_path, TLoc* const loc, TLocs* archive_locs)
{
	if(dir->loc)
		dir->loc = (TLoc*)-1;
	else
		dir->loc = loc;

#ifndef NO_DIR_WATCH
	res_watch_dir(p_path, &dir->watch);
#endif

	// add files and subdirs to vdir
	const FileCBParams params(dir, loc, archive_locs);
	file_enum(p_path, add_dirent_cb, (uintptr_t)&params);

	// recurse over all subdirs
	for(TDirIt it = dir->subdirs.begin(); it != dir->subdirs.end(); ++it)
	{
		TDir* const subdir = &it->second;
		const char* const d_subdir_name = (it->first).c_str();

		// don't clutter the tree with versioning system dirs.
		// only applicable for normal dirs; the archive builder
		// takes care of removing these there.
		if(!strcmp(d_subdir_name, "CVS") || !strcmp(d_subdir_name, ".svn"))
			continue;

		char p_subdir_path[PATH_MAX];
		CHECK_ERR(path_append(p_subdir_path, p_path, d_subdir_name));

		tree_add_dirR(subdir, p_subdir_path, loc, 0);
	}

	return 0;
}


static int tree_displayR(TDir* const dir, int indent = 0)
{
	// dump files in this dir
	for(TFileIt file_it = dir->files.begin(); file_it != dir->files.end(); ++file_it)
	{
		const char* name = file_it->first.c_str();
		TFile* file = &file_it->second;

		char is_archive = file->in_archive? 'A' : 'L';
		char* timestamp = ctime(&file->mtime);
		timestamp[24] = '\0';	// remove '\n'
		const off_t size = file->size;

		for(int i = 0; i < indent; i++)
			printf("    ");
		char fmt[25];
		int chars = 80 - indent*4;
		sprintf(fmt, "%%-%d.%ds (%%c; %%6d; %%s)\n", chars, chars);
		printf(fmt, name, is_archive, size, timestamp);
	}

	// recurse over all subdirs
	for(TDirIt dir_it = dir->subdirs.begin(); dir_it != dir->subdirs.end(); ++dir_it)
	{
		TDir* const subdir = &dir_it->second;
		const char* subdir_name = dir_it->first.c_str();

		// write subdir's name
		// note: do it now, instead of in recursive call so that:
		// - we don't have to pass dir_name parameter;
		// - the VFS root node isn't displayed.
		for(int i = 0; i < indent; i++)
			printf("    ");
		printf("[%s/]\n", subdir_name);

		tree_displayR(subdir, indent+1);
	}

	return 0;
}


int vfs_display()
{
	return tree_displayR(&vfs_root);
}



///////////////////////////////////////////////////////////////////////////////
//
// mount directories into the VFS
//
///////////////////////////////////////////////////////////////////////////////


struct Mount
{
	// note: we basically duplicate the mount information in dir_loc.
	// this is because it's needed in Mount when remounting, but also
	// in Loc when adding files. accessing everything via Loc is ugly.
	// it's no big deal - there won't be many mountings.

	// mounting into this VFS directory ("" for root)
	const std::string v_mount_point;

	// name of directory being mounted
	const std::string p_dir_name;

	uint pri;

	// storage for all Locs ensuing from this mounting.
	// it's safe to store pointers to them: the Mount and Locs containers
	// are std::lists; all pointers are reset after unmounting something.
	// the VFS stores TLoc* pointers for each file and for dirs with exactly
	// 1 associated real dir, so that newly written files may be added.
	TLoc dir_loc;
	TLocs archive_locs;
		// contains one Loc for every archive in the directory
		// (but not its children - see remount()).
		//
		// supports mounting multiple archives in a directory
		// (useful for mix-in mods and patches).
		//
		// archives are added here in alphabetical order!


	Mount(const char* _v_mount_point, const char* _p_dir, uint _pri)
		: v_mount_point(_v_mount_point), p_dir_name(_p_dir), pri(_pri),
		  dir_loc(0, _v_mount_point, _p_dir, pri), archive_locs() {}
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
	const char* p_dir_name    = m.p_dir_name.c_str();
	const uint pri            = m.pri;
	TLoc* dir_loc             = &m.dir_loc;
	TLocs& archive_locs       = m.archive_locs;

	// callers have a tendency to forget required trailing '/';
	// complain if it's not there, unless path = "" (root dir)
#ifndef NDEBUG
	size_t len = strlen(v_mount_point);
	if(len && v_mount_point[len-1] != '/')
		debug_warn("remount: path doesn't end in '/'");
#endif

	TDir* dir;
	CHECK_ERR(tree_lookup_dir(v_mount_point, &dir, LF_CREATE_MISSING));

	// add all loose files and subdirectories in subtree
	// (before adding archives, so that it doesn't try to add subdirs
	// that are only in the archive).
	if(tree_add_dirR(dir, p_dir_name, dir_loc, &archive_locs) < 0)
		debug_warn("remount: adding files failed");

	return 0;
}


static int unmount(Mount& m)
{
	for(TLocIt it = m.archive_locs.begin(); it != m.archive_locs.end(); ++it)
		zip_archive_close(it->archive);
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
int vfs_mount(const char* const v_mount_point, const char* const p_dir_name, const uint pri)
{
	ONCE(atexit2(vfs_shutdown));

	// make sure it's not already mounted, i.e. in mounts.
	// also prevents mounting a parent directory of a previously mounted
	// directory, or vice versa. example: mount $install/data and then
	// $install/data/mods/official - mods/official would also be accessible
	// from the first mount point - bad.
	// no matter if it's an archive - still shouldn't be a "subpath".
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		if(is_subpath(p_dir_name, it->p_dir_name.c_str()))
		{
			debug_warn("vfs_mount: already mounted");
			return -1;
		}

	// disallow "." because "./" isn't supported on Windows.
	// it would also create a loophole for the parent dir check above.
	// "./" and "/." are caught by CHECK_PATH.
	if(!strcmp(p_dir_name, "."))
	{
		debug_warn("vfs_mount: mounting . not allowed");
		return -1;
	}

	mounts.push_back(Mount(v_mount_point, p_dir_name, pri));

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
int vfs_unmount(const char* p_dir_name)
{
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		// found the corresponding entry
		if(it->p_dir_name == p_dir_name)
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
		const char* remove = it->p_dir_name.c_str();
		const char* replace = it->v_mount_point.c_str();

		if(path_replace(vfs_path, path, remove, replace) == 0)
			return 0;
	}

	return -1;
}


static int make_file_path(const char* vfs_path, const TLoc* loc, char* const path)
{
	assert(loc->archive == 0);

	const char* remove = loc->v_mount_point.c_str();
	const char* replace = loc->p_dir_name.c_str();
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
	TDirs* subdirs;
	TDirIt subdir_it;
	TFiles* files;
	TFileIt file_it;
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

	// make caller's life easier and add required trailing slash
	// if not already present
	char path_slash[PATH_MAX];
	CHECK_ERR(path_append(path_slash, path, ""));

	TDir* dir;
	CHECK_ERR(tree_lookup_dir(path_slash, &dir));

	// rationale for copying the dir's data: see VDir definition
	// note: bad_alloc exception handled by h_alloc.
	vd->subdirs   = new TDirs(dir->subdirs);
	vd->subdir_it = vd->subdirs->begin();
	vd->files   = new TFiles(dir->files);
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
// return 0 on success, ERR_DIR_END if no matching entry was found,
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
			return ERR_DIR_END;
		ent->name = vd->subdir_it->first.c_str();
		++vd->subdir_it;
		return 0;
	}

	// caller wants a file; loop until one matches or end of list.
	for(;;)
	{
		if(vd->file_it == vd->files->end())
			return ERR_DIR_END;
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
	TFile* file;
	CHECK_ERR(tree_lookup(v_path, &file));

	if(file->in_archive)
	{
		const char* archive_fn = h_filename(file->loc->archive);
		if(!archive_fn)
			return -1;
		CHECK_ERR(path_append(realpath, archive_fn, v_path));
	}
	// file is in normal directory
	else
		CHECK_ERR(make_file_path(v_path, file->loc, realpath));

	return 0;
}


// does the specified file exist? return false on error.
// useful because a "file not found" warning is not raised, unlike vfs_stat.
bool vfs_exists(const char* v_fn)
{
	TFile* file;
	return (tree_lookup(v_fn, &file) == 0);
}


// get file status (size, mtime). output param is zeroed on error.
int vfs_stat(const char* v_path, struct stat* s)
{
	TFile* file;
	CHECK_ERR(tree_lookup(v_path, &file));

	// all stat members currently supported are stored in TFile,
	// so we can return that without consulting file_stat.
	s->st_mtime = file->mtime;
	s->st_size  = file->size;
	s->st_mode  = S_IFREG;

/*
	const char* z_fn = v_fn;	// Zip is case-insensitive as well
	if(file->in_archive)
		CHECK_ERR(zip_stat(loc->archive, z_fn, s));
	else
	{
		char p_fn[PATH_MAX];
		CHECK_ERR(make_file_path(v_fn, loc, p_fn));
		CHECK_ERR(file_stat(p_fn, s));
	}
*/
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



static int VFile_reload(VFile* vf, const char* v_fn, Handle)
{
	uint& flags = vf_flags(vf);
		// note: no matter if flags are assigned and the function later
		// fails: the Handle will be closed anyway.

	// we're done if file is already open. need to check this because
	// reload order (e.g. if resource opens a file) is unspecified.
	if(flags & VF_OPEN)
		return 0;

	file_listing_add(v_fn);

	TFile* file;
	uint lf = (flags & FILE_WRITE)? LF_CREATE_MISSING : LF_DEFAULT;
	int err = tree_lookup(v_fn, &file, lf);
	if(err < 0)
	{
		// don't CHECK_ERR - this happens often and the dialog is annoying
		debug_out("tree_lookup failed for %s\n", v_fn);
		return err;
	}

	if(file->in_archive)
	{
		if(flags & FILE_WRITE)
		{
			debug_warn("requesting write access to file in archive");
			return -1;
		}

		CHECK_ERR(zip_open(file->loc->archive, v_fn, &vf->zf));

		flags |= VF_ZIP;
	}
	// normal file
	else
	{
		char p_fn[PATH_MAX];
		CHECK_ERR(make_file_path(v_fn, file->loc, p_fn));
		CHECK_ERR(file_open(p_fn, flags, &vf->f));
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
