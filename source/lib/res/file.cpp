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
#include "sysdep/sysdep.h"

#include <vector>
#include <algorithm>

#include <string>

// block := power-of-two sized chunk of a file.
// all transfers are expanded to naturally aligned, whole blocks
// (this makes caching parts of files feasible; it is also much faster
// for some aio implementations, e.g. wposix).
const size_t BLOCK_SIZE_LOG2 = 16;		// 2**16 = 64 KB
const size_t BLOCK_SIZE = 1ul << BLOCK_SIZE_LOG2;

const size_t SECTOR_SIZE = 4096;
	// reasonable guess. if too small, aio will do alignment.



// rationale for aio, instead of only using mmap:
// - parallelism: instead of just waiting for the transfer to complete,
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


enum Conversion
{
	TO_NATIVE,
	TO_PORTABLE
};

static int convert_path(char* dst, const char* src, Conversion conv = TO_NATIVE)
{
	/*
	// if there's a platform with multiple-character DIR_SEP,
	// scan through the path and add space for each separator found.
	const size_t len = strlen(p_path);

	n_path = (const char*)malloc(len * sizeof(char));
	if(!n_path)
	return ERR_NO_MEM;
	*/

	const char* s = src;
	char* d = dst;

	char from = DIR_SEP, to = '/';
	if(conv == TO_NATIVE)
		from = '/', to = DIR_SEP;

	size_t len = 0;

	for(;;)
	{
		len++;
		if(len >= PATH_MAX)
			return -1;

		char c = *s++;

		if(c == from)
			c = to;

		*d++ = c;

		// end of string - done
		if(c == '\0')
			return 0;
	}
}


// set by file_rel_chdir
static char n_root_dir[PATH_MAX];
static size_t n_root_dir_len;


// return the native equivalent of the given portable path
// (i.e. convert all '/' to the platform's directory separator)
// makes sure length < PATH_MAX.
int file_make_native_path(const char* const path, char* const n_path)
{
	strcpy(n_path, n_root_dir);
	return convert_path(n_path+n_root_dir_len, path, TO_NATIVE);
}


int file_make_portable_path(const char* const n_path, char* const path)
{
	if(strncmp(n_path, n_root_dir, n_root_dir_len) != 0)
		return -1;
	return convert_path(path, n_path+n_root_dir_len, TO_PORTABLE);
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

	// get full path to executable
	char n_path[PATH_MAX];
	// .. first try safe, but system-dependent version
	if(get_executable_name(n_path, PATH_MAX) < 0)
	{
		// .. failed; use argv[0]
		if(!realpath(argv0, n_path))
			goto fail;
	}

	// make sure it's valid
	if(access(argv0, X_OK) < 0)
		goto fail;

	// strip executable name and append rel_path
	char* fn = strrchr(n_path, DIR_SEP);
	if(!fn)
	{
		msg = "realpath returned an invalid path?";
		goto fail;
	}
	CHECK_ERR(convert_path(fn+1, rel_path));

	if(chdir(n_path) < 0)
		goto fail;

	// get actual root dir - previous n_path may include ..
	// (slight optimization, speeds up path lookup)
	if(getcwd(n_root_dir, sizeof(n_root_dir)) < 0)
		goto fail;
	n_root_dir_len = strlen(n_root_dir)+1;	// +1 for trailing DIR_SEP
	n_root_dir[n_root_dir_len-1] = DIR_SEP;
		// append to simplify code that uses n_root_dir
		// already 0-terminated, since it's static

	return 0;

	}

fail:
	debug_warn("file_rel_chdir failed");
	if(msg)
	{
		debug_out("file_rel_chdir: %s\n", msg);
		return -1;
	}

	return -errno;
}


// need to store entries returned by readdir so they can be sorted.
struct DirEnt
{
	const std::string name;
	const off_t size;

	DirEnt(const char* const _name, const off_t _size)
		: name(_name), size(_size) {}

private:
	DirEnt& operator=(const DirEnt&);
};

// pointer to DirEnt: faster sorting, but more allocs.
typedef std::vector<const DirEnt*> DirEnts;
typedef DirEnts::const_iterator DirEntIt;

static bool dirent_less(const DirEnt* const d1, const DirEnt* const d2)
	{ return d1->name.compare(d2->name) < 0; }


