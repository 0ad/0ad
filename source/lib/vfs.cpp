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

#include <string>
#include <vector>
#include <algorithm>

#include "posix.h"
#include "zip.h"
#include "misc.h"
#include "vfs.h"
#include "mem.h"


// H_VFILE handle
struct VFILE
{
	int fd;

	size_t size;	// compressed size, if a Zip file

	// Zip only:
	size_t ucsize;
	size_t ofs;

	Handle hm;		// memory handle to the file or archive, if a Zip file
};


// rationale for n-archives per PATH entry:
// We need to be able to unmount specific paths (e.g. when switching mods).
// Don't want to remount everything (slow), or specify a mod tag when mounting
// (not this module's job). Instead, we include all archives in one path entry;
// the game keeps track of what path(s) it mounted for a mod,
// and unmounts those when needed.

struct PATH
{
	struct PATH* next;	// linked list

	char* dir;			// relative to root dir;
						// points to space at end of this struct

	size_t num_archives;
	Handle archives[1];

	// space allocated here for archive Handles + dir string
};
static PATH* path_list;


static void vfile_dtor(void* p)
{
	VFILE* vf = (VFILE*)p;

	if(vf->fd > 0)
	{
		close(vf->fd);
		vf->fd = -1;
	}

	mem_free(vf->hm);
}


