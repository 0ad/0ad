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

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "lib.h"
#include "file.h"
#include "zip.h"
#include "misc.h"
#include "vfs.h"
#include "mem.h"

#include <string>
#include <vector>
#include <stack>
#include <algorithm>

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
//   version, that's what they're for).


// rationale for n-archives per PATH entry:
// We need to be able to unmount specific paths (e.g. when switching mods).
// Don't want to remount everything (slow), or specify a mod tag when mounting
// (not this module's job). Instead, we include all archives in one path entry;
// the game keeps track of what path(s) it mounted for a mod,
// and unmounts those when needed.

struct PATH
{
	struct PATH* next;

	// "" if root, otherwise path from root, including DIR_SEP.
	// points to space at end of this struct.
	char* dir;

	size_t num_archives;
	Handle archives[1];

	// space allocated here for archive Handles + dir string
};
static PATH* path_list;





int vfs_set_root(const char* argv0, const char* root)
{
	if(access(argv0, X_OK) < 0)
		return errno;

	char path[PATH_MAX+1];
	path[PATH_MAX] = 0;
	if(!realpath(argv0, path))
		return errno;

	// remove executable name
	char* fn = strrchr(path, DIR_SEP);
	if(!fn)
		return -1;
	*fn = 0;

	chdir(path);
	chdir(root);

	return vfs_mount(".");
}


int vfs_mount(const char* path)
{
	size_t i;

	const size_t path_len = strlen(path);
	if(path_len > VFS_MAX_PATH)
	{
		assert(!"vfs_mount_dir: path name is longer than VFS_MAX_PATH");
		return -1;
	}

	// security check: path must not contain "..",
	// so that it can't access the FS above the VFS root.
	if(strstr(path, ".."))
	{
		assert(0 && "vfs_mount: .. in path string");
		return -1;
	}

	// enumerate all archives in <path>
	std::vector<std::string> archives;
	DIR* dir = opendir(path);
	struct dirent* ent;
	while((ent = readdir(dir)))
	{
		const char* fn = ent->d_name;
		struct stat s;
		if(stat(fn, &s) < 0)
			continue;
		// regular file
		if(s.st_mode & S_IFREG)
		{
			char* ext = strrchr(fn, '.');
			// it's a Zip file - add to list
			if(ext && !strcmp(ext, ".zip"))
				archives.push_back(fn);
		}
	}
	closedir(dir);

	// number of Zip files we'll try to open;
	// final # archives may be less, if opening fails on some of them
	// note: this many are allocated for the PATH entry,
	//       but only successfully opened archives are added
	const size_t num_zip_files = archives.size();

	// alloc search path entry (add to front)
	const size_t archives_size = num_zip_files*sizeof(Handle);
	const size_t dir_size = path_len+1+1;	// DIR_SEP and '\0' appended
	const size_t tot_size = sizeof(PATH)+archives_size+dir_size;
	const size_t entry_size = round_up((long)(tot_size), 32);
	PATH* entry = (PATH*)mem_alloc(entry_size, 32);
	if(!entry)
		return ERR_NO_MEM;
	entry->next = path_list;
	path_list = entry;

	// copy over path string, and convert '/' to platform specific DIR_SEP.
	entry->dir = (char*)&entry->archives[0] + archives_size;
	char* d = entry->dir;
	for(i = 0; i < path_len; i++)
	{
		int c = path[i];
		if(c == '/' || c == '\\' || c == ':')
		{
			assert(c == '/' && "vfs_mount: path string contains platform specific dir separator; use '/'");
			c = DIR_SEP;
		}
		*d++ = c;
	}

	// add trailing DIR_SEP, so we can just append filename when opening.
	// exception: if mounting the current directory "." (i.e. the VFS root),
	// make it "" - guaranteed to be portable and possibly faster.
	if(!strcmp(path, "."))
		entry->dir[0] = '\0';
	else
	{
		d[0] = DIR_SEP;
		d[1] = '\0';
	}

	// add archives in alphabetical order
	std::sort(archives.begin(), archives.end());
	Handle* p = entry->archives;
	for(i = 0; i < num_zip_files; i++)
	{
		const Handle h = zip_archive_open(archives[i].c_str());
		if(h > 0)
			*p++ = h;
	}
	entry->num_archives = p - entry->archives;	// actually valid archives

	return 0;
}


