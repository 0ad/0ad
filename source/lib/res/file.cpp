// file layer on top of POSIX.
// provides streaming support and caching.
//
// Copyright (c) 2004 Jan Wassenberg
//
// This file is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful, but
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
#include "h_mgr.h"
#include "mem.h"
#include "detect.h"
#include "adts.h"

#include <vector>
#include <algorithm>

// block := power-of-two sized chunk of a file.
// all transfers are expanded to naturally aligned, whole blocks
// (this makes caching parts of files feasible; it is also much faster
// for some aio implementations, e.g. wposix).

const size_t BLOCK_SIZE_LOG2 = 16;		// 2**16 = 64 KB
const size_t BLOCK_SIZE = 1ul << BLOCK_SIZE_LOG2;


// rationale for aio, instead of only using mmap:
// - parallel: instead of just waiting for the transfer to complete,
//   other work can be done in the meantime.
//   example: decompressing from a Zip archive is practically free,
//   because we inflate one block while reading the next.
// - throughput: with aio, the drive always has something to do, as opposed
//   to read requests triggered by the OS for mapped files, which come
//   in smaller chunks. this leads to much higher transfer rates.
// - memory: when used with VFS, aio makes better use of a file cache.
//   data is generally compressed in an archive. a cache should store the
//   decompressed and decoded (e.g. TGA color swapping) data; mmap would
//   keep the original, compressed data in memory, which doesn't help.
//   we bypass the OS file cache via aio, and store partial blocks here (*);
//   higher level routines will cache the actual useful data.
//   * requests for part of a block are usually followed by another.




// return the native equivalent of the given portable path
// (i.e. convert all '/' to the platform's directory separator)
// makes sure length < PATH_MAX.
static int mk_native_path(const char* const path, char* const n_path)
{
/*
	// if there's a platform with multiple-character DIR_SEP,
	// scan through the path and add space for each separator found.
	const size_t len = strlen(p_path);

	n_path = (const char*)malloc(len * sizeof(char));
	if(!n_path)
		return ERR_NO_MEM;
*/

	const char* portable = path;
	char* native = (char*)n_path;

	size_t len = 0;

	for(;;)
	{
		len++;
		if(len >= PATH_MAX)
			return -1;

		char c = *portable++;

		if(c == '/')
			c = DIR_SEP;

		*native++ = c;

		if(c == '\0')
			break;
	}

	return 0;
}


// set current directory to rel_path, relative to the path to the executable,
// which is taken from argv0.
//
// example: executable in "$install_dir/system"; desired root dir is
// "$install_dir/data" => rel_path = "../data".
//
// this is necessary because the current directory is unknown at startup
// (e.g. it isn't set when invoked via batch file), and this is the
// easiest portable way to find our install directory.
//
// can only be called once, by design (see below). rel_path is trusted.
int file_rel_chdir(const char* argv0, const char* const rel_path)
{
	const char* msg = 0;

	// security check: only allow attempting to chdir once, so that malicious
	// code cannot circumvent the VFS checks that disallow access to anything
	// above the current directory (set here).
	// this routine is called early at startup, so any subsequent attempts
	// are likely bogus.
	static bool already_attempted;
	if(already_attempted)
	{
		msg = "called more than once";
		goto fail;
	}
	already_attempted = true;

	{

	// Win32-specific: if started via batch file, argv0 might not end
	// with ".exe". access and realpath require the filename with extension,
	// so append it if not there. we don't get the full path either in that
	// case, but realpath takes care of it.
#ifdef _WIN32
	char fixed_argv0[PATH_MAX];
	if(!strchr(argv0, '.'))
	{
		strncpy(fixed_argv0, argv0, PATH_MAX-5);
		strcat(fixed_argv0, ".exe");
		argv0 = fixed_argv0;
	}
#endif

	// get full path to executable
	if(access(argv0, X_OK) < 0)
		goto fail;
	char n_path[PATH_MAX+1];
	n_path[PATH_MAX] = '\0';
	if(!realpath(argv0, n_path))
		goto fail;
	const size_t n_path_len = strlen(n_path);

	// strip executable name and append rel_path
	char* fn = strrchr(n_path, DIR_SEP);
	if(!fn)
	{
		msg = "realpath returned an invalid path?";
		goto fail;
	}
	char* pos = fn+1;	// will overwrite executable name
	CHECK_ERR(mk_native_path(rel_path, pos));

	if(chdir(n_path) < 0)
		goto fail;

	return 0;

	}

fail:
	debug_warn("file_rel_chdir failed");
	if(msg)
	{
		debug_out("file_rel_chdir: %s\n", msg);
printf("%s\n", msg);
		return -1;
	}

	return -errno;
}






