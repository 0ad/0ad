#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "posix.h"
#include "unzip.h"
#include "misc.h"
#include "vfs.h"
#include "mem.h"


struct PATH
{
	Handle zip;
	const char* path;
	struct PATH* next;
};
static PATH* path_list;



int vfs_goto_app_dir(char* argv0)
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

	return chdir(path);
}


// TODO: check CD drives?
int vfs_add_search_path(const char* p)
{
	// copy path string - need to append .zip / save a copy if dir
	const size_t path_len = strlen(p);
	char* path = (char*)mem_alloc(path_len+4+1);
	if(!p)
		return -1;
	strcpy(path, p);

	// ZIP file?
	strcpy(path+path_len, ".zip");	// append zip extension
	Handle zip = zopen(path);

	// dir?
	path[path_len] = 0;	// remove .zip extension
	DIR* dir = opendir(path);
	if(dir)
		closedir(dir);
	else
		mem_free(path);

	// neither
	if(!zip && !dir)
		return -1;

	PATH* entry = (PATH*)mem_alloc(sizeof(PATH));
	if(!entry)
		return -1;
	entry->next = path_list;
	path_list = entry;
	entry->zip = zip;
	entry->path = dir? path : 0;

	return 0;
}


int vfs_remove_first_path()
{
	PATH* const p = path_list;
	if(!p)
		return -1;

	mem_free((void*)p->path);
	path_list = p->next;
	zclose(p->zip);
	mem_free(p);

	return 0;
}


static void vfile_dtor(HDATA* hd)
{
}


Handle vfs_open(const char* fn)
{
	char path[PATH_MAX+1]; path[PATH_MAX] = 0;

	// for each search path:
	for(PATH* entry = path_list; entry; entry = entry->next)
	{
		// dir - memory map the file
		{
		if(!entry->path)
			goto not_dir;
		struct stat stat_buf;
		snprintf(path, PATH_MAX, "%s%c%s", entry->path, DIR_SEP, fn);
		if(stat(path, &stat_buf) != 0)
			goto not_dir;
		int fd = open(path, O_RDONLY);
		if(fd < 0)
			goto not_dir;
 		size_t size = stat_buf.st_size;
		void* p = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
		close(fd);
		if(p != MAP_FAILED)
		{
			HDATA* hd;
			Handle h = h_alloc(0, RES_VFILE, vfile_dtor, hd);
			if(h && hd)
			{
				hd->p = p;
				hd->size = size;
				return h;
			}
		}
		}
not_dir:

		// archive
		if(entry->zip)
			return entry->zip;
	}

	// not found
	return 0;
}


int vfs_read(Handle h, void*& p, size_t& size, size_t ofs)
{
	p = 0;
	size = 0;

	HDATA* hd = h_data(h, RES_VFILE);
	if(hd)
	{
		p = (u8*)hd->p + ofs;
		if(!size || size > hd->size)
			size = hd->size;
		return 0;
	}
//	else
//		return zread(h, fn, p, size, ofs);
return -1;
}


int vfs_close(Handle h)
{
	h_free(h, RES_ZIP);
	h_free(h, RES_VFILE);
	return 0;
}