int vfs_umount(const char* path)
{
	PATH** prev = &path_list;
	PATH* entry = path_list;
	while(entry)
	{
		// found 
		if(!strcmp(entry->dir, path))
		{
			// close all archives
			for(size_t i = 0; i < entry->num_archives; i++)
				zip_archive_close(entry->archives[i]);

			// remove from list
			*prev = entry->next;
			mem_free(entry);

			return 0;
		}

		prev = &entry->next;
		entry = entry->next;
	}

	// not found
	return -1;
}


typedef int (*VFS_PATH_CB)(const char* full_rel_path, Handle ha, uintptr_t ctx);

// call cb, passing the ctx argument, for each mounted path or archive.
// if it returns an error (< 0) other than ERR_FILE_NOT_FOUND or succeeds
// (returns 0), return that value; otherwise, continue calling.
// if it never succeeded, fail with ERR_FILE_NOT_FOUND.
// rationale: we want to fail with the correct error value if something
// actually goes wrong (e.g. file locked). callbacks can abort the sequence
// by returning some error value.
static int vfs_foreach_path(VFS_PATH_CB cb, const char* fn, uintptr_t ctx)
{
	char full_rel_path[PATH_MAX+1]; full_rel_path[PATH_MAX] = 0;

	int err;

	for(PATH* entry = path_list; entry; entry = entry->next)
	{
		// dir (already includes DIR_SEP)
		snprintf(full_rel_path, PATH_MAX, "%s%s", entry->dir, fn);
		err = cb(full_rel_path, 0, ctx);
		if(err <= 0 && err != ERR_FILE_NOT_FOUND)
			return err;

		// archive
		for(size_t i = 0; i < entry->num_archives; i++)
		{
			err = cb(fn, entry->archives[i], ctx);
			if(err <= 0 && err != ERR_FILE_NOT_FOUND)
				return err;
		}
	}

	// if we get here, the function always failed with ERR_FILE_NOT_FOUND or
	// requested the next path.
	return ERR_FILE_NOT_FOUND;
}



static int realpath_cb(const char* path, Handle ha, uintptr_t ctx)
{
	char* full_path = (char*)ctx;
	struct stat s;
	int err;

	if(!path && !ha)
	{
		assert(0 && "realpath_cb: called with invalid path and archive handle");
		return 1;
	}

	if(ha)
	{
		err = zip_stat(ha, path, &s);
		if(!err)
		{
			const char* fn = h_filename(ha);
			if(!fn)
			{
				assert(0 && "realpath_cb: h_filename 0, despite successful zip_stat");
				return 1;
			}

			strncpy(full_path, fn, PATH_MAX);
			return 0;
		}
	}
	else
	{
		err = stat(path, &s);
		if(!err)
		{
			strncpy(full_path, path, PATH_MAX);
			return 0;
		}
	}

	// failed *stat above - return error code.
	return err;
}

int vfs_realpath(const char* fn, char* full_path)
{
	return vfs_foreach_path(realpath_cb, fn, (uintptr_t)full_path);
}


static int stat_cb(const char* path, Handle ha, uintptr_t ctx)
{
	struct stat* s = (struct stat*)ctx;

	if(!ha)
		return stat(path, s);
	else
		return zip_stat(ha, path, s);

	assert(0 && "stat_cb: called with invalid path and archive handle");
	return 1;
}

int vfs_stat(const char* fn, struct stat* s)
{
	return vfs_foreach_path(stat_cb, fn, (uintptr_t)s);
}