// need to store entries returned by readdir so they can be sorted.
struct DirEnt
{
	const std::string name;
	const uint flags;
	const off_t size;

	DirEnt(const char* const _name, const uint _flags, const off_t _size)
		: name(_name), flags(_flags), size(_size) {}
};

// pointer to DirEnt: faster sorting, but more allocs.
typedef std::vector<const DirEnt*> DirEnts;
typedef DirEnts::const_iterator DirEntIt;

static bool dirent_less(const DirEnt* const d1, const DirEnt* const d2)
	{ return d1->name.compare(d2->name) < 0; }

// we give the callback the directory-entry-name only - not the
// absolute path, nor <dir> prepended. rationale: some users don't need it,
// and would need to strip it. there are not enough users requiring it to
// justify that. this routine does actually generate the absolute path
// for use with stat, but in native form - can't use that.
int file_enum(const char* const dir, const FileCB cb, const uintptr_t user)
{
	char n_path[PATH_MAX+1];
	n_path[PATH_MAX] = '\0';
		// will append filename to this, hence "path".
		// 0-terminate simplifies filename strncpy below.
	CHECK_ERR(mk_native_path(dir, n_path));

	// all entries are enumerated (adding to this container),
	// std::sort-ed, then all passed to cb.
	DirEnts dirents;

	int stat_err = 0;
	int cb_err = 0;
	int ret;

	DIR* const os_dir = opendir(n_path);
	if(!os_dir)
		return -1;

	// will append file names here
	const size_t n_path_len = strlen(n_path);
	char* fn_start = n_path + n_path_len;
	*fn_start++ = DIR_SEP;

	struct dirent* os_ent;
	while((os_ent = readdir(os_dir)))
	{
		const char* fn = os_ent->d_name;

		strncpy(fn_start, fn, PATH_MAX-n_path_len);
			// stat needs the full path. this is easier than changing
			// directory every time, and should be fast enough.
			// BTW, direct strcpy is faster than path_append -
			// we save a strlen every iteration.

		// no need to go through file_stat -
		// we already have the native path.
		struct stat s;
		ret = stat(n_path, &s);
		if(ret < 0)
		{
			if(stat_err == 0)
				stat_err = ret;
			continue;
		}

		uint flags = 0;
		off_t size = s.st_size;

		// dir
		if(s.st_mode & S_IFDIR)
		{
			// skip . and ..
			if(fn[0] == '.' && (fn[1] == '\0' || (fn[1] == '.' && fn[2] == '\0')))
				continue;

			flags |= LOC_DIR;
			size = -1;
		}
		// skip if neither dir nor file
		else if(!(s.st_mode & S_IFREG))
			continue;

		const DirEnt* const ent = new DirEnt(fn, flags, size);
		dirents.push_back(ent);
	}
	closedir(os_dir);

	std::sort(dirents.begin(), dirents.end(), dirent_less);

	DirEntIt it;
	for(it = dirents.begin(); it != dirents.end(); ++it)
	{
		const DirEnt* const ent = *it;
		const char* name_c = ent->name.c_str();
		const uint flags   = ent->flags;
		const ssize_t size = ent->size;
		ret = cb(name_c, flags, size, user);
		if(ret < 0)
			if(cb_err == 0)
				cb_err = ret;
	}

	for(it = dirents.begin(); it != dirents.end(); ++it)
		delete *it;

	if(cb_err < 0)
		return cb_err;
	return stat_err;
}


int file_stat(const char* const path, struct stat* const s)
{
	char n_path[PATH_MAX+1];
	CHECK_ERR(mk_native_path(path, n_path));

	return stat(n_path, s);
}


///////////////////////////////////////////////////////////////////////////////
//
// file open/close
// stores information about file (e.g. size) in File struct
//
///////////////////////////////////////////////////////////////////////////////


// interface rationale:
// - this module depends on the handle manager for IO management,
//   but should be useable without the VFS (even if they are designed
//   to work together).
// - allocating a Handle for the file info would solve several problems
//   (see below), but we don't want to allocate 2..3 (VFS, file, Zip file)
//   for every file opened - that'd add up quickly.
//   the Files are always freed at exit though, since they're part of
//   VFile handles in the VFS.
// - we want the VFS open logic to be triggered on file invalidate
//   (if the dev. file is deleted, we should use what's in the archives).
//   we don't want to make this module depend on VFS, so we can't
//   call up into vfs_foreach_path from reload here =>
//   VFS needs to allocate the handle.
// - no problem exposing our internals via File struct -
//   we're only used by the VFS and Zip modules. don't bother making
//   an opaque struct - that'd have to be kept in sync with the real thing.
// - when Zip opens its archives via file_open, a handle isn't needed -
//   the Zip module hides its File struct (required to close the file),
//   and the Handle approach doesn't guard against some idiot calling
//   close(our_fd) directly, either.


