#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include <string>
#include <vector>
#include <algorithm>

#include "posix.h"
#include "unzip.h"
#include "misc.h"
#include "vfs.h"
#include "mem.h"


// We need to be able to unmount specific paths (e.g. when switching mods).
// Don't want to remount everything (slow), or specify a mod tag when mounting
// (not this module's job). Instead, we include all archives in one path entry;
// the game keeps track of what paths it mounted for a mod, and unmounts those
// when needed.

struct PATH
{
	char* dir;	// relative to root; points to space at end of this struct
	struct PATH* next;
	int num_archives;
	Handle archives[1];
	// space allocated here for archive Handles + dir string
};
static PATH* path_list;


static void vfile_dtor(HDATA* hd)
{
	VFILE* vf = (VFILE*)hd->internal;

	// normal file
	if(vf->fd != -1)
	{
		munmap(hd->p, hd->size);
		hd->p = 0;
		hd->size = 0;

		close(vf->fd);
		vf->fd = -1;
	}

	// in archive
	if(vf->ha && vf->hz)
	{
		zclose(vf->hz);
		zclose(vf->ha);
		vf->ha = vf->hz = 0;
	}
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
	const int num_archives = archives.size();

	// alloc search path entry (add to front)
	const int archives_size = num_archives*sizeof(Handle);
	const int entry_size = round_up(sizeof(PATH)+archives_size+path_len+1, 8);
	PATH* entry = (PATH*)mem_alloc(entry_size);
	if(!entry)
		return -1;
	entry->next = path_list;
	path_list = entry;

	entry->dir = (char*)&entry->archives[0] + archives_size;
	strcpy(entry->dir, path);

	// add archives in alphabetical order
	std::sort(archives.begin(), archives.end());
	entry->num_archives = num_archives;
	for(int i = 0; i < num_archives; i++)
		entry->archives[i] = zopen(archives[i].c_str());

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
			for(int i = 0; i < entry->num_archives; i++)
				zclose(entry->archives[i]);

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


Handle vfs_map(const char* fn)
{
	struct stat stat_buf;
	if(stat(fn, &stat_buf) != 0)
		return 0;
 	size_t size = stat_buf.st_size;

	int fd = open(fn, O_RDONLY);
	if(fd < 0)
		return 0; 

	u32 fn_hash = fnv_hash(fn, strlen(fn));

	void* p = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if(p != MAP_FAILED)
	{
		HDATA* hd;
		Handle h = h_alloc(0, 0, vfile_dtor, &hd);
		if(h)
		{
			hd->p = p;
			hd->size = size;
			return h;
		}
	}

	return 0;
}


Handle vfs_open(const char* fn)
{
	char buf[PATH_MAX+1]; buf[PATH_MAX] = 0;

	// for each search path:
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
		Handle h = vfs_map(path);
		if(h)
			return h;

		// archive
		for(int i = 0; i < entry->num_archives; i++)
		{
			Handle h = zopen(entry->archives[i], fn);
			if(h)
				return h;
		}
	}

	// not found
	return 0;
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
	HDATA* hfd = h_data(hf, 0);
	if(!hfd)
		return -1;
	VFILE* vf = (VFILE*)hfd->internal;

	ssize_t bytes_left = hfd->size - ofs;
	if(bytes_left < 0)
		return -1;

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
		return -1;
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
		cb->aio_buf = buf;
	// allocate our own (reused for subsequent requests)
	else
		if(!cb->aio_buf)
		{
			cb->aio_buf = mem_alloc(64*KB, MEM_HEAP, 64*KB);
			if(!cb->aio_buf)
				return -1;
		}

	// align to 64 KB for speed
	u32 rsize = min(64*KB - (ofs & 0xffff), bytes_left);

	cb->aio_offset = ofs;
	cb->aio_nbytes = rsize;
	aio_read(cb);

	ofs += rsize;
	if(buf)
		(int&)*buf += rsize;

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



int vfs_read(Handle h, void*& p, size_t& size, size_t ofs)
{
	p = 0;
	size = 0;

	HDATA* hd = h_data(h, RES_VFILE);
	if(hd)
	{
		if(ofs+size > hd->size)
			return -1;
		p = (u8*)hd->p + ofs;
		if(!size)
			size = hd->size - ofs;
		return 0;
	}
	// RES_ZFILE
	else
		return zread(h, p, size, ofs);
}


int vfs_close(Handle h)
{
	h_free(h, 0);
	return 0;
}