///////////////////////////////////////////////////////////////////////////////
//
// file
//
///////////////////////////////////////////////////////////////////////////////


enum
{
	VF_OPEN = 1,
	VF_ZIP = 2,
	VF_WRITE = 4
};

struct VFile
{
	int flags;
	size_t size;
		// duplicated in File/ZFile below - oh well.
		// resource data size is fixed anyway.

	// cached contents of file from vfs_load
	// (can't just use pointer - may be freed behind our back)
	Handle hm;

	union
	{
		File f;
		ZFile zf;
	};
};

H_TYPE_DEFINE(VFile)


static void VFile_init(VFile* vf, va_list args)
{
	vf->flags = va_arg(args, int);
}


static void VFile_dtor(VFile* vf)
{
	if(vf->flags & VF_OPEN)
	{
		if(vf->flags & VF_ZIP)
			zip_close(&vf->zf);
		else
			file_close(&vf->f);

		vf->flags &= ~(VF_OPEN);
	}
	

	mem_free_h(vf->hm);
}


// note: can't use the same callback: must check all paths
// for a plain file version before looking in archives.


// called for each mounted path or archive.
static int file_open_cb(const char* path, Handle ha, uintptr_t ctx)
{
	VFile* vf = (VFile*)ctx;

	// not a normal file - ask for next path.
	if(ha != 0)
		return 1;

	int err = file_open(path, vf->flags, &vf->f);
	if(!err)
	{
		vf->size = vf->f.size;
			// somewhat of a hack.. but returning size from file_open
			// is uglier, and relying on start of VFile / File to
			// overlap is unsafe (#define PARANOIA adds a magic field).
		return 0;	// found it - done
	}

	// failed to open.
	return err;
}


// called for each mounted path or archive.
static int zip_open_cb(const char* path, Handle ha, uintptr_t ctx)
{
	VFile* vf = (VFile*)ctx;

	// file not in archive - ask for next path.
	if(ha == 0)
		return 1;

	int err = zip_open(ha, path, &vf->zf);
	if(!err)
	{
		vf->size = vf->zf.ucsize;
		vf->flags |= VF_ZIP;
		return 0;	// found it - done
	}

	// failed to open
	return err;
}



static int VFile_reload(VFile* vf, const char* fn)
{
	// we're done if file is already open. need to check this because reload order
	// (e.g. if resource opens a file) is unspecified.
	if(vf->flags & VF_OPEN)
		return 0;

	int err = -1;

	// careful! this code is a bit tricky. sorry :P

	// only allow plain-file lookup before looking in Zip archives if
	// opening for writing (we don't support writing to archive), or
	// on dev builds. rationale for disabling in final builds: much faster,
	// and a bit more secure (we can protect archives from modification more
	// easily than the individual files).

	// dev build: always allow plain-file lookup.
	// final: only allow if opening for writing.
	//
	// note: flags have been set by init already
#ifdef FINAL
	if(vf->flags & VF_WRITE)
#endif
		err = vfs_foreach_path(file_open_cb, fn, (uintptr_t)vf);

	// load from Zip iff we didn't successfully open the plain file,
	// and we're not opening for writing.
	if(err < 0 && !(vf->flags & VF_WRITE))
		err = vfs_foreach_path(zip_open_cb, fn, (uintptr_t)vf);

	// failed - return error code
	if(err < 0)
		return err;

	// success
	vf->flags |= VF_OPEN;
	return 0;
}


Handle vfs_open(const char* fn, int flags /* = 0 */)
{
	// security check: path must not include ".."
	// (checking start of string isn't enough)
	if(strstr(fn, ".."))
	{
		assert(0 && "vfs_open: .. in path string");
		return 0;
	}

	return h_alloc(H_VFile, fn, 0, flags);
		// pass file flags to init
}


inline int vfs_close(Handle& h)
{
	return h_free(h, H_VFile);
}