// for all files and dirs in <dir> (but not its subdirs!):
// call <cb>, passing <user> and the entries's name (not path!)
//
// rationale:
//   this makes file_enum and zip_enum slightly incompatible, since zip_enum
//   returns the full path. that's necessary because VFS add_dirent_cb
//   has no other way of determining what VFS dir a Zip file is in,
//   since zip_enum enumerates all files in the archive (not only those
//   in a given dir). no big deal though, since add_dirent_cb has to
//   special-case Zip files anyway.
//   the advantage here is simplicity, and sparing callbacks the trouble
//   of converting from/to native path (we just give 'em the dirent name).
int file_enum(const char* const dir, const FileCB cb, const uintptr_t user)
{
	// full path for stat
	char n_path[PATH_MAX];
	n_path[PATH_MAX-1] = '\0';
		// will append filename to this, hence "path".
		// 0-terminate simplifies filename strncpy below.
	CHECK_ERR(convert_path(n_path, dir));

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

		strncpy(fn_start, fn, PATH_MAX-n_path_len-1);
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

		off_t size = s.st_size;

		// dir
		if(S_ISDIR(s.st_mode))
		{
			// skip . and ..
			if(fn[0] == '.' && (fn[1] == '\0' || (fn[1] == '.' && fn[2] == '\0')))
				continue;

			size = -1;
		}
		// skip if neither dir nor file
		else if(!S_ISREG(s.st_mode))
			continue;

		const DirEnt* const ent = new DirEnt(fn, size);
		dirents.push_back(ent);
	}
	closedir(os_dir);

	std::sort(dirents.begin(), dirents.end(), dirent_less);

	DirEntIt it;
	for(it = dirents.begin(); it != dirents.end(); ++it)
	{
		const DirEnt* const ent = *it;
		const char* name_c = ent->name.c_str();
		const ssize_t size = ent->size;
		ret = cb(name_c, size, user);
		if(ret < 0)
			if(cb_err == 0)
				cb_err = ret;

		delete ent;
	}

	if(cb_err < 0)
		return cb_err;
	return stat_err;
}