// marker for File struct, to make sure it's valid
#ifdef PARANOIA
static const u32 FILE_MAGIC = FOURCC('F','I','L','E');
#endif


static int file_validate(const uint line, File* const f)
{
	const char* msg = "";
	int err = -1;

	if(!f)
	{
		msg = "File* parameter = 0";
		err = ERR_INVALID_PARAM;
	}
#ifdef PARANOIA
	else if(f->magic != FILE_MAGIC)
		msg = "File corrupted (magic field incorrect)";
#endif
	else if(f->fd < 0)
		msg = "File fd invalid (< 0)";
	else if((f->mapping != 0) ^ (f->map_refs != 0))
		msg = "File mapping without refs";
#ifndef NDEBUG
	else if(!f->fn_hash)
		msg = "File fn_hash not set";
#endif
	// everything is OK
	else
		return 0;

	// failed somewhere - err is the error code,
	// or -1 if not set specifically above.
	debug_out("file_validate at line %d failed: %s\n", line, msg);
	debug_warn("file_validate failed");
	return err;
}


#define CHECK_FILE(f)\
do\
{\
	int err = file_validate(__LINE__, f);\
	if(err < 0)\
		return err;\
}\
while(0);


int file_open(const char* const p_fn, const uint flags, File* const f)
{
	memset(f, 0, sizeof(File));

	char n_fn[PATH_MAX];
	CHECK_ERR(mk_native_path(p_fn, n_fn));

	if(!f)
		goto invalid_f;
		// jump to CHECK_FILE post-check, which will handle this.

{
	// don't stat if opening for writing - the file may not exist yet
	off_t size = 0;

	int mode = O_RDONLY;
	if(flags & FILE_WRITE)
		mode = O_WRONLY;
	else
	{
		struct stat s;
		if(stat(n_fn, &s) < 0)
			return -1;
		if(!(s.st_mode & S_IFREG))
			return -1;
		size = s.st_size;
	}

	int fd = open(n_fn, mode);
	if(fd < 0)
		return -1;

#ifdef PARANOIA
	f->magic = FILE_MAGIC;
#endif

	f->flags    = flags;
	f->size     = size;
	f->fn_hash  = fnv_hash(n_fn);		// copy filename instead?
	f->mapping  = 0;
	f->map_refs = 0;
	f->fd       = fd;
}

invalid_f:
	CHECK_FILE(f);

	return 0;
}


