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

#include "lib.h"
#include "file.h"
#include "h_mgr.h"
#include "mem.h"
#include "detect.h"
#include "adts.h"

#include <cassert>

#include <vector>
#include <functional>
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


///////////////////////////////////////////////////////////////////////////////
//
// file open/close
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


static int file_validate(uint line, File* f)
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
	assert(0 && "file_validate failed");
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


int file_open(const char* path, int flags, File* f)
{
	memset(f, 0, sizeof(File));

	if(!f)
		goto invalid_f;
		// jump to CHECK_FILE post-check, which will handle this.

{
	// don't stat if opening for writing - the file may not exist yet
	size_t size = 0;

	int mode = O_RDONLY;
	if(flags & FILE_WRITE)
		mode = O_WRONLY;
	else
	{
		struct stat s;
		int err = stat(path, &s);
		if(err < 0)
			return err;
		size = s.st_size;
	}

	int fd = open(path, mode);
	if(fd < 0)
		return 1;

#ifdef PARANOIA
	f->magic = FILE_MAGIC;
#endif

	f->flags   = flags;
	f->size    = size;
	f->fn_hash = fnv_hash(path);
	f->mapping = 0;
	f->fd      = fd;
}

invalid_f:
	CHECK_FILE(f)

	return 0;
}


int file_close(File* f)
{
	CHECK_FILE(f);

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
// block cache
//
///////////////////////////////////////////////////////////////////////////////


static Cache c;


// create a tag for use with the Cache that uniquely identifies
// the block from the file <fn_hash> containing <ofs>.
static u64 make_tag(u32 fn_hash, size_t ofs)
{
	// tag format: filename hash | block number
	//             63         32   31         0
	//
	// we assume the hash (currently: FNV) is unique for all filenames.
	// chance of a collision is tiny, and a build tool will ensure
	// filenames in the VFS archives are safe.
	//
	// block_num will always fit in 32 bits (assuming maximum file size
	// = 2^32 * BLOCK_SIZE = 2^48 - enough); we check this, but don't
	// include a workaround. we could return 0, and the caller would have
	// to allocate their own buffer, but don't bother.

	// make sure block_num fits in 32 bits
	size_t block_num = ofs / BLOCK_SIZE;
	assert(block_num <= 0xffffffff);

	u64 tag = fn_hash;	// careful, don't shift a u32 32 bits left
	tag <<= 32;
	tag |= block_num;
	return tag;
}


// 
static void* block_alloc(u64 tag)
{
	void* p;

	// initialize pool, if not done already.
	static size_t cache_size;
	static size_t cache_pos = 0;
	static void* cache = 0;
	if(!cache)
	{
		cache_size = 16 * BLOCK_SIZE;
		get_mem_status();
		// TODO: adjust cache_size
		cache = mem_alloc(cache_size, BLOCK_SIZE);
		if(!cache)
			return 0;
	}

	// we have free blocks - add to cache
	if(cache_pos < cache_size)
	{
		p = (char*)cache + cache_pos;
		cache_pos += BLOCK_SIZE;

		if(c.add(tag, p) < 0)
		{
			assert(0 && "block_alloc: Cache::add failed!");
			return 0;
		}
	}
	// all of our pool's blocks are in the cache.
	// displace the LRU entry. if not possible (all are locked), fail.
	else
	{
		p = c.replace_lru_with(tag);
		if(!p)
			return 0;
	}

	if(c.lock(tag, true) < 0)
		assert(0 && "block_alloc: Cache::lock failed!");
		// can't happen: only cause is tag not found, but we successfully
		// added it above. if it did fail, that'd be bad: we leak the block,
		// and/or the buffer may be displaced while in use. hence, assert.
	
	return p;
}


// modifying cached blocks is not supported.
// we could allocate a new buffer and update the cache to point to that,
// but that'd fragment our memory pool.
// instead, add a copy on write call, if necessary.



static int block_retrieve(u64 tag, void*& p)
{
	p = c.get(tag);
	return p? 0 : -1;

	// important note:
	// for the purposes of starting the IO, we can regard blocks whose read
	// is still pending it as cached. when getting the IO results,
	// we'll have to wait on the actual IO for that block.
	//
	// don't want to require IOs to be completed in order of issue:
	// that'd mean only 1 caller can read from file at a time.
	// would obviate associating tag with IO, but is overly restrictive.
}


static int block_discard(u64 tag)
{
	return c.lock(tag, false);
}


void file_free_buf(void *p)
{
}


// remove from cache?
int discard_buf(void* p)
{
	return 0;
}

int free_buf(void* p)
{
	uintptr_t _p = (uintptr_t)p;
	void* actual_p = (void*)(_p - (_p % BLOCK_SIZE));	// round down

	return mem_free(actual_p);
}

///////////////////////////////////////////////////////////////////////////////
//
// async I/O
//
///////////////////////////////////////////////////////////////////////////////


enum
{
	CACHED  = 1,
};

struct IO
{
	struct aiocb* cb;
		// struct aiocb is too big to store here.
		// IOs are reused, so we don't allocate a
		// new aiocb every file_start_io.

	u64 tag;

	void* user_p;
	size_t user_ofs;
	size_t user_size;

	uint flags;

	void* block;
};

H_TYPE_DEFINE(IO)


// don't support forcibly closing files => don't need to keep track of
// all IOs pending for each file. too much work, little benefit.


static void IO_init(IO* io, va_list args)
{
	size_t size = round_up(sizeof(struct aiocb), 16);
	io->cb = (struct aiocb*)mem_alloc(size, 16, MEM_ZERO);
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
	if(!io->cb)
		return -1;
	return 0;
}


///////////////////////////////////////////////////////////////////////////////


// extra layer on top of h_alloc, so we can reuse IOs
//
// (avoids allocating the aiocb every IO => less fragmentation)
//
// don't worry about reassigning IOs to their associated file -
// they don't need to be reloaded, since the VFS refuses reload
// requests for files with pending IO.

typedef std::vector<Handle> IOList;

// accessed MRU for better cache locality
static IOList free_ios;

// list of all IOs allocated.
// used to find active IO, given tag (see below).
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
static void io_cleanup(void)
{
	std::for_each(all_ios.begin(), all_ios.end(), Free());
}


static Handle io_alloc()
{
	ONCE(atexit(io_cleanup))

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
		// note that no one else can actually free an IO -
		// that would require its handle type, which is private to
		// this module. the free_io call just adds it to the freelist;
		// all allocated IOs are destroyed by the handle manager at exit.
	}

	// allocate a new IO
	Handle h = h_alloc(H_IO, 0);
	// .. it's valid - store in list.
	if(h > 0)
		all_ios.push_back(h);
	return h;
}


