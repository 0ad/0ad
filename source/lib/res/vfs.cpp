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
#include "file.h"
#include "zip.h"
#include "misc.h"
#include "vfs.h"
#include "mem.h"
#include "adts.h"


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
				goto fail;
			}
			last_was_dot = true;
		}
		else
			last_was_dot = false;

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

	std::string dir;

	uint pri;

	Loc() {}
	Loc(Handle _archive, const char* _dir, uint _pri)
		: archive(_archive), dir(_dir), pri(_pri) {}
};


struct VDir;

typedef std::map<std::string, VDir*> SubDirs;
typedef SubDirs::iterator SubDirIt;

typedef std::map<std::string, const Loc*> Files;
typedef Files::iterator FileIt;
	// note: priority is accessed by following the Loc pointer.
	// keeping a copy in the map would lead to better cache coherency,
	// but it's a bit more clumsy (map filename to struct {pri, Loc*}).
	// revisit if file lookup open is too slow (unlikely).

struct VDir
{
	std::string v_name;

	void* watch;


	int file_add(const char* const fn, const uint pri, const Loc* const loc)
	{
		std::string _fn(fn);

		typedef std::pair<std::string, const Loc*> Ent;
		Ent ent = std::make_pair(_fn, loc);
		std::pair<FileIt, bool> ret;
		ret = files.insert(ent);
		// file already in dir
		if(!ret.second)
		{
			FileIt it = ret.first;
			const Loc*& old_loc = it->second;

			// new Loc is of higher priority; replace pointer
			if(old_loc->pri <= loc->pri)
			{
				old_loc = loc;
				return 0;
			}
			// new Loc is of lower priority; keep old pointer
			else
				return 1;
		}

		return 0;
	}

	const Loc* file_find(const char* fn)
	{
		std::string _fn(fn);
		FileIt it = files.find(_fn);
		if(it == files.end())
			return 0;
		return it->second;
	}

	VDir* subdir_add(const char* name)
	{
		VDir* vdir = new VDir;
		const std::string _name(name);
		vdir->v_name = _name;

		std::pair<std::string, VDir*> item = std::make_pair(_name, vdir);
		std::pair<SubDirIt, bool> res;
		res = subdirs.insert(item);
		// already in container
		if(!res.second)
			debug_warn("already in subdir");

		SubDirIt it = res.first;
		return it->second;
	}

	VDir* subdir_find(const char* name)
	{
		const std::string _name(name);
		SubDirIt it = subdirs.find(_name);
		if(it == subdirs.end())
			return 0;
		return it->second;
	}

	void subdir_clear()
	{
		for(SubDirIt it = subdirs.begin(); it != subdirs.end(); ++it)
			delete(it->second);
		subdirs.clear();
	}

	friend void tree_clearR(VDir*);

	SubDirs subdirs;	// can't make private; needed for iterator
	Files files;


private:;


};


static VDir vfs_root;


enum LookupFlags
{
	LF_DEFAULT                   = 0,
	LF_CREATE_MISSING_COMPONENTS = 1
};


// starts in VFS root directory (path = "").
// path doesn't need to, and shouldn't, start with '/'.
static int tree_lookup(const char* path, const Loc** const loc = 0, VDir** const dir = 0, LookupFlags flags = LF_DEFAULT)
{
	CHECK_PATH(path);

	// copy into (writeable) buffer so we can 'tokenize' path components
	// by replacing '/' with '\0'.
	// note: CHECK_PATH does length checking
	char buf[VFS_MAX_PATH];
	strcpy(buf, path);
	const char* cur_component = buf;

	const bool create_missing_components = flags & LF_CREATE_MISSING_COMPONENTS;

	VDir* cur_dir = &vfs_root;

	for(;;)
	{
		char* slash = strchr(cur_component, '/');
		// we have followed all path components.
		// cur_component is the filename or ""
		if(!slash)
		{
			// caller wants pointer to file location returned
			if(loc)
			{
				const char* fn = cur_component;
				*loc = cur_dir->file_find(fn);

				// user wanted its loc, but it's not found - fail
				if(!*loc)
					return ERR_FILE_NOT_FOUND;
			}
			// caller wants pointer to this dir returned
			if(dir)
				*dir = cur_dir;
			return 0;
		}
		// cur_component is a subdirectory name; change to it
		else
		{
			const char* subdir_name = cur_component;
			*slash = 0;

			VDir* subdir = cur_dir->subdir_find(subdir_name);
			if(!subdir)
			{
				if(create_missing_components)
					subdir = cur_dir->subdir_add(subdir_name);
				else
					return ERR_PATH_NOT_FOUND;
			}

			cur_dir = subdir;
			cur_component = slash+1;
		}
	}
}