int file_close(File* const f)
{
	CHECK_FILE(f);

	// make sure the mapping is actually freed,
	// regardless of how many references remain.
	if(f->map_refs > 1)
		f->map_refs = 1;
	file_unmap(f);

	// (check fd to avoid BoundsChecker warning about invalid close() param)
	if(f->fd != -1)
	{
		close(f->fd);
		f->fd = -1;
	}
	f->size = 0;

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// low level IO
// thin wrapper over aio; no alignment or caching
//
///////////////////////////////////////////////////////////////////////////////


struct ll_cb
{
	aiocb cb;
};


int ll_start_io(File* const f, const off_t ofs, size_t size, void* const p, ll_cb* const lcb)
{
	CHECK_FILE(f);

	if(size == 0)
	{
		debug_warn("ll_start_io: size = 0 - why?");
		return ERR_INVALID_PARAM;
	}

	const int op = (f->flags & FILE_WRITE)? LIO_WRITE : LIO_READ;

	// cut off at EOF.
	// avoid min() due to type conversion warnings.
	const off_t bytes_left = f->size - ofs;
	if(bytes_left < 0)
		return ERR_EOF;
	if((off_t)size > bytes_left)
		size = (size_t)bytes_left;
		// guaranteed to fit, since size was > bytes_left

	aiocb* cb = &lcb->cb;

	// send off async read/write request
	cb->aio_lio_opcode = op;
	cb->aio_buf        = p;
	cb->aio_fildes     = f->fd;
	cb->aio_offset     = (off_t)ofs;
	cb->aio_nbytes     = (size_t)size;
	return lio_listio(LIO_NOWAIT, &cb, 1, (struct sigevent*)0);
		// this just issues the I/O - doesn't wait until complete.
}


// as a convenience, return a pointer to the transfer buffer
// (rather than expose the ll_cb internals)
ssize_t ll_wait_io(ll_cb* const lcb, void*& p)
{
	aiocb* cb = &lcb->cb;

	// wait for transfer to complete.
	while(aio_error(cb) == EINPROGRESS)
		aio_suspend(&cb, 1, NULL);

	// posix has aio_buf as volatile void, and gcc doesn't like to cast it
	// implicitly
	p = (void *)cb->aio_buf;

	// return how much was actually transferred,
	// or -1 if the transfer failed.
	return aio_return(cb);
}


///////////////////////////////////////////////////////////////////////////////
//
// block cache
//
///////////////////////////////////////////////////////////////////////////////


static Cache<void*> c;
	// don't need a Block struct as cache data -
	// it stores its associated ID, and does refcounting
	// with lock().


// create an id for use with the Cache that uniquely identifies
// the block from the file <fn_hash> containing <ofs>.
static u64 block_make_id(const u32 fn_hash, const off_t ofs)
{
	// id format: filename hash | block number
	//            63         32   31         0
	//
	// we assume the hash (currently: FNV) is unique for all filenames.
	// chance of a collision is tiny, and a build tool will ensure
	// filenames in the VFS archives are safe.
	//
	// block_num will always fit in 32 bits (assuming maximum file size
	// = 2^32 * BLOCK_SIZE = 2^48 -- plenty); we check this, but don't
	// include a workaround. we could return 0, and the caller would have
	// to allocate their own buffer, but don't bother.

	// make sure block_num fits in 32 bits
	const size_t block_num = ofs / BLOCK_SIZE;
	assert(block_num <= 0xffffffff);

	u64 id = fn_hash;	// careful, don't shift a u32 32 bits left
	id <<= 32;
	id |= block_num;
	return id;
}


// 
static void* block_alloc(const u64 id)
{
	// initialize cache, if not done already.
	static bool cache_initialized;
	if(!cache_initialized)
	{
		get_mem_status();
		// TODO: calculate size
		const size_t num_blocks = 16;

		// evil: waste some mem (up to one block) to make sure the first block
		// isn't at the start of the allocation, so that users can't
		// mem_free() it. do this by manually aligning the pool.
		//
		// allocator will free the whole thing at exit.
		void* const pool = mem_alloc((num_blocks+1) * BLOCK_SIZE);
		if(!pool)
			return 0;

		const uintptr_t start = round_up((uintptr_t)pool + 1, BLOCK_SIZE);
			// +1 => if already block-aligned, add a whole block!

		// add all blocks to cache
		void* p = (void*)start;
		for(size_t i = 0; i < num_blocks; i++)
		{
			if(c.grow(p) < 0)
				debug_warn("block_alloc: Cache::grow failed!");
				// currently can't fail.
			p = (char*)p + BLOCK_SIZE;
		}

		cache_initialized = true;
	}

	void** const entry = c.assign(id);
	if(!entry)
		return 0;
	void* const block = *entry;

	if(c.lock(id, true) < 0)
		debug_warn("block_alloc: Cache::lock failed!");
		// can't happen: only cause is tag not found, but we successfully
		// added it above. if it did fail, that'd be bad: we leak the block,
		// and/or the buffer may be displaced while in use. hence, assert.
	
	return block;
}


// modifying cached blocks is not supported.
// we could allocate a new buffer and update the cache to point to that,
// but that'd fragment our memory pool.
// instead, add a copy on write call, if necessary.


static int block_retrieve(const u64 id, void*& p)
{
	void** const entry = c.retrieve(id);
	if(entry)
	{
		p = *entry;
		c.lock(id, true);	// add reference
		return 0;
	}
	else
	{
		p = 0;
		return -1;
	}

	// important note:
	// for the purposes of starting the IO, we can regard blocks whose read
	// is still pending it as cached. when getting the IO results,
	// we'll have to wait on the actual IO for that block.
	//
	// don't want to require IOs to be completed in order of issue:
	// that'd mean only 1 caller can read from file at a time.
	// would obviate associating id with IO, but is overly restrictive.
}


static int block_discard(const u64 id)
{
	return c.lock(id, false);
}


int file_free_buf(void*& p)
{
	const uintptr_t _p = (uintptr_t)p;
	void* const actual_p = (void*)(_p - (_p % BLOCK_SIZE));	// round down

	return mem_free(actual_p);

	// remove from cache?

	// check if in use?
}


///////////////////////////////////////////////////////////////////////////////
//
// async I/O
//
///////////////////////////////////////////////////////////////////////////////


struct IO
{
	u64 block_id;
		// transferring via cache (=> BLOCK_SIZE aligned) iff != 0

	void* block;
		// valid because cache line is locked
		// (rug can't be pulled out from under us)

	ll_cb* cb;
		// this is too big to store here. IOs are reused,
		// so we don't allocate a new cb every file_start_io.

	void* user_p;
	off_t user_ofs;
	size_t user_size;

	int cached  : 1;
	int pending : 1;
};

H_TYPE_DEFINE(IO)


// don't support forcibly closing files => don't need to keep track of
// all IOs pending for each file. too much work, little benefit.


static void IO_init(IO* io, va_list args)
{
	const size_t cb_size = round_up(sizeof(struct ll_cb), 16);
	io->cb = (ll_cb*)mem_alloc(cb_size, 16, MEM_ZERO);
}

static void IO_dtor(IO* io)
{
	mem_free(io->cb);
}


// TODO: prevent reload if already open, i.e. IO pending

// we don't support transparent read resume after file invalidation.
// if the file has changed, we'd risk returning inconsistent data.
// i don't think it's possible anyway, without controlling the AIO
// implementation: when we cancel, we can't prevent the app from calling
// aio_result, which would terminate the read.
static int IO_reload(IO* io, const char* fn)
{
	return io->cb? 0 : ERR_NO_MEM;
}


///////////////////////////////////////////////////////////////////////////////


// extra layer on top of h_alloc, so we can reuse IOs
//
// (avoids allocating the cb every IO => less heap fragmentation)
//
// don't worry about reassigning IOs to their associated file -
// they don't need to be reloaded, since the VFS refuses reload
// requests for files with pending IO.

typedef std::vector<Handle> IOList;

// accessed MRU for better cache locality
static IOList free_ios;

// list of all IOs allocated.
// used to find active IO, given id (see below).
// also used to free all IOs before the handle manager
// cleans up at exit, so they aren't seen as resource leaks.

static IOList all_ios;

struct Free
{
	void operator()(Handle h)
	{
		h_free(h, H_IO);
	}
};

// free all allocated IOs, so they aren't seen as resource leaks.
static void io_shutdown(void)
{
	std::for_each(all_ios.begin(), all_ios.end(), Free());
}


static Handle io_alloc()
{
	ONCE(atexit(io_shutdown));
/*
	// grab from freelist
	if(!free_ios.empty())
	{
		Handle h = free_ios.back();
		free_ios.pop_back();

		// note:
		// we don't check if the freelist contains valid handles.
		// that "can't happen", and if it does, it'll be caught
		// by the handle dereference in file_start_io.
		// 
		// no one else can actually free an IO - that would require
		// its handle type, which is private to this module.
		// the free_io call just adds it to the freelist;
		// all allocated IOs are destroyed in io_cleanup at exit.

		return h;
	}
*/
	// allocate a new IO
	Handle h = h_alloc(H_IO, 0);
	// .. it's valid - store in list.
	if(h > 0)
		all_ios.push_back(h);
	return h;
}


static int io_free(const Handle hio)
{
	H_DEREF(hio, IO, io);

	if(io->pending)
	{
		debug_warn("io_free: IO pending");
		return -1;
	}

	// clear the other IO fields, just to be sure.
	// (but don't memset the whole thing - that'd trash the cb pointer!)
	io->block_id  = 0;
	io->block     = 0;
	io->cached    = 0;
	io->user_ofs  = 0;
	io->user_size = 0;
	io->user_p    = 0;
	memset(io->cb, 0, sizeof(ll_cb));

	// TODO: complain if buffer not yet freed?

	// we know hio is valid, since we successfully dereferenced above.
	free_ios.push_back(hio);
	return 0;
}


// need to find IO, given id, to make sure a block
// that is marked cached has actually been read.
// it is expected that there only be a few allocated IOs,
// so it's ok to search this list every cache hit.
// adding to the cache data structure would be messier.

struct FindBlock : public std::binary_function<Handle, u64, bool>
{
	bool operator()(const Handle hio, const u64 block_id) const
	{
		// can't use H_DEREF - we return bool
		IO* io = (IO*)h_user_data(hio, H_IO);
		if(!io)
		{
			debug_warn("invalid handle in all_ios list!");
			return false;
		}
		return io->block_id == block_id;
	}
};

static Handle io_find(const u64 block_id)
{
	IOList::const_iterator it;
	it = std::find_if(all_ios.begin(), all_ios.end(), std::bind2nd(FindBlock(), block_id));
	// not found
	if(it == all_ios.end())
		return 0;

	return *it;
}


///////////////////////////////////////////////////////////////////////////////



// rationale for extra alignment / copy layer, even though aio takes care of it:
// aio would pad to its minimum read alignment, copy over, and be done;
// in our case, if something is unaligned, a request for the remainder of the
// block is likely to follow, so we want to cache the whole block.


// pads the request up to BLOCK_SIZE, and stores the original parameters in IO.
// transfers of more than 1 block (including padding) are allowed, but do not
// go through the cache. don't see any case where that's necessary, though.
Handle file_start_io(File* const f, const off_t user_ofs, size_t user_size, void* const user_p)
{
	int err;

	CHECK_FILE(f);

	if(user_size == 0)
	{
		debug_warn("file_start_io: user_size = 0 - why?");
		return ERR_INVALID_PARAM;
	}

	const int op = (f->flags & FILE_WRITE)? LIO_WRITE : LIO_READ;

	// cut off at EOF.
	// avoid min() due to type conversion warnings.
	const off_t bytes_left = f->size - user_ofs;
	if(bytes_left < 0)
		return ERR_EOF;
	if((off_t)user_size > bytes_left)
		user_size = (size_t)bytes_left;
		// guaranteed to fit, since size was > bytes_left

	u64 block_id = block_make_id(f->fn_hash, user_ofs);
		// reset to 0 if transferring more than 1 block.

	// allocate IO slot
	Handle hio = io_alloc();
	H_DEREF(hio, IO, io);
	io->block_id  = block_id;
	io->user_p    = user_p;
	io->user_ofs  = user_ofs;
	io->user_size = user_size;
		// notes: io->cached, io->pending and io->block are already zeroed;
		// cb will receive the actual IO request (aligned offset and size).

#ifdef PARANOIA
debug_out("file_start_io hio=%I64x ofs=%d size=%d\n", hio, user_ofs, user_size);
#endif

	// aio already safely handles unaligned buffers or offsets.
	// when reading zip files, we don't want to repeat a read
	// if a block contains the end of one file and start of the next
	// (speed concern).
	// therefore, we align and round up to whole blocks.
	//
	// note: cache even if this is the last block before EOF:
	// a zip archive may contain one last file in the block.
	// if not, no loss - the buffer will be LRU, and reused.

	off_t ofs = user_ofs;
	const size_t padding = ofs % BLOCK_SIZE;
	ofs -= (off_t)padding;
	const size_t size = round_up(padding + user_size, BLOCK_SIZE);



	// if already cached, we're done
	if(size == BLOCK_SIZE && block_retrieve(block_id, io->block) == 0)
	{
#ifdef PARANOIA
debug_out("file_start_io: cached! block # = %d\n", block_id & 0xffffffff);
#endif
		io->cached = 1;
		return hio;
	}


	void* buf = 0;
	void* our_buf = 0;

	if(user_p && !padding)
		buf = user_p;
	else
	{
		if(size == BLOCK_SIZE)
			our_buf = io->block = block_alloc(block_id);
		// transferring more than one block - doesn't go through cache!
		else
		{
			our_buf = mem_alloc(size, BLOCK_SIZE);
			block_id = 0;
		}
		if(!our_buf)
		{
			err = ERR_NO_MEM;
			goto fail;
		}

		buf = our_buf;
	}

	err = ll_start_io(f, ofs, size, buf, io->cb);

	if(err < 0)
	{
fail:
		file_discard_io(hio);
		if(size != BLOCK_SIZE)
			file_free_buf(our_buf);
		return err;
	}

	return hio;
}


int file_wait_io(const Handle hio, void*& p, size_t& size)
{
#ifdef PARANOIA
debug_out("file_wait_io: hio=%I64x\n", hio);
#endif

	int ret = 0;

	p = 0;
	size = 0;

	H_DEREF(hio, IO, io);
	ll_cb* cb = io->cb;

	size = io->user_size;

	void* transfer_buf;
	ssize_t bytes_transferred;

	// block's tag is in cache. need to check if its read is still pending.
	if(io->cached)
	{
		Handle cache_hio = io_find(io->block_id);
		// was already finished - don't wait
		if(cache_hio <= 0)
			goto skip_wait;

		// not finished yet; will wait for it below, as with uncached reads.
		H_DEREF(cache_hio, IO, cache_io);
			// can't fail, since io_find has to dereference each handle.
		cb = cache_io->cb;
	}

	bytes_transferred = ll_wait_io(cb, transfer_buf);

skip_wait:

	if(io->block)
	{
		size_t padding = io->user_ofs % BLOCK_SIZE;
		void* src = (char*)io->block + padding;

		// copy over into user's buffer
		if(io->user_p)
		{
			p = io->user_p;
			memcpy(p, src, io->user_size);
		}
		// return pointer to cache block
		else
			p = src;
	}
	// we had read directly into target buffer
	else
		p = transfer_buf;

	return ret;
}


int file_discard_io(Handle& hio)
{
	H_DEREF(hio, IO, io);
	block_discard(io->block_id);
	io_free(hio);
	return 0;
}




// transfer modes:
// *p != 0: *p is the source/destination address for the transfer.
//          (FILE_MEM_READONLY?)
// *p == 0: allocate a buffer, read into it, and return it in *p.
//          when no longer needed, it must be freed via file_free_buf.
//  p == 0: read raw_size bytes from file, starting at offset raw_ofs,
//          into temp buffers; each block read is passed to cb, which is
//          expected to write actual_size bytes total to its output buffer
//          (for which it is responsible).
//          useful for reading compressed data.
//
// return (positive) number of raw bytes transferred if successful;
// otherwise, an error code.
ssize_t file_io(File* const f, const off_t raw_ofs, size_t raw_size, void** const p,
	const FILE_IO_CB cb, const uintptr_t ctx) // optional
{
#ifdef PARANOIA
debug_out("file_io fd=%d size=%d ofs=%d\n", f->fd, raw_size, raw_ofs);
#endif

	CHECK_FILE(f);

	const bool is_write = (f->flags == FILE_WRITE);

	//
	// transfer parameters
	//

	// reading: make sure we don't go beyond EOF
	if(!is_write)
	{
		// cut off at EOF.
		// avoid min() due to type conversion warnings.
		off_t bytes_left = f->size - raw_ofs;
		if(bytes_left < 0)
			return ERR_EOF;
		if((off_t)raw_size > bytes_left)
			raw_size = (size_t)bytes_left;
			// guaranteed to fit, since size was > bytes_left
	}
	// writing: make sure buffer is valid
	else
	{
		// temp buffer OR supposed to be allocated here: invalid
		if(!p || !*p)
		{
			debug_warn("file_io: write to file from 0 buffer");
			return ERR_INVALID_PARAM;
		}
	}

	const size_t misalign = raw_ofs % BLOCK_SIZE;

	// actual transfer start offset
	// not aligned! aio takes care of initial unalignment;
	// next read will be aligned, because we read up to the next block.
	const off_t start_ofs = raw_ofs;


	void* buf = 0;				// I/O source or sink; assume temp buffer
	void* our_buf = 0;			// buffer we allocate, if necessary

	// check buffer param
	// .. temp buffer requested
	if(!p)
		; // nothing to do - buf already initialized to 0
	// .. user specified, or requesting we allocate the buffer
	else
	{
		// the underlying aio implementation likes buffer and offset to be
		// sector-aligned; if not, the transfer goes through an align buffer,
		// and requires an extra memcpy.
		//
		// if the user specifies an unaligned buffer, there's not much we can
		// do - we can't assume the buffer contains padding. therefore,
		// callers should let us allocate the buffer if possible.
		//
		// if ofs misalign = buffer, only the first and last blocks will need
		// to be copied by aio, since we read up to the next block boundary.
		// otherwise, everything will have to be copied; at least we split
		// the read into blocks, so aio's buffer won't have to cover the
		// whole file.

		// user specified buffer
		if(*p)
		{
			buf = *p;

			// warn in debug build if buffer not aligned
#ifndef NDEBUG
			size_t buf_misalign = ((uintptr_t)buf) % BLOCK_SIZE;
			if(misalign != buf_misalign)
				debug_out("file_io: warning: buffer %p and offset %x are misaligned", buf, raw_ofs);
#endif
		}
		// requesting we allocate the buffer
		else
		{
			size_t buf_size = round_up(misalign + raw_size, BLOCK_SIZE);
			our_buf = mem_alloc(buf_size, BLOCK_SIZE);
			if(!our_buf)
				return ERR_NO_MEM;
			buf = our_buf;
			*p = (char*)buf + misalign;
		}
	}

	// buf is now the source or sink, regardless of who allocated it.
	// we need to keep our_buf (memory we allocated), so we can free
	// it if we fail; it's 0 if the caller passed in a buffer.


	//
	// now we read the file in 64 KB chunks, N-buffered.
	// if reading from Zip, inflate while reading the next block.
	//

	const int MAX_IOS = 2;
	Handle ios[MAX_IOS] = { 0 };

	int head = 0;
	int tail = 0;
	int pending_ios = 0;

	bool all_issued = false;

	// (useful, raw data: possibly compressed, but doesn't count padding)
	size_t raw_transferred_cnt = 0;
	size_t issue_cnt = 0;

	// if callback, what it reports; otherwise, = raw_transferred_cnt
	// this is what we'll return
	size_t actual_transferred_cnt = 0;

	ssize_t err = +1;		// loop terminates if <= 0


	for(;;)
	{
		// queue not full, data remaining to transfer, and no error:
		// start transferring next block.
		if(pending_ios < MAX_IOS && !all_issued && err > 0)
		{
			// calculate issue_size:
			// at most, transfer up to the next block boundary.
			off_t issue_ofs = (off_t)(start_ofs + issue_cnt);
			const size_t left_in_block = BLOCK_SIZE - (issue_ofs % BLOCK_SIZE);
			const size_t total_left = raw_size - issue_cnt;
			size_t issue_size = MIN(left_in_block, total_left);

			// assume temp buffer allocated by file_start_io
			void* data = 0;
			// if transferring to/from normal file, use buf instead
			if(buf)
				data = (void*)((uintptr_t)buf + issue_cnt);

			Handle hio = file_start_io(f, issue_ofs, issue_size, data);
			if(hio <= 0)
				err = (ssize_t)hio;
				// transfer failed - loop will now terminate after
				// waiting for all pending transfers to complete.

			issue_cnt += issue_size;
			if(issue_cnt >= raw_size)
				all_issued = true;

			// store IO in ring buffer
			ios[head] = hio;
			head = (head + 1) % MAX_IOS;
			pending_ios++;
		}
		// IO pending: wait for it to complete, and process it.
		else if(pending_ios)
		{
			Handle& hio = ios[tail];
			tail = (tail + 1) % MAX_IOS;
			pending_ios--;

			void* block;
			size_t size;
			int ret = file_wait_io(hio, block, size);
			if(ret < 0)
				err = (ssize_t)ret;

			//// if size comes out short, we must be at EOF

			raw_transferred_cnt += size;

			if(cb && !(err <= 0))
			{
				ssize_t ret = cb(ctx, block, size);
				// if negative: processing failed; if 0, callback is finished.
				// either way, loop will now terminate after waiting for all
				// pending transfers to complete.
				if(ret <= 0)
					err = ret;
				else
					actual_transferred_cnt += ret;
			}
			// no callback to process data: raw = actual
			else
				actual_transferred_cnt += size;

			file_discard_io(hio);	// zeroes array entry
		}
		// (all issued OR error) AND no pending transfers - done.
		else
			break;
	}

	// failed (0 means callback reports it's finished)
	if(err < 0)
	{
		// user didn't specify output buffer - free what we allocated,
		// and clear 'out', which points to the freed buffer.
		if(our_buf)
		{
			mem_free(our_buf);
			*p = 0;
				// we only allocate if p && *p, but had set *p above.
		}
		return err;
	}

	assert(issue_cnt == raw_transferred_cnt && raw_transferred_cnt == raw_size); 

	return (ssize_t)actual_transferred_cnt;
}


///////////////////////////////////////////////////////////////////////////////
//
// memory mapping
//
///////////////////////////////////////////////////////////////////////////////


// no significance aside from preventing uint overflow.
static const uint MAX_MAP_REFS = 255;


// map the entire file <f> into memory. if already currently mapped,
// return the previous mapping (reference-counted).
// output parameters are zeroed on failure.
//
// the mapping will be removed (if still open) when its file is closed.
// however, map/unmap calls should still be paired so that the mapping
// may be removed when no longer needed.
//
// rationale: reference counting is required for zip_map: several
// Zip "mappings" each reference one ZArchive's actual file mapping.
// implement it here so that we also get refcounting for normal files.
int file_map(File* const f, void*& p, size_t& size)
{
	p = 0;
	size = 0;

	CHECK_FILE(f);

	// already mapped - increase refcount and return previous mapping.
	if(f->mapping)
	{
		// prevent overflow; if we have this many refs, should find out why.
		if(f->map_refs >= MAX_MAP_REFS)
		{
			debug_warn("file_map: too many references to mapping");
			return -1;
		}
		f->map_refs++;
		goto have_mapping;
	}

	const int prot = (f->flags & FILE_WRITE)? PROT_WRITE : PROT_READ;
	f->mapping = mmap((void*)0, f->size, prot, MAP_PRIVATE, f->fd, (off_t)0);
	if(!f->mapping)
		return ERR_NO_MEM;

	f->map_refs = 1;

have_mapping:
	p = f->mapping;
	size = f->size;
	return 0;
}


// decrement the reference count for the mapping belonging to file <f>.
// fail if there are no references; remove the mapping if the count reaches 0.
//
// the mapping will be removed (if still open) when its file is closed.
// however, map/unmap calls should still be paired so that the mapping
// may be removed when no longer needed.
int file_unmap(File* const f)
{
	CHECK_FILE(f);

	// file is not currently mapped
	if(f->map_refs == 0)
		return -1;

	// still more than one reference remaining - done.
	if(--f->map_refs > 0)
		return 0;

	// no more references: remove the mapping
	void* const p = f->mapping;
	f->mapping = 0;
	// don't clear f->size - the file is still open.

	return munmap(p, f->size);
}