static int io_free(Handle hio)
{
	// mark it unused, and incidentally make sure hio is valid
	// before adding to freelist.
	H_DEREF(hio, IO, io);
	io->tag = 0;
	free_ios.push_back(hio);
	return 0;
}


// need to find IO, given tag, to make sure a block
// that is marked cached has actually been read.
// it is expected that there only be a few allocated IOs,
// so it's ok to search this list every cache hit.
// adding to the cache data structure would be messier.

struct FindTag : public std::binary_function<Handle, u64, bool>
{
	bool operator()(Handle hio, u64 tag) const
	{
		// can't use H_DEREF - we return bool
		IO* io = (IO*)h_user_data(hio, H_IO);
		if(!io)
		{
			assert(0 && "invalid handle in all_ios list!");
			return false;
		}
		return io->tag == tag;
	}
};

static Handle io_find_tag(u64 tag)
{
	IOList::const_iterator it;
	it = std::find_if(all_ios.begin(), all_ios.end(), std::bind2nd(FindTag(), tag));
	// not found
	if(it == all_ios.end())
		return 0;

	return *it;
}


///////////////////////////////////////////////////////////////////////////////



// rationale for extra alignment / copy layer, even though aio takes care of it:
// aio would read pad to its minimum read alignment, copy over, and be done;
// in our case, if something is unaligned, a request for the remainder of the
// block is likely to follow, so we want to cache the whole block.