static void tree_clearR(VDir* const dir)
{
	SubDirIt it;
	for(it = dir->subdirs.begin(); it != dir->subdirs.end(); ++it)
	{
		VDir* subdir = it->second;
		tree_clearR(subdir);
	}

	dir->files.clear();
	dir->subdir_clear();
}


static inline void tree_clear()
{
	tree_clearR(&vfs_root);
}


struct FileCBParams
{
	VDir* dir;
	const Loc* loc;
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
	VDir* const cur_dir      = params->dir;
	const Loc* const cur_loc = params->loc;

	// directory
	if(flags & LOC_DIR)
		cur_dir->subdir_add(fn);
	// file
	else
		CHECK_ERR(cur_dir->file_add(fn, cur_loc->pri, cur_loc));

	return 0;
}


static int tree_add_dirR(VDir* const vdir, const char* const f_path, const Loc* const loc)
{
	CHECK_PATH(f_path);

	// add watch
	if(!vdir->watch)
		vdir->watch = 0;

	// add files and subdirs to vdir
	const FileCBParams params = { vdir, loc };
	file_enum(f_path, add_dirent_cb, (uintptr_t)&params);

	for(SubDirIt it = vdir->subdirs.begin(); it != vdir->subdirs.end(); ++it)
	{
		VDir* const vsubdir = it->second;

		char f_subdir_path[VFS_MAX_PATH];
		const char* const v_subdir_name_c = vsubdir->v_name.c_str();
		CHECK_ERR(path_append(f_subdir_path, f_path, v_subdir_name_c));

		tree_add_dirR(vsubdir, f_subdir_path, loc);
	}

	return 0;
}