// we return a ZFile handle for files in an archive, instead of making the
// handle a member of File, so we don't open 2 handles per file. there's too
// much internal Zip state to store here - we want to keep it encapsulated.
// note: since handle types are private to each module, each function has to
// give the Zip version a crack at its handle, and only continue if it fails.
// when adding file types, their state should go in File, or else the
// 'is this our handle' checks would get unwieldy.


	

ssize_t vfs_io(Handle hf, size_t ofs, size_t size, void*& p)
{
	H_DEREF(hf, VFile, vf);

	// (vfs_open makes sure it's not opened for writing if zip)
	if(vf->flags & VF_ZIP)
		return zip_read(&vf->zf, ofs, size, p);

	// normal file:
	// let file_io alloc the buffer if the caller didn't,
	// because it knows about alignment / padding requirements
	return file_io(&vf->f, ofs, size, &p);
}


Handle vfs_load(const char* fn, void*& p, size_t& size)
{
	p = 0;		// vfs_io needs initial 0 value
	size = 0;

	Handle hf = vfs_open(fn);
	if(hf <= 0)
		return hf;	// error code
	H_DEREF(hf, VFile, vf);

	Handle hm = 0;

	// already read into mem - return existing mem handle
	// TODO: what if mapped?
	if(vf->hm > 0)
	{
		p = mem_get_ptr(vf->hm, &size);
		if(p)
		{
			assert(vf->size == size && "vfs_load: mismatch between File and Mem size");
			hm = vf->hm;
			goto skip_read;
		}
		else
			assert(0 && "vfs_load: invalid MEM attached to vfile (0 pointer)");
			// happens if someone frees the pointer. not an error!
	}

	size = vf->size;
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





 



// separate mem and mmap handles

//mmap used rarely, don't need transparency
//mmap needs filename (so it's invalidated when file changes), passed to h_alloc
//also uses reload and dtor - don't want to stretch mem too far (doesn't belong there)


struct MMap
{
	void* p;
	size_t size;
	File f;
};

H_TYPE_DEFINE(MMap)


static void MMap_init(MMap* m, va_list args)
{
}


void MMap_dtor(MMap* m)
{
//	munmap(m->p, m->size);
//	vfs_close(m->hf);
}

/*

// get pointer to archive in memory
Handle ham;
if(0)
//			if(zip_archive_info(0, 0, &ham) == 0)
{
void* archive_p;
size_t archive_size;
archive_p = mem_get_ptr(ham, &archive_size);

// return file's pos in mapping
assert(ofs < archive_size && "vfs_load: vfile.ofs exceeds Zip archive size");
_p = (char*)archive_p + ofs;
_size = out_size;
hm = mem_assign(_p, _size);
goto done;
}
}
*/

int MMap_reload(MMap* m, const char* fn)
{
	/*
	Handle hf = vfs_open(fn);
	if(!hf)
	return -1;
	File* vf = H_USER_DATA(hf, File);
	Handle hf2;
	size_t ofs;
	#if 0
	if(vf->hz)
	if(zip_get_file(vf->hz, hf2, ofs) < 0)
	return -1;
	#endif
	void* p = mmap(0, (uint)vf->size, PROT_READ, MAP_PRIVATE, vf->fd, 0);
	if(!p)
	{
	vfs_close(hf);
	return -1;
	}

	m->p = p;
	m->size = vf->size;
	*/
	return 0;
}


Handle vfs_map(const char* fn, int flags, void*& p, size_t& size)
{
	Handle hf = vfs_open(fn, flags);
	H_DEREF(hf, VFile, vf);
	int err = file_map(&vf->f, p, size);
	if(err < 0)
		return err;
MEM_DTOR dtor = 0;
uintptr_t ctx = 0;
	return mem_assign(p, size, 0, dtor, ctx);
}


int vfs_unmap(Handle& hm)
{
	return h_free(hm, H_MMap);
}