// pads the request up to BLOCK_SIZE, and stores the original parameters in IO.
// transfers of more than 1 block (including padding) are allowed, but do not
// go through the cache. don't see any case where that's necessary, though.
Handle file_start_io(File* f, size_t user_ofs, size_t user_size, void* user_p)
{
	int err;

	CHECK_FILE(f)

	if(user_size == 0)
	{
		assert(0 && "file_start_io: user_size = 0 - why?");
		return ERR_INVALID_PARAM;
	}
	if(user_ofs >= f->size)
	{
		assert(0 && "file_start_io: user_ofs beyond f->size");
		return -1;
	}

	size_t bytes_left = f->size - user_ofs;	// > 0
	int op = (f->flags & FILE_WRITE)? LIO_WRITE : LIO_READ;

	// don't read beyond EOF
	if(user_size > bytes_left)		// avoid min() - it wants int
		user_size = bytes_left;


	const u64 tag = make_tag(f->fn_hash, user_ofs);

	// allocate IO slot
	Handle hio = io_alloc();
	H_DEREF(hio, IO, io);
	struct aiocb* cb = io->cb;

	io->tag = tag;
	io->user_p = user_p;
	io->user_ofs = user_ofs;
	io->user_size = user_size;
		// notes: io->flags and io->block are already zeroed;
		// cb holds the actual IO request (aligned offset and size).

	// if already cached, we're done
	if(block_retrieve(tag, io->block) == 0)
	{
		io->flags = CACHED;
		return hio;
	}


	// aio already safely handles unaligned buffers or offsets.
	// when reading zip files, we don't want to repeat a read
	// if a block containing end of one file and start of the next
	// (speed concern).
	//
	// note: cache even if this is the last block before EOF:
	// a zip archive may contain one last file in the block.
	// if not, no loss - the buffer will be LRU, and reused.


	size_t ofs = user_ofs;
	size_t padding = ofs % BLOCK_SIZE;
	ofs -= padding;
	size_t size = round_up(padding + user_size, BLOCK_SIZE);

	void* buf = 0;
	void* our_buf = 0;

	if(user_p && !padding)
		buf = user_p;
	else
	{
		if(size == BLOCK_SIZE)
			our_buf = io->block = block_alloc(tag);
		// transferring more than one block - doesn't go through cache!
		else
			our_buf = mem_alloc(size, BLOCK_SIZE);
		if(!our_buf)
		{
			err = ERR_NO_MEM;
			goto fail;
		}

		buf = our_buf;
	}

	// send off async read/write request
	cb->aio_lio_opcode = op;
	cb->aio_buf        = buf;
	cb->aio_fildes     = f->fd;
	cb->aio_offset     = (off_t)ofs;
	cb->aio_nbytes     = size;
	err = lio_listio(LIO_NOWAIT, &cb, 1, (struct sigevent*)0);
		// return as soon as I/O is queued

	if(err < 0)
	{
fail:
		file_discard_io(hio);
		file_free_buf(our_buf);
		return err;
	}

	return hio;
}


int file_wait_io(const Handle hio, void*& p, size_t& size)
{
	int ret = 0;

	p = 0;
	size = 0;

	H_DEREF(hio, IO, io);
	struct aiocb* cb = io->cb;

	size = io->user_size;
	ssize_t bytes_transferred;

	// block's tag is in cache. need to check if its read is still pending.
	if(io->flags & CACHED)
	{
		Handle cache_hio = io_find_tag(io->tag);
		// was already finished - don't wait
		if(cache_hio <= 0)
			goto skip_wait;
		// not finished yet; wait for it below, as with uncached reads.
		else		
		{
			H_DEREF(cache_hio, IO, cache_io);
				// can't fail, since io_find_tag has to dereference each handle.
			cb = cache_io->cb;
		}
	}

	// wait for transfer to complete
	{
		while(aio_error(cb) == -EINPROGRESS)
			aio_suspend(&cb, 1, NULL);

		bytes_transferred = aio_return(cb);
		assert(bytes_transferred == BLOCK_SIZE);
		ret = bytes_transferred? 0 : -1;
	}
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
	// read directly into target buffer
	else
		p = (void *)cb->aio_buf; // cb->aio_buf is volatile, p is not

	return ret;
}