static int tree_add_loc(VDir* const vdir, const Loc* const loc)
{
	if(loc->archive > 0)
	{
		FileCBParams params = { vdir, loc };
		return zip_enum(loc->archive, add_dirent_cb, (uintptr_t)&params);
	}
	else
	{
		const char* f_path_c = loc->dir.c_str();
		return tree_add_dirR(vdir, f_path_c, loc);
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
typedef std::list<Loc> Locs;
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
	// the VFS tree only holds pointers to Loc, which is why the
	// Locs container must not invalidate its contents after adding,
	// and also why the VFS tree must be rebuilt after unmounting something.
	Loc dir_loc;
	Locs archive_locs;
		// if not is_single_archive, contains one Loc for every archive
		// in the directory (but not its children - see remount()).
		// otherwise, contains exactly one Loc for the single archive.

	// is f_name an archive filename? if not, it's a directory.
	bool is_single_archive;

	Mount() {}
	Mount(const char* _v_path, const char* _f_name, uint _pri)
		: v_path(_v_path), f_name(_f_name), pri(_pri),
		dir_loc(0, "", 0), archive_locs(), is_single_archive(false)
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

	// will add one Loc to this container for
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
		archive_locs->push_back(Loc(archive, "", pri));

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
	Loc& dir_loc             = m.dir_loc;
	Locs& archive_locs       = m.archive_locs;

	VDir* vdir;
	CHECK_ERR(tree_lookup(v_path, 0, &vdir, LF_CREATE_MISSING_COMPONENTS));

	// check if target is a single Zip archive
	// order doesn't matter; can't have both an archive and dir
	const Handle archive = zip_archive_open(f_name);
	if(archive > 0)
	{
		m.is_single_archive = true;
		archive_locs.push_back(Loc(archive, "", pri));
		const Loc* loc = &archive_locs.front();
		return tree_add_loc(vdir, loc);
	}

	// enumerate all archives
	ArchiveCBParams params = { f_name, pri, &archive_locs };
	file_enum(f_name, archive_cb, (uintptr_t)&params);

	for(LocIt it = archive_locs.begin(); it != archive_locs.end(); ++it)
	{
		const Loc* const loc = &*it;
		tree_add_loc(vdir, loc);
	}


	dir_loc.dir = f_name;
	err = tree_add_loc(vdir, &dir_loc);
	if(err < 0)
		err = err;

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


static void cleanup(void)
{
	tree_clear();
	unmount_all();
}


int vfs_mount(const char* const vfs_mount_point, const char* const name, const uint pri)
{
	ONCE(atexit2(cleanup));

	// make sure it's not already mounted, i.e. in mounts
	for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		if(it->f_name == name)
		{
			debug_warn("vfs_mount: already mounted");
			return -1;
		}

	mounts.push_back(Mount(vfs_mount_point, name, pri));

	// actually mount the entry
	Mount& m = mounts.back();
	return remount(m);
}


int vfs_rebuild()
{
	tree_clear();

	unmount_all();
	remount_all();
	return 0;
}


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
//
//
///////////////////////////////////////////////////////////////////////////////

// OLD
// rationale for n-archives per PATH entry:
// We need to be able to unmount specific paths (e.g. when switching mods).
// Don't want to remount everything (slow), or specify a mod tag when mounting
// (not this module's job). Instead, we include all archives in one path entry;
// the game keeps track of what path(s) it mounted for a mod,
// and unmounts those when needed.


int vfs_realpath(const char* fn, char* full_path)
{
	const Loc* loc;
	CHECK_ERR(tree_lookup(fn, &loc));

	if(loc->archive > 0)
	{
		const char* archive_fn = h_filename(loc->archive);
		if(!archive_fn)
			return -1;
		strncpy(full_path, archive_fn, PATH_MAX);
	}
	else
	{
		strncpy(full_path, loc->dir.c_str(), PATH_MAX);
	}

	return 0;
}


int vfs_stat(const char* fn, struct stat* s)
{
	const Loc* loc;
	CHECK_ERR(tree_lookup(fn, &loc));

	if(loc->archive > 0)
		return zip_stat(loc->archive, fn, s);
	else
	{
		const char* dir = loc->dir.c_str(); 
		return file_stat(dir, s);
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


static size_t& vf_size(VFile* vf)
{
	assert(offsetof(struct File, size) == offsetof(struct ZFile, ucsize));
	return vf->f.size;
}


static int& vf_flags(VFile* vf)
{
	assert(offsetof(struct File, flags) == offsetof(struct ZFile, flags));
	return vf->f.flags;
}



static void VFile_init(VFile* vf, va_list args)
{
	int flags = va_arg(args, int);
	vf_flags(vf) = flags;
}


static void VFile_dtor(VFile* vf)
{
	int& flags = vf_flags(vf);

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
	int& flags = vf_flags(vf);

	// we're done if file is already open. need to check this because reload order
	// (e.g. if resource opens a file) is unspecified.
	if(flags & VF_OPEN)
		return 0;

	int err = -1;


	const Loc* loc;
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


Handle vfs_open(const char* fn, uint flags /* = 0 */)
{
	Handle h = h_alloc(H_VFile, fn, 0, flags);
		// pass file flags to init

#ifdef PARANOIA
debug_out("vfs_open fn=%s %I64x\n", fn, h);
#endif

	return h;
}


inline int vfs_close(Handle& h)
{
#ifdef PARANOIA
debug_out("vfs_close %I64x\n", h);
#endif

	return h_free(h, H_VFile);
}


ssize_t vfs_io(Handle hf, size_t ofs, size_t size, void*& p)
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


Handle vfs_load(const char* fn, void*& p, size_t& size)
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


int vfs_store(const char* fn, void* p, size_t size)
{
	Handle hf = vfs_open(fn, VFS_WRITE);
	if(hf <= 0)
		return (int)hf;	// error code
	H_DEREF(hf, VFile, vf);
	int ret = vfs_io(hf, 0, size, p);
	vfs_close(hf);
	return ret;
}




Handle vfs_map(const char* fn, uint flags, void*& p, size_t& size)
{
	Handle hf = vfs_open(fn, flags);
	H_DEREF(hf, VFile, vf);
	CHECK_ERR(file_map(&vf->f, p, size));
MEM_DTOR dtor = 0;
uintptr_t ctx = 0;
	return mem_assign(p, size, 0, dtor, ctx);
}


int vfs_unmap(Handle& hm)
{
	return -1;
//	return h_free(hm, H_MMap);
}