int file_stat(const char* const path, struct stat* const s)
{
	char n_path[PATH_MAX+1];
	CHECK_ERR(convert_path(n_path, path));

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
//   we don't want to make this module depend on VFS, so we don't
//   have access to the file location DB; VFS needs to allocate the handle.
// - no problem exposing our internals via File struct -
//   we're only used by the VFS and Zip modules. don't bother making
//   an opaque struct - that'd have to be kept in sync with the real thing.
// - when Zip opens its archives via file_open, a handle isn't needed -
//   the Zip module hides its File struct (required to close the file),
//   and the Handle approach doesn't guard against some idiot calling
//   close(our_fd_value) directly, either.


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
	CHECK_ERR(convert_path(n_fn, p_fn));

	if(!f)
		goto invalid_f;
		// jump to CHECK_FILE post-check, which will handle this.

{
	// don't stat if opening for writing - the file may not exist yet
	off_t size = 0;

	int oflag = O_RDONLY;
	if(flags & FILE_WRITE)
		oflag = O_WRONLY | O_CREAT;
	else
	{
		struct stat s;
		if(stat(n_fn, &s) < 0)
			return -1;
		if(!S_ISREG(s.st_mode))
			return -1;
		size = s.st_size;
	}

#ifdef _WIN32
	oflag |= O_BINARY;
#endif

	int fd = open(n_fn, oflag, S_IRWXO|S_IRWXU|S_IRWXG);
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
	if(f->mapping)	// only free if necessary (unmap complains if not mapped)
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
// async I/O
//
///////////////////////////////////////////////////////////////////////////////


// L3 cache: intended to cache raw compressed data, since files aren't aligned
// in the archive; alignment code would force a read of the whole block,
// which would be a slowdown unless we keep them in memory.
//
// it's a part of the async code (instead of the sync, block-splitting code)
// because if cached, the IO must not be issued. also, when waiting,
// we can return directly if in the cache.
//
// only allow caching for buffers allocated by async read code:
// 1) caller may pull the rug out from under us, freeing its buffer after
//    calling file_discard_io. don't want to go to the trouble of getting
//    the handle; even worse, buffer may be stack-allocated.
//
// side effect: any async reads into buffers we allocate may be cached
// (if FILE_CACHE_BLOCK specified).



// create an id for use with the Cache that uniquely identifies
// the block from the file <fn_hash> starting at <ofs> (aligned).
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


struct Block
{
	FileIO* pending_io;
	void* p;

	Block()
	{
		pending_io = 0;
		p = 0;
	}
};

typedef std::map<u64, Block> BlockCache;
typedef BlockCache::iterator BlockIt;
static BlockCache block_cache;


enum FileIOFlags
{
// coming from cache
// aio_return called
// we allocated buffer
};



			// pads the request up to BLOCK_SIZE, and stores the original parameters in IO.
			// transfers of more than 1 block (including padding) are allowed, but do not
			// go through the cache. don't see any case where that's necessary, though.
int file_start_io(File* const f, const off_t user_ofs, size_t user_size, void* const user_p, FileIO* io)
{
	int err;

	memset(io, 0, sizeof(FileIO));

	//
	// check params
	//

	CHECK_FILE(f);

	const bool is_write = (f->flags & FILE_WRITE) != 0;
	const bool alloc_buf = (user_p == 0);
	const bool cache_block = (f->flags & FILE_CACHE_BLOCK) != 0 && alloc_buf;


	if(user_size == 0)
	{
		debug_warn("file_start_io: user_size = 0 - why?");
		return ERR_INVALID_PARAM;
	}

	// cut off at EOF.
	if(!is_write)
	{
		// avoid min() due to type conversion warnings.
		const off_t bytes_left = f->size - user_ofs;
		if(bytes_left < 0)
		{
			debug_warn("file_start_io: EOF");
			return ERR_EOF;
		}
		if((off_t)user_size > bytes_left)
			user_size = (size_t)bytes_left;
			// guaranteed to fit, since size was > bytes_left
	}


#ifdef PARANOIA
debug_out("file_start_io hio=%I64x ofs=%d size=%d\n", hio, user_ofs, user_size);
#endif

	size_t padding = 0;
	size_t size = user_size;
	void* buf = user_p;
	off_t ofs = user_ofs;

	// we're supposed to allocate the buffer
	if(alloc_buf)
	{
		if(is_write)
		{
			debug_warn("file_start_io: writing but buffer = 0");
			return ERR_INVALID_PARAM;
		}

		// optimization: pad to eliminate a memcpy if unaligned
		ofs = user_ofs;
		padding = ofs % SECTOR_SIZE;
		ofs -= (off_t)padding;
		size = round_up(padding + user_size, SECTOR_SIZE);

		buf = mem_alloc(size, SECTOR_SIZE);
		if(!buf)
			return ERR_NO_MEM;
	}

	// store request params needed by file_wait_io
	io->cb = (aiocb*)calloc(sizeof(aiocb), 1);
		// must be zeroed! (waio complains about req_ != 0)
	if(!io->cb)
	{
		err = ERR_NO_MEM;
		goto fail;
	}
	io->padding   = padding;
	io->user_size = user_size;

	io->block_id  = 0;

	io->our_buf   = alloc_buf;
	// all other members zeroed by memset above.

	// note: cb will hold the actual IO request
		// (possibly aligned offset and size).

const u64 block_id = block_make_id(f->fn_hash, ofs);
//debug_out("ofs=%x\tid=%I64x", user_ofs, block_id);

	// already in cache?
	if(cache_block)
	{
		io->block_id = block_id;
		Block& b = block_cache[block_id];
		// yes; no need to issue
		if(b.p)
		{
//			debug_out(".. hit\n");
			io->from_cache = true;
			return 0;
		}
		io->given_to_cache = true;
//		debug_out(".. miss\n");
		b.pending_io = io;
		b.p = (void*)buf;
	}
//else
//debug_out(".. uncacheable => miss\n");


	// send off async read/write request
	aiocb* cb = io->cb;
	cb->aio_lio_opcode = is_write? LIO_WRITE : LIO_READ;
	cb->aio_buf        = buf;
	cb->aio_fildes     = f->fd;
	cb->aio_offset     = ofs;
	cb->aio_nbytes     = size;
	err = lio_listio(LIO_NOWAIT, &cb, 1, (struct sigevent*)0);
	if(err < 0)
	{
fail:
		file_discard_io(io);
		if(alloc_buf)
			mem_free(buf);
		return err;
	}

	return 0;
}


// indicates if the IO referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
int file_io_complete(FileIO* io)
{
	int ret = aio_error(io->cb);
	if(ret == EINPROGRESS)
		return 0;
	if(ret == 0)
		return 1;
	
	debug_warn("file_io_complete: unexpected aio_error return");
	return -1;
}


int file_wait_io(FileIO* io, void*& p, size_t& size)
{
#ifdef PARANOIA
debug_out("file_wait_io: hio=%I64x\n", hio);
#endif

	// zero output params in case something (e.g. H_DEREF) fails.
	p = 0;
	size = 0;

	// aio_return may be called exactly once.
	// if user code must be able to call this > 1x, store bytes_transferred
	// in FileIO.
	if(io->return_called)
	{
		debug_warn("file_wait_io: already called");
		return -1;
	}
	io->return_called = 1;

	aiocb* cb = io->cb;
	ssize_t bytes_transferred;

	Block* b;
	if(io->from_cache || io->given_to_cache)
	{
		b = &block_cache[io->block_id];
		if(b->pending_io)
			cb = b->pending_io->cb;
		// already finished, no wait necessary
		else
		{
			cb = 0;
			p = b->p;
			bytes_transferred = BLOCK_SIZE;
		}
	}

	// wait for transfer to complete.
	if(cb)
	{
		const aiocb** cbs = (const aiocb**)&cb;	// pass in an "array"
		while(aio_error(cb) == EINPROGRESS)
			aio_suspend(cbs, 1, (timespec*)0);	// wait indefinitely

		// query number of bytes transferred (-1 if the transfer failed)
		bytes_transferred = aio_return(cb);
		p = (void*)cb->aio_buf;	// cast from volatile void*
	}

	// mark block's pending IO as complete
	if(io->from_cache || io->given_to_cache)
		b->pending_io = 0;

	if(bytes_transferred < (ssize_t)io->user_size)
		return -1;

	size = io->user_size;

	// padding optimization: we rounded the start offset down
	// to avoid a buffer memcpy in waio. skip past that
	(char*&)p += io->padding;

	return 0;
}


int file_discard_io(FileIO* io)
{
	if(!io->return_called)
	{
		debug_warn("file_discard_io: file_wait_io wasn't called yet");
		return -1;
	}

	if(io->our_buf && !io->given_to_cache)
		mem_free(io->cb->aio_buf);

	memset(io->cb, 0, sizeof(aiocb));
	free(io->cb);

	memset(io, 0, sizeof(FileIO));
	return 0;
}




///////////////////////////////////////////////////////////////////////////////









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

ssize_t file_io(File* const f, const off_t data_ofs, size_t data_size, void** const p,
	const FileIOCB cb, const uintptr_t ctx) // optional
{
#ifdef PARANOIA
debug_out("file_io fd=%d size=%d ofs=%d\n", f->fd, data_size, data_ofs);
#endif

	CHECK_FILE(f);

	const bool is_write = !!(f->flags & FILE_WRITE);
	const bool no_aio = !!(f->flags & FILE_NO_AIO);

	void* data_buf = 0;	// I/O source or sink buffer


	// when reading:
	if(!is_write)
	{
		// cut data_size off at EOF
		const ssize_t bytes_left = f->size - data_ofs;
		if(bytes_left < 0)
			return -1;
		data_size = MIN(data_size, (size_t)bytes_left);
	}


	//
	// set buffer options
	//

	enum { TEMP, USER, ALLOC } buf_type;

	// .. temp buffer
	if(!p)
		buf_type = TEMP;
	// .. user-specified buffer (=> no align)
	else if(*p)
	{
		buf_type = USER;
		data_buf = *p;
	}
	// .. we allocate the buffer
	else
	{
		buf_type = ALLOC;
		// data_buf will be set from actual_buf
	}


	// sanity checks:
	// .. temp blocks requested AND
	//    (not reading OR using lowio OR no callback)
	if(buf_type == TEMP && (is_write || no_aio || !cb))
	{
invalid:
		debug_warn("file_io: invalid parameter");
		return ERR_INVALID_PARAM;
	}
	// .. write, but no buffer passed in.
	if(is_write && buf_type != USER)
		goto invalid;


	// only align if we allocate the buffer and in AIO mode
	const bool do_align = buf_type != USER && !no_aio;


	//
	// calculate aligned transfer size (no change if !do_align)
	//

	off_t actual_ofs   = data_ofs;
	size_t actual_size = data_size;
	void* actual_buf   = data_buf;

	// note: we go to the trouble of aligning the first block (instead of
	// just reading up to the next block and letting aio realign it),
	// so that it can be taken from the cache.
	// this is not possible if !do_align, since we have to allocate
	// extra buffer space for the padding.

	const size_t ofs_misalign = data_ofs % BLOCK_SIZE;
	const size_t lead_padding = do_align? ofs_misalign : 0;
		// for convenience; used below.

	if(do_align)
	{
		actual_ofs -= (off_t)ofs_misalign;
		actual_size = round_up(ofs_misalign + data_size, BLOCK_SIZE);
	}

	if(buf_type == ALLOC)
	{
		actual_buf = mem_alloc(actual_size, BLOCK_SIZE);
		if(!actual_buf)
			return ERR_NO_MEM;
		data_buf = (char*)actual_buf + lead_padding;
	}

	// warn in debug build if buffer and offset don't match
	// (=> aio would have to realign every block).
#ifndef NDEBUG
	size_t buf_misalign = ((uintptr_t)actual_buf) % BLOCK_SIZE;
	if(actual_buf && actual_ofs % BLOCK_SIZE != buf_misalign)
		debug_out("file_io: warning: buffer %p and offset %x are misaligned\n", actual_buf, data_ofs);
#endif


	// transferring via lowio only.
	if(no_aio)
	{
		lseek(f->fd, data_ofs, SEEK_SET);

		if(is_write)
			return write(f->fd, data_buf, data_size);
		else
			return read(f->fd, data_buf, data_size);
	}


	//
	// now we read the file in 64 KB chunks, N-buffered.
	// if reading from Zip, inflate while reading the next block.
	//

	const int MAX_IOS = 4;
	FileIO ios[MAX_IOS] = { 0 };

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
			off_t issue_ofs = (off_t)(actual_ofs + issue_cnt);
			size_t issue_size = BLOCK_SIZE;
			if(!do_align)
			{
				const size_t left_in_block = BLOCK_SIZE - (issue_ofs % BLOCK_SIZE);
				const size_t total_left = data_size - issue_cnt;
				issue_size = MIN(left_in_block, total_left);
			}

			

			// get next free IO slot in ring buffer
			FileIO* io = &ios[head];
			head = (head + 1) % MAX_IOS;
			pending_ios++;

			// if using buffer, set position in it; otherwise, 0 (temp)
			void* data = (buf_type == TEMP)? 0 : (char*)actual_buf + issue_cnt;
			int ret = file_start_io(f, issue_ofs, issue_size, data, io);
			if(ret < 0)
				err = (ssize_t)ret;
				// transfer failed - loop will now terminate after
				// waiting for all pending transfers to complete.

			issue_cnt += issue_size;
			if(issue_cnt >= actual_size)
				all_issued = true;
		}
		// IO pending: wait for it to complete, and process it.
		else if(pending_ios)
		{
			FileIO* io = &ios[tail];
			tail = (tail + 1) % MAX_IOS;
			pending_ios--;

			void* block;
			size_t size;
			int ret = file_wait_io(io, block, size);
			if(ret < 0)
				err = (ssize_t)ret;

						// first time; skip past padding
						void* data = block;
						if(raw_transferred_cnt == 0)
						{
							(char*&)data += lead_padding;
							size -= lead_padding;
						}

						// don't include trailing padding
						if(raw_transferred_cnt + size > data_size)
							size = data_size - raw_transferred_cnt;

			//// if size comes out short, we must be at EOF

			raw_transferred_cnt += size;

			if(cb && !(err <= 0))
			{
				ssize_t ret = cb(ctx, data, size);
				// if negative: processing failed.
				// loop will now terminate after waiting for all
				// pending transfers to complete.
				// note: don't abort if = 0: zip callback may not actually
				// output anything if passed very little data.
				if(ret < 0)
					err = ret;
				else
					actual_transferred_cnt += ret;
			}
			// no callback to process data: raw = actual
			else
				actual_transferred_cnt += size;

			file_discard_io(io);
		}
		// (all issued OR error) AND no pending transfers - done.
		else
			break;
	}

	// failed (0 means callback reports it's finished)
	if(err < 0)
	{
		// user didn't specify output buffer - free what we allocated,
		// and clear p (value-return param)
		if(buf_type == ALLOC)
		{
			mem_free(actual_buf);
			*p = 0;
				// alloc_buf => p != 0
		}
		return err;
	}

	if(p)
		*p = data_buf;

	assert(issue_cnt >= raw_transferred_cnt && raw_transferred_cnt >= data_size); 

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

	const int prot = (f->flags & FILE_WRITE)? PROT_WRITE : PROT_READ;

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

	// don't allow mapping zero-length files (doesn't make sense,
	// and BoundsChecker warns about wposix mmap failing).
	// then again, don't complain, because this might happen when mounting
	// a dir containing empty files; each is opened as a Zip file.
	if(f->size == 0)
		return -1;

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
	{
		debug_warn("file_unmap: not currently mapped");
		return -1;
	}

	// still more than one reference remaining - done.
	if(--f->map_refs > 0)
		return 0;

	// no more references: remove the mapping
	void* const p = f->mapping;
	f->mapping = 0;
	// don't clear f->size - the file is still open.

	return munmap(p, f->size);
}