int file_discard_io(Handle& hio)
{
	H_DEREF(hio, IO, io);
	block_discard(io->tag);
	io_free(hio);
	return 0;
}




// transfer modes:
// *p != 0: *p is the source/destination address for the transfer.
//          (FILE_MEM_READONLY?)
// *p == 0: allocate a buffer, read into it, and return it in *p.
//          when no longer needed, it must be freed via file_discard_buf.
//  p == 0: read raw_size bytes from file, starting at offset raw_ofs,
//          into temp buffers; each block read is passed to cb, which is
//          expected to write actual_size bytes total to its output buffer
//          (for which it is responsible).
//          useful for reading compressed data.
//
// return (positive) number of raw bytes transferred if successful;
// otherwise, an error code.
ssize_t file_io(File* const f, const size_t raw_ofs, size_t raw_size, void** const p,
	const FILE_IO_CB cb, const uintptr_t ctx) // optional
{
	CHECK_FILE(f)

	const bool is_write = (f->flags == FILE_WRITE);

	//
	// transfer parameters
	//

	// reading: make sure we don't go beyond EOF
	if(!is_write)
	{
		if(raw_ofs >= f->size)
			return ERR_EOF;
		raw_size = MIN(f->size - raw_ofs, raw_size);
	}
	// writing: make sure buffer is valid
	else
	{
		// temp buffer OR supposed to be allocated here: invalid
		if(!p || !*p)
		{
			assert(0 && "file_io: write to file from 0 buffer");
			return ERR_INVALID_PARAM;
		}
	}

	const size_t misalign = raw_ofs % BLOCK_SIZE;

	// actual transfer start offset
	const size_t start_ofs = raw_ofs - misalign;	// BLOCK_SIZE-aligned


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
	Handle ios[MAX_IOS] = { 0, 0 };
	int head = 0;
	int tail = 0;
	int pending_ios = 0;

	bool all_issued = false;

	size_t raw_cnt = 0;		// amount of raw data transferred so far
	size_t issue_cnt = 0;	// sum of I/O transfer requests

	ssize_t err = +1;		// loop terminates if <= 0

	for(;;)
	{
		// queue not full, data remaining to transfer, and no error:
		// start transferring next block.
		if(pending_ios < MAX_IOS && !all_issued && err > 0)
		{
			// calculate issue_size:
			// want to transfer up to the next block boundary.
			size_t issue_ofs = start_ofs + issue_cnt;
			const size_t left_in_block = BLOCK_SIZE - (issue_ofs % BLOCK_SIZE);
			const size_t left_in_file = raw_size - issue_cnt;
			size_t issue_size = MIN(left_in_block, left_in_file);

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

			raw_cnt += size;

			if(cb && !(err <= 0))
			{
				ssize_t ret = cb(ctx, block, size);
				// if negative: processing failed; if 0, callback is finished.
				// either way, loop will now terminate after waiting for all
				// pending transfers to complete.
				if(ret <= 0)
					err = ret;
			}

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

	assert(issue_cnt >= raw_cnt && raw_cnt == raw_size); 

	return (ssize_t)raw_cnt;
}


///////////////////////////////////////////////////////////////////////////////
//
// memory mapping
//
///////////////////////////////////////////////////////////////////////////////


int file_map(File* f, void*& p, size_t& size)
{
	CHECK_FILE(f)

	p = f->mapping;
	size = f->size;

	// already mapped - done
	if(p)
		return 0;

	int prot = (f->flags & FILE_WRITE)? PROT_WRITE : PROT_READ;

	p = f->mapping = mmap((void*)0, (uint)size, prot, MAP_PRIVATE, f->fd, (long)0);
	if(!p)
	{
		size = 0;
		return ERR_NO_MEM;
	}

	return 0;
}


int file_unmap(File* f)
{
	CHECK_FILE(f)

	void* p = f->mapping;
	// not currently mapped
	if(!p)
		return -1;
	f->mapping = 0;
	// don't reset size - the file is still open.

	return munmap(p, (uint)f->size);
}
