// virtual file system - transparent access to files in archives;
// allows multiple search paths
//
// Copyright (c) 2003 Jan Wassenberg
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


// currently not thread safe, but that will most likely change
// (if prefetch thread is to be used).
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
// portable (/ as directory separator; no ':' or '\\')
// v_* : VFS
// f_* : no path at all, filename only


static int path_append(char* dst, const char* path, const char* path2)
{
	const size_t path_len = strlen(path);
	const size_t path2_len = strlen(path2);

	if(path_len+path2_len+1 > VFS_MAX_PATH)
		return -1;

	char* p = dst;

	strcpy(p, path);
	p += path_len;
	if(path_len > 0 && p[-1] != '/')
		*p++ = '/';
	strcpy(p, path2);
	return 0;
}


static int path_validate(const uint line, const char* const path)
{
	size_t path_len = 0;

	const char* msg = 0;	// error occurred <==> != 0
	int err = -1;			// pass error code to caller

	// disallow absolute path
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

typedef std::map<std::string, Loc*> Files;
typedef Files::iterator FileIt;
	// note: priority is accessed by following the Loc pointer.
	// keeping a copy in the map would lead to better cache coherency,
	// but it's a bit more clumsy (map filename to struct {pri, Loc*}).
	// revisit if file lookup open is too slow (unlikely).

struct VDir
{
	std::string v_name;

	void* watch;


	int file_add(const char* const fn, const uint pri, Loc* const loc)
	{
		std::string _fn(fn);

		typedef std::pair<std::string, Loc*> Ent;
		Ent ent = std::make_pair(_fn, loc);
		std::pair<FileIt, bool> ret;
		ret = files.insert(ent);
		// file already in dir
		if(!ret.second)
		{
			FileIt it = ret.first;
			Loc*& old_loc = it->second;

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

	Loc* file_find(const char* fn)
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
		std::string _name(name);
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

	VDir* subdir_find(const char* fn)
	{
		std::string _fn(fn);
		SubDirIt it = subdirs.find(_fn);
		if(it == subdirs.end())
			return 0;
		return it->second;
	}

	friend void tree_clearR(VDir*);

	SubDirs subdirs;	// can't make private; needed for iterator
	Files files;


private:;


};


static VDir vfs_root;


enum LookupFlags
{
	LF_DEFAULT,
	LF_CREATE_MISSING_COMPONENTS = 1
};

static int tree_lookup(const char* vfs_path, Loc** loc = 0, VDir** dir = 0, LookupFlags flags = LF_DEFAULT)
{
	CHECK_PATH(vfs_path);

	// copy into (writeable) buffer so we can 'tokenize' path components
	// by replacing '/' with '\0'.
	// note: CHECK_PATH does length checking
	char buf[VFS_MAX_PATH];
	strcpy(buf, vfs_path);
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
	dir->subdirs.clear();
}


static inline void tree_clear()
{
	tree_clearR(&vfs_root);
}








struct FileCBParams
{
	VDir* dir;
	Loc* loc;
};

	// somewhat of a hack. which archives are mounted into the VFS is stored
	// in an Archives list in the Mount struct; they don't have anything to
	// do with a VFS dir. we want to enumerate the archives in a dir via the
	// normal populate(), though, so have to pass this to its callback.



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
static int add_dirent_cb(const char* fn, uint flags, ssize_t size, uintptr_t user)
{
	FileCBParams* params = (FileCBParams*)user;
	VDir* cur_dir      = params->dir;
	Loc* cur_loc   = params->loc;

	// directory
	if(flags & LOC_DIR)
		cur_dir->subdir_add(fn);
	// file
	else
		cur_dir->file_add(fn, cur_loc->pri, cur_loc);

	return 0;
}


static int tree_add_dirR(VDir* vdir, const char* dir, Loc* loc)
{
	// add watch
	if(!vdir->watch)
		vdir->watch = 0;

	// add files and subdirs to dir
	FileCBParams params = { vdir, loc };
	file_enum(dir, add_dirent_cb, (uintptr_t)&params);

	for(SubDirIt it = vdir->subdirs.begin(); it != vdir->subdirs.end(); ++it)
	{
		VDir* subdir = it->second;

		char v_subdir_path[PATH_MAX];
		const char* v_subdir_name_c = subdir->v_name.c_str();
		CHECK_ERR(path_append(v_subdir_path, dir, v_subdir_name_c));

		tree_add_dirR(subdir, v_subdir_path, loc);
	}

	return 0;
}


static int tree_add_loc(VDir* vdir, Loc* loc)
{
	const char* dir = loc->dir.c_str();

	FileCBParams params = { vdir, loc };

	if(loc->archive > 0)
		return zip_enum(loc->archive, add_dirent_cb, (uintptr_t)&params);
	else
	{
		CHECK_PATH(dir);
		return tree_add_dirR(vdir, dir, loc);
	}
}


///////////////////////////////////////////////////////////////////////////////
//
// mount archives and directories into the VFS
//
///////////////////////////////////////////////////////////////////////////////



typedef std::vector<Loc> Locs;
typedef Locs::iterator LocIt;

struct Mount
{
	std::string vfs_mount_point;
	std::string name;

	uint pri;

	Loc loc;
	Locs archive_locs;

	Mount() {}
	Mount(const char* _vfs_mount_point, const char* _name, uint _pri)
		: vfs_mount_point(_vfs_mount_point), name(_name), pri(_pri) {}
};

typedef std::vector<Mount> Mounts;
typedef Mounts::iterator MountIt;
static Mounts mounts;




// called for each OS dir ent.
// add each archive to list.
static int archive_cb(const char* fn, uint flags, ssize_t size, uintptr_t user)
{
	Locs* archive_locs = (Locs*)user;
	// only add to list; don't enumerate its files yet for easier debugging
	// (we see which files are in a dir / archives)
	// also somewhat faster, due to better locality.
	//
	// don't check filename extension - archives won't necessarily
	// be called .zip (example: Quake III .pk3).
	// just try to open the file.
	const Handle archive = zip_archive_open(fn);
	if(archive > 0)
		archive_locs->push_back(Loc(archive, "", 0));

/// HACK HACK HACK pass along pri


	// tree_add_loc them here?

	return 0;
}



// actually mount the specified entry (either Zip archive or dir).
// split out of vfs_mount because we need to mount without changing the
// mount list, when invalidating (reloading) the VFS.
static int remount(Mount& m)
{
	int err;

	const char* vfs_mount_point = m.vfs_mount_point.c_str();
	const char* name = m.name.c_str();
	const uint pri = m.pri;

	VDir* vdir;
	CHECK_ERR(tree_lookup(vfs_mount_point, 0, &vdir, LF_CREATE_MISSING_COMPONENTS));

	// check if target is a single Zip archive
	// order doesn't matter; can't have both an archive and dir

	const Handle archive = zip_archive_open(name);
	if(archive > 0)
	{
		m.archive_locs.push_back(Loc(archive, "", pri));
		LocIt it = m.archive_locs.end();
		Loc* loc = &*(--it);
		return tree_add_loc(vdir, loc);
	}

	m.loc.dir.assign(m.name);
	err = tree_add_loc(vdir, &m.loc);
	if(err < 0)
		err = err;

	// enumerate all archives
	return file_enum(name, archive_cb, (uintptr_t)&m.archive_locs);
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


static void unmount_all(void)
	{ std::for_each(mounts.begin(), mounts.end(), unmount); }

static void remount_all()
	{ std::for_each(mounts.begin(), mounts.end(), remount); }


int vfs_mount(const char* const vfs_mount_point, const char* const name, const uint pri)
{
	ONCE(atexit(unmount_all));

	MountIt it;

	// make sure it's not already mounted, i.e. in mounts
	for(it = mounts.begin(); it != mounts.end(); ++it)
		if(it->name == name)
		{
			debug_warn("vfs_mount: already mounted");
			return -1;
		}

	mounts.push_back(Mount(vfs_mount_point, name, pri));

	// actually mount the entry
	it = mounts.end();
	Mount& m = *(--it);
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
		if(it->name == name)
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
	Loc* loc;
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
	Loc* loc;
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



static int VFile_reload(VFile* vf, const char* fn)
{
	int& flags = vf_flags(vf);

	// we're done if file is already open. need to check this because reload order
	// (e.g. if resource opens a file) is unspecified.
	if(flags & VF_OPEN)
		return 0;

	int err = -1;


	Loc* loc;
	CHECK_ERR(tree_lookup(fn, &loc));

	if(loc->archive <= 0)
	{
		char path[PATH_MAX];
		const char* dir = loc->dir.c_str();
		CHECK_ERR(path_append(path, dir, fn));
		CHECK_ERR(file_open(path, vf_flags(vf), &vf->f));
	}
	else
	{
		if(flags & VFS_WRITE)
		{
			debug_warn("requesting write access to file in archive");
			return -1;
		}

		CHECK_ERR(zip_open(loc->archive, fn, &vf->zf));

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

debug_out("vfs_open fn=%s %I64x\n", fn, h);
return h;
}


inline int vfs_close(Handle& h)
{
debug_out("vfs_close %I64x\n", h);
	return h_free(h, H_VFile);
}


ssize_t vfs_io(Handle hf, size_t ofs, size_t size, void*& p)
{
debug_out("vfs_io ofs=%d size=%d\n", ofs, size);
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
debug_out("vfs_load fn=%s\n", fn);
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