int vfs_set_root(const char* argv0, const char* root)
{
	if(access(argv0, X_OK) != 0)
		return -1;

	char path[PATH_MAX+1];
	path[PATH_MAX] = 0;
	if(!realpath(argv0, path))
		return -1;

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
	const size_t path_len = strlen(path);
	if(path_len > VFS_MAX_PATH)
	{
		assert(!"vfs_mount_dir: path name is longer than VFS_MAX_PATH");
		return -1;
	}

	// enumerate all archives in <path>
	std::vector<std::string> archives;
	DIR* dir = opendir(path);
	struct dirent* ent;
	while((ent = readdir(dir)))
	{
		struct stat s;
		if(stat(ent->d_name, &s) < 0)
			continue;
		if(s.st_mode == S_IFREG)	// regular file
			archives.push_back(ent->d_name);
	}
	closedir(dir);
	const size_t num_archives = archives.size();

	// alloc search path entry (add to front)
	const size_t archives_size = num_archives*sizeof(Handle);
	const size_t entry_size = round_up((long)(sizeof(PATH)+archives_size+path_len+1), 32);
	PATH* entry = (PATH*)mem_alloc(entry_size, 32, MEM_HEAP);
	if(!entry)
		return -1;
	entry->next = path_list;
	path_list = entry;

	entry->dir = (char*)&entry->archives[0] + archives_size;
	strcpy(entry->dir, path);

	// add archives in alphabetical order
	std::sort(archives.begin(), archives.end());
	entry->num_archives = num_archives;
	for(size_t i = 0; i < num_archives; i++)
		entry->archives[i] = zip_open(archives[i].c_str());

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
				h_free(entry->archives[i], H_ZARCHIVE);

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


// call func, passing the data argument, for each mounted path
// fail if its return value is < 0, stop if it returns 0
static int vfs_foreach_path(int (*func)(const char* path, Handle ha, void* data), const char* fn, void* data)
{
	char buf[PATH_MAX+1]; buf[PATH_MAX] = 0;

	for(PATH* entry = path_list; entry; entry = entry->next)
	{
		// dir
		const char* path = fn;
		if(entry->dir[0] != '.' || entry->dir[1] != '\0')
		{
			// only prepend dir if not "." (root) - "./" isn't portable
			snprintf(buf, PATH_MAX, "%s/%s", entry->dir, fn);
			path = buf;
		}

		int err = func(path, 0, data);
		if(err <= 0)
			return err;
		
		// archive
		for(size_t i = 0; i < entry->num_archives; i++)
		{
			err = func(path, entry->archives[i], data);
			if(err <= 0)
				return err;
		}
	}

	return -1;	// func never returned 0
}



static int realpath_cb(const char* path, Handle ha, void* data)
{
	char* full_path = (char*)data;
	struct stat s;

	if(!path && !ha)
	{
		assert(0 && "realpath_cb: called with invalid path and archive handle");
		return 1;
	}

	if(path)
	{
		if(!stat(path, &s))
		{
			strncpy(full_path, path, PATH_MAX);
			return 0;
		}
	}
	else if(ha)
	{
		if(!zip_stat(ha, path, &s))
		{
			zip_archive_info(ha, full_path, 0);
			return 0;
		}
	}

	return 1;
}

int vfs_realpath(const char* fn, char* full_path)
{
	return vfs_foreach_path(realpath_cb, fn, full_path);
}


static int stat_cb(const char* path, Handle ha, void* data)
{
	struct stat* s = (struct stat*)data;

	if(path)
		return stat(path, s)? 1 : 0;
	else if(ha)
		return zip_stat(ha, path, s)? 1 : 0;

	assert(0 && "stat_cb: called with invalid path and archive handle");
	return 1;
}

int vfs_stat(const char* fn, struct stat* s)
{
	return vfs_foreach_path(stat_cb, fn, s);
}



static int open_cb(const char* path, Handle ha, void* data)
{
	struct stat s;
	VFILE* vf = (VFILE*)data;

	// normal file
	if(path)
	{
		if(stat(path, &s) < 0)
			return 1;

		int fd = open(path, O_RDONLY);
		if(fd < 0)
			return 1;

		vf->fd   = fd;
		vf->size = s.st_size;
	}
	// from archive
	else if(ha)
	{
		ZFILE* zf = zip_lookup(ha, path);
		if(!zf)
			return 1;

		Handle hm;
		if(zip_archive_info(ha, 0, &hm) < 0)
			return 1;

		vf->ofs    = zf->ofs;
		vf->size   = zf->csize;
		vf->ucsize = zf->ucsize;
		vf->fd     = -1;
		vf->hm     = hm;
	}
	else
	{
		assert(0 && "open_cb: called with invalid path and archive handle");
		return 1;
	}

	return 0;
}

Handle vfs_open(const char* fn)
{
	u32 fn_hash = fnv_hash(fn, strlen(fn));

	VFILE* vf;
	Handle hv = h_alloc(fn_hash, H_VFILE, vfile_dtor, (void**)&vf);
	if(!hv)
		return 0;

	// already open
	if(vf->size)
		return hv;

	if(vfs_foreach_path(open_cb, fn, vf) < 0)
	{
		h_free(hv, H_VFILE);
		return 0;
	}

	return hv;
}



const uint IDX_BITS = 4;
const uint NUM_SLOTS = 1ul << IDX_BITS;
const uint TAG_BITS = 32 - IDX_BITS;
static struct Slot
{
	u32 tag;			// = 0 <==> slot available
	struct aiocb cb;
}
slots[NUM_SLOTS];

u32 vfs_start_read(const Handle hf, size_t& ofs, void** buf)
{
	VFILE* vf = (VFILE*)h_user_data(hf, H_VFILE);
	if(!vf)
		return 0;

	if(ofs >= vf->size)
		return 0;
	size_t bytes_left = vf->size - ofs;

// TODO: thread safety

	// find a free slot
	int i = 0;
	Slot* s = slots;
	for(; i < NUM_SLOTS; i++, s++)
		if(!s->tag)
			break;
	if(i == NUM_SLOTS)
	{
		assert(!"vfs_start_read: too many active reads; increase NUM_SLOTS");
		return 0;
	}

	// mark it in use
	static u32 tag;
	if(++tag == 1ul << TAG_BITS)
	{
		assert(!"vfs_start_read: tag overflow!");
		tag = 1;
	}
	s->tag = tag;

	struct aiocb* cb = &s->cb;

	// use the buffer given (e.g. read directly into output buffer)
	if(buf)
		cb->aio_buf = *buf;
	// allocate our own (reused for subsequent requests)
	else
		if(!cb->aio_buf)
		{
			cb->aio_buf = mem_alloc(64*KB, 64*KB, MEM_HEAP);
			if(!cb->aio_buf)
				return 0;
		}

	// align to 64 KB for speed
	size_t rsize = 64*KB - (ofs & 0xffff);	// min(~, bytes_left) - avoid warning
	if(rsize > bytes_left)
		rsize = bytes_left;

	cb->aio_offset = (off_t)ofs;
	cb->aio_nbytes = rsize;
	aio_read(cb);

	ofs += rsize;
	if(buf)
		(size_t&)*buf += rsize;

	return 0;
}


int vfs_finish_read(const u32 slot, void*& p, size_t& size)
{
	p = 0;
	size = 0;

	const uint idx = slot & (NUM_SLOTS-1);
	const u32 tag = slot >> IDX_BITS;
	Slot* const s = &slots[idx];
	if(s->tag != tag)
	{
		assert(!"vfs_finish_read: invalid slot");
		return -1;
	}

	struct aiocb* cb = &s->cb;	

	// wait for read to complete
	while(aio_error(cb) == -EINPROGRESS)
		aio_suspend(&cb, 1, 0);

	ssize_t bytes_read = aio_return(cb);

	s->tag = 0;		// free this slot

	p = cb->aio_buf;
	size = bytes_read;

	return (bytes_read > 0)? 0 : -1;
}



Handle vfs_load(const char* fn, void*& _p, size_t& _size, bool dont_map)
{
	_p = 0;
	_size = 0;

	Handle hf = vfs_open(fn);
	if(!hf)
		return 0;

	VFILE* vf = (VFILE*)h_user_data(hf, H_VFILE);

	const bool deflated = vf->fd == -1 && vf->size != vf->ucsize;
	const size_t in_size = vf->size;
	const size_t out_size = deflated? vf->ucsize : vf->size;

	// already mapped or read
	if(vf->hm)
	{
		MEM* m = (MEM*)h_user_data(vf->hm, H_MEM);
		if(m)
		{
			assert(out_size == m->size && "vfs_load: mismatch between VFILE and MEM size");

			_p = m->p;
			_size = m->size;
			return vf->hm;
		}
		else
			assert(0 && "vfs_load: invalid MEM attached to VFILE");
	}

	// decide whether to map the file, or read it
	MemType mt = MEM_MAPPED;
	if(deflated || dont_map)
		mt = MEM_POOL;

	// allocate memory / map the file
	Handle hm;
	void* out = mem_alloc(out_size, 64*KB, mt, vf->fd, &hm);
	if(!out)
	{
		vfs_close(hf);
		return 0;
	}

	if(mt == MEM_MAPPED)
	{
		_p = out;
		_size = out_size;
		return vf->hm = hm;
	}

	// now we read the file in 64 KB chunks (double buffered);
	// if in an archive, we inflate while waiting for the next chunk to finish
	u32 slots[2];
	int active_read = 0;

	void* pos = out;	// if not inflating, read directly into output buffer
	size_t ofs = vf->ofs;

	void* ctx;
	if(deflated)
	{
		pos = 0;	// read into separate buffer
		ctx = zip_inflate_start(out, out_size);
	}

	bool first = true;
	bool done = false;

	for(;;)
	{
		// start reading next block
		if(!done)
			slots[active_read] = vfs_start_read(hf, ofs, &pos);

		active_read ^= 1;

		// process block read in previous iteration
		if(!first)
		{
			void* p;
			size_t bytes_read;
			vfs_finish_read(slots[active_read], p, bytes_read);

			// inflate what we read
			if(deflated)
				zip_inflate_process(ctx, p, bytes_read);
		}

		first = false;
		if(done)
			break;
		// one more iteration to process the last pending block
		if(ofs >= in_size)
			done = true;
	}

	if(deflated)
	{
		if(zip_inflate_end(ctx) < 0)
		{
			mem_free(out);
			return 0;
		}
	}

	_p = out;
	_size = out_size;
	return vf->hm = hm;
}


int vfs_close(Handle h)
{
	return h_free(h, H_VFILE);
}