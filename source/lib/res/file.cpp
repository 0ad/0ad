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
#include "res.h"
#include "file.h"
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
const size_t BLOCK_SIZE_LOG2 = 16;		// 2**16 = 64 KiB
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



// is s2 a subpath of s1, or vice versa? used by VFS and wdir_watch.
// works for portable and native paths.
bool file_is_subpath(const char* s1, const char* s2)
{
	// make sure s1 is the shorter string
	if(strlen(s1) > strlen(s2))
		std::swap(s1, s2);

	int c1 = 0, last_c1, c2;
	for(;;)
	{
		last_c1 = c1;
		c1 = *s1++, c2 = *s2++;

		// end of s1 reached:
		if(c1 == '\0')
		{
			// s1 matched s2 up until:
			if((c2 == '\0') ||	// its end (i.e. they're equal length)
			   (c2 == '/' || c2 == DIR_SEP) ||	// start of next component
			   (last_c1 == '/' || last_c1 == DIR_SEP))	// ", but both have a trailing slash
				// => is subpath
				return true;
		}

		// mismatch => is not subpath
		if(c1 != c2)
			return false;
	}
}



enum Conversion
{
	TO_NATIVE,
	TO_PORTABLE
};

static int convert_path(char* dst, const char* src, Conversion conv = TO_NATIVE)
{
	// DIR_SEP is assumed to be a single character!

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
			return ERR_PATH_LENGTH;

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


// return the native equivalent of the given relative portable path
// (i.e. convert all '/' to the platform's directory separator)
// makes sure length < PATH_MAX.
int file_make_native_path(const char* path, char* n_path)
{
	return convert_path(n_path, path, TO_NATIVE);
}

// return the portable equivalent of the given relative native path
// (i.e. convert the platform's directory separators to '/')
// makes sure length < PATH_MAX.
int file_make_portable_path(const char* n_path, char* path)
{
	return convert_path(path, n_path, TO_PORTABLE);
}


// return the native equivalent of the given portable path
// (i.e. convert all '/' to the platform's directory separator).
// also prepends current directory => n_full_path is absolute.
// makes sure length < PATH_MAX.
int file_make_full_native_path(const char* path, char* n_full_path)
{
	strcpy_s(n_full_path, PATH_MAX, n_root_dir);
	return convert_path(n_full_path+n_root_dir_len, path, TO_NATIVE);
}

// return the portable equivalent of the given relative native path
// (i.e. convert the platform's directory separators to '/')
// n_full_path is absolute; if it doesn't match the current dir, fail.
// (note: portable paths are always relative to file_rel_chdir root).
// makes sure length < PATH_MAX.
int file_make_full_portable_path(const char* n_full_path, char* path)
{
	if(strncmp(n_full_path, n_root_dir, n_root_dir_len) != 0)
		return -1;
	return convert_path(path, n_full_path+n_root_dir_len, TO_PORTABLE);
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
int file_rel_chdir(const char* argv0, const char* rel_path)
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
	if(access(n_path, X_OK) < 0)
		goto fail;

	// strip executable name and append rel_path
	char* fn = strrchr(n_path, DIR_SEP);
	if(!fn)
	{
		msg = "realpath returned an invalid path?";
		goto fail;
	}
	CHECK_ERR(file_make_native_path(rel_path, fn+1));

	if(chdir(n_path) < 0)
		goto fail;

	// get actual root dir - previous n_path may include ..
	// (slight optimization, speeds up path lookup)
	if(getcwd(n_root_dir, sizeof(n_root_dir)) == 0)
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

	// store only required fields from struct stat to save space.
	// in order of decl in VC2003 sys/stat.h.
	mode_t st_mode;
	off_t  st_size;
// glibc compat hack - they have st_mtime #defined to st_mtim.tv_sec
#ifdef st_mtime
	struct timespec st_mtim;
#else
	time_t st_mtime;
#endif

	DirEnt(const char* _name, mode_t _st_mode, off_t _st_size, time_t _st_mtime)
		: name(_name)
	{
		st_mode  = _st_mode;
		st_size  = _st_size;
		st_mtime = _st_mtime;
	}

	// no copy ctor, since some members are const
private:
	DirEnt& operator=(const DirEnt&);
};

// pointer to DirEnt: faster sorting, but more allocs.
typedef std::vector<const DirEnt*> DirEnts;
typedef DirEnts::const_iterator DirEntCIt;
typedef DirEnts::reverse_iterator DirEntRIt;

static bool dirent_less(const DirEnt* d1, const DirEnt* d2)
	{ return d1->name.compare(d2->name) < 0; }


// call <cb> for each file and subdirectory in <dir> (alphabetical order),
// passing the entry name (not full path!), stat info, and <user>.
//
// first builds a list of entries (sorted) and remembers if an error occurred.
// if <cb> returns non-zero, abort immediately and return that; otherwise,
// return first error encountered while listing files, or 0 on success.
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
int file_enum(const char* dir, const FileCB cb, const uintptr_t user)
{
	// full path for stat
	char n_path[PATH_MAX];
	n_path[PATH_MAX-1] = '\0';
		// will append filename to this, hence "path".
		// 0-terminate simplifies filename strncpy below.
	CHECK_ERR(file_make_native_path(dir, n_path));

	// all entries are enumerated (adding to this container),
	// std::sort-ed, then all passed to cb.
	DirEnts dirents;

	int stat_err = 0;	// first error encountered by stat()
	int cb_err = 0;		// first error returned by cb
	int ret;

	// [16.6ms]
	DIR* os_dir = opendir(n_path);
	if(!os_dir)
		return ERR_PATH_NOT_FOUND;

	// will append file names here
	const size_t n_path_len = strlen(n_path);
	char* fn_start = n_path + n_path_len;
	*fn_start++ = DIR_SEP;

	struct dirent* os_ent;
	while((os_ent = readdir(os_dir)))
	{
		const char* fn = os_ent->d_name;

		strncpy(fn_start, fn, PATH_MAX-n_path_len-1);	// safe
			// need path for stat (we only have filename ATM).
			// this is actually minimally faster than changing directory!
			// BTW, direct strcpy is faster than path_append -
			// we save a strlen every iteration.

		struct stat s;
#ifdef _WIN32
		// wposix readdir has enough information to return dirent
		// status directly (much faster than calling stat).
		ret = readdir_stat_np(os_dir, &s);
#else
		// no need to go through file_stat - we already have the native path.
		// [290ms]
		ret = stat(n_path, &s);
#endif
		if(ret < 0)
		{
			if(stat_err == 0)
				stat_err = ret;	// first error (see decl)
			continue;
		}

		// dir
		if(S_ISDIR(s.st_mode))
		{
			// skip . and ..
			if(fn[0] == '.' && (fn[1] == '\0' || (fn[1] == '.' && fn[2] == '\0')))
				continue;
		}
		// skip if neither dir nor file
		else if(!S_ISREG(s.st_mode))
			continue;

		// [21ms]
		dirents.push_back(new DirEnt(fn, s.st_mode, s.st_size, s.st_mtime));
	}

	// [2.5ms]
	closedir(os_dir);

	// [5.8ms]
	std::sort(dirents.begin(), dirents.end(), dirent_less);

	struct stat s;
	memset(&s, 0, sizeof(s));
	for(DirEntCIt it = dirents.begin(); it != dirents.end(); ++it)
	{
		const DirEnt* ent = *it;
		const char* name_c = ent->name.c_str();
		s.st_mode  = ent->st_mode;
		s.st_size  = ent->st_size;
		s.st_mtime = ent->st_mtime;
		ret = cb(name_c, &s, user);
		if(ret != 0)
		{
			cb_err = ret;	// first error (since we now abort)
			break;
		}
	}


	// free all memory (can't do in loop above because it can be aborted).
	// [10.4ms]
	for(DirEntRIt rit = dirents.rbegin(); rit != dirents.rend(); ++rit)
		delete *rit;

	if(cb_err != 0)
		return cb_err;
	return stat_err;
}


// get file status. output param is zeroed on error.
int file_stat(const char* path, struct stat* s)
{
	memset(s, 0, sizeof(struct stat));

	char n_path[PATH_MAX+1];
	CHECK_ERR(file_make_native_path(path, n_path));

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


static int file_validate(const uint line, File* f)
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


int file_open(const char* p_fn, const uint flags, File* f)
{
	// zero output param in case we fail below.
	memset(f, 0, sizeof(File));

	char n_fn[PATH_MAX];
	CHECK_ERR(file_make_native_path(p_fn, n_fn));

	if(!f)
		goto invalid_f;
		// jump to CHECK_FILE post-check, which will handle this.

{
	// don't stat if opening for writing - the file may not exist yet
	off_t size = 0;

	int oflag = O_RDONLY;
	if(flags & FILE_WRITE)
		oflag = O_WRONLY | O_CREAT;
	// read access requested
	else
	{
		// get file size
		struct stat s;
		if(stat(n_fn, &s) < 0)
			return ERR_FILE_NOT_FOUND;
		size = s.st_size;

		// note: despite increased overhead, the AIO read method is still
		// significantly faster, even with small files.
		// we therefore don't automatically disable AIO.
		// notes:
		// - up to 32KB can be read by one SCSI request.
		// - flags are stored below and will influence file_io.
//		if(size <= 32*KiB)
//			flags |= FILE_NO_AIO;

		// make sure <n_fn> is a regular file
		if(!S_ISREG(s.st_mode))
			return ERR_NOT_FILE;
	}

#ifdef _WIN32
	oflag |= O_BINARY;

	// if AIO is disabled (at user's behest or because the file is small),
	// so inform wposix.
	if(flags & FILE_NO_AIO)
		oflag |= O_NO_AIO_NP;
#endif

	int fd = open(n_fn, oflag, S_IRWXO|S_IRWXU|S_IRWXG);
	if(fd < 0)
		return ERR_FILE_ACCESS;

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


int file_close(File* f)
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


// rationale:
// asynchronous IO routines don't cache; they're just a thin AIO wrapper.
// it's taken care of by file_io, which splits transfers into blocks
// and keeps temp buffers in memory (not user-allocated, because they
// might pull the rug out from under us at any time).
//
// doing so here would be more complicated: would have to handle "forwarding",
// i.e. recognizing that the desired block has been issued, but isn't yet
// complete. file_io also knows more about whether a block should be cached.
//
// disadvantages:
// - streamed data will always be read from disk. no problem, because
//   such data (e.g. music, long speech) is unlikely to be used again soon.
// - prefetching (issuing the next few blocks from an archive during idle
//   time, so that future out-of-order reads don't need to seek) isn't
//   possible in the background (unless via thread, but that's discouraged).
//   the utility is questionable, though: how to prefetch so as not to delay
//   real IOs? can't determine "idle time" without completion notification,
//   which is hard.
//   we could get the same effect by bridging small gaps in file_io,
//   and rearranging files in the archive in order of access.




// starts transferring to/from the given buffer.
// no attempt is made at aligning or padding the transfer.
int file_start_io(File* f, off_t ofs, size_t size, void* p, FileIO* io)
{
	// zero output param in case we fail below.
	memset(io, 0, sizeof(FileIO));

#ifdef PARANOIA
	debug_out("file_start_io: ofs=%d size=%d\n", ofs, size);
#endif


	//
	// check params
	//

	CHECK_FILE(f);

	if(!size || !p || !io)
		return ERR_INVALID_PARAM;

	const bool is_write = (f->flags & FILE_WRITE) != 0;

	// cut off at EOF.
	if(!is_write)
	{
		// avoid min() due to type conversion warnings.
		const off_t bytes_left = f->size - ofs;
		if(bytes_left < 0)
		{
			debug_warn("file_start_io: EOF");
			return ERR_EOF;
		}
		if((off_t)size > bytes_left)
			size = (size_t)bytes_left;
			// guaranteed to fit, since size was > bytes_left
	}


	// set the "I/O context", a pointer to the (newly allocated) aiocb.
	// we can't store the whole aiocb in a struct - glibc's version is
	// 144 bytes large. we don't currently need anything else (since this
	// code is only a thin aio wrapper); if that changes, instead return a
	// pointer to a struct containing the aiocb*.

	aiocb* cb = (aiocb*)malloc(sizeof(aiocb));
	memset(cb, 0, sizeof(aiocb));
	if(!cb)
		return ERR_NO_MEM;
	io->cb = cb;

	// send off async read/write request
	cb->aio_lio_opcode = is_write? LIO_WRITE : LIO_READ;
	cb->aio_buf        = p;
	cb->aio_fildes     = f->fd;
	cb->aio_offset     = ofs;
	cb->aio_nbytes     = size;
#ifdef PARANOIA
	debug_out("file_start_io: io=%p nbytes=%d\n", io, cb->aio_nbytes);
#endif
	int err = lio_listio(LIO_NOWAIT, &cb, 1, (struct sigevent*)0);
	if(err < 0)
	{
#ifdef PARANOIA
		debug_out("file_start_io: lio_listio: %d, %d[%s]\n", err, errno, strerror(errno));
#endif
		file_discard_io(io);
		return err;
	}

	return 0;
}


// indicates if the IO referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
int file_io_complete(FileIO* io)
{
	aiocb* cb = (aiocb*)io->cb;
	int ret = aio_error(cb);
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
debug_out("file_wait_io: hio=%p\n", io);
#endif

	// zero output params in case something (e.g. H_DEREF) fails.
	p = 0;
	size = 0;

	aiocb* cb = (aiocb*)io->cb;

	// wait for transfer to complete.
	const aiocb** cbs = (const aiocb**)&cb;	// pass in an "array"
	while(aio_error(cb) == EINPROGRESS)
		aio_suspend(cbs, 1, (timespec*)0);	// wait indefinitely

	// query number of bytes transferred (-1 if the transfer failed)
	const ssize_t bytes_transferred = aio_return(cb);
#ifdef PARANOIA
	debug_out("file_wait_io: bytes_transferred=%d aio_nbytes=%d\n",
		bytes_transferred, cb->aio_nbytes);
#endif
	// (size was clipped to EOF in file_io => this is an actual IO error)
	if(bytes_transferred < (ssize_t)cb->aio_nbytes)
		return ERR_IO;

	p = (void*)cb->aio_buf;	// cast from volatile void*
	size = bytes_transferred;
	return 0;
}


int file_discard_io(FileIO* io)
{
	memset(io->cb, 0, sizeof(aiocb));
		// discourage further use.
	free(io->cb);
	io->cb = 0;
	return 0;
}




///////////////////////////////////////////////////////////////////////////////



ssize_t lowio(int fd, bool is_write, off_t ofs, size_t size, void* buf)
{
	lseek(fd, ofs, SEEK_SET);

	if(is_write)
		return write(fd, buf, size);
	else
		return read (fd, buf, size);
}




// L3 cache: intended to cache raw compressed data, since files aren't aligned
// in the archive; alignment code would force a read of the whole block,
// which would be a slowdown unless we keep them in memory.
//
// keep out of async code (although extra work for sync: must not issue/wait
// if was cached) to simplify things. disadvantage: problems if same block
// is issued twice, before the first call completes (via wait_io).
// that won't happen though unless we have threaded file_ios =>
// rare enough not to worry about performance.
//
// since sync code allocates the (temp) buffer, it's guaranteed
// to remain valid.
//



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


typedef std::pair<u64, void*> BlockCacheEntry;
typedef std::map<u64, void*> BlockCache;
typedef BlockCache::iterator BlockIt;
static BlockCache block_cache;







struct IOSlot
{
	FileIO io;
	void* temp_buf;

	u64 block_id;
		// needed so that we can add the block to the cache when
		// its IO is complete. if we add it when issuing, we'd no longer be
		// thread-safe: someone else might find it in the cache before its
		// transfer has completed. don't want to add an "is_complete" flag,
		// because that'd be hard to update (on every wait_io).

	void* cached_block;
		// != 0 <==> data coming from cache and no IO issued.


// given buffer
// given buffer, will copy from cache
// temp buffer allocated here
// temp buffer taken from cache
};


// don't just use operator[], so that block_cache isn't cluttered
// with IDs associated with 0 (blocks that wouldn't be cached anyway).
static void* block_find(u64 block_id)
{
	BlockIt it = block_cache.find(block_id);
	if(it == block_cache.end())
		return 0;
	return it->second;
}


static void block_add(u64 block_id, void* block)
{
	if(block_find(block_id))
		debug_warn("block_add: already in cache");
	else
		block_cache[block_id] = block;
}


static ssize_t block_issue(File* f, IOSlot* slot, const off_t issue_ofs, void* buf)
{
	memset(slot, 0, sizeof(IOSlot));

	ssize_t issue_size = BLOCK_SIZE;

	// check if in cache
	slot->block_id = block_make_id(f->fn_hash, issue_ofs);
	slot->cached_block = block_find(slot->block_id);
	if(slot->cached_block)
		goto skip_issue;

//debug_out("%x miss\n", issue_ofs);

	// allocate temp buffer
	if(!buf)
		buf = slot->temp_buf = mem_alloc(BLOCK_SIZE, BLOCK_SIZE);


	// if using buffer, set position in it; otherwise, use temp buffer
	CHECK_ERR(file_start_io(f, issue_ofs, BLOCK_SIZE, buf, &slot->io));

skip_issue:

	return issue_size;
}


// remove all blocks loaded from the file <fn>. used when reloading the file.
int file_invalidate_cache(const char* fn)
{
	// convert to native path to match fn_hash set by file_open
	char n_fn[PATH_MAX];
	file_make_native_path(fn, n_fn);

	const u32 fn_hash = fnv_hash(fn);
	// notes:
	// - don't use remove_if, because std::pair doesn't have operator=.
	// - erasing elements during loop is ok because map iterators aren't
	//   invalidated.
	for(BlockIt it = block_cache.begin(); it != block_cache.end(); ++it)
		if((it->first >> 32) == fn_hash)
			block_cache.erase(it);

	return 0;
}



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



// transfer <size> bytes, starting at <ofs>, to/from the given file.
// (read or write access was chosen at file-open time).
//
// if non-NULL, <cb> is called for each block transferred, passing <ctx>.
// it returns how much data was actually transferred, or a negative error
// code (in which case we abort the transfer and return that value).
// the callback mechanism is useful for user progress notification or
// processing data while waiting for the next I/O to complete
// (quasi-parallel, without the complexity of threads).
//
// return number of bytes transferred (see above), or a negative error code.
ssize_t file_io(File* f, off_t data_ofs, size_t data_size, void* data_buf,
	FileIOCB cb, uintptr_t ctx) // optional
{
#ifdef PARANOIA
debug_out("file_io fd=%d size=%d ofs=%d\n", f->fd, data_size, data_ofs);
#endif

	CHECK_FILE(f);

	const bool is_write = !!(f->flags & FILE_WRITE);
	const bool no_aio = !!(f->flags & FILE_NO_AIO);

	// when reading:
	if(!is_write)
	{
		// cut data_size off at EOF
		const ssize_t bytes_left = f->size - data_ofs;
		if(bytes_left < 0)
			return ERR_EOF;
		data_size = MIN(data_size, (size_t)bytes_left);
	}


	bool temp = (data_buf == 0);

	// sanity checks:
	// .. temp blocks requested AND
	//    (not reading OR using lowio OR no callback)
	if(temp && (is_write || no_aio || !cb))
	{
		debug_warn("file_io: invalid parameter");
		return ERR_INVALID_PARAM;
	}


	// only align if we allocate the buffer and in AIO mode
	const bool do_align = temp;
	const bool cache = temp;


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


	// skip aio code, use lowio
	if(no_aio)
		return lowio(f->fd, is_write, data_ofs, data_size, data_buf);


	//
	// now we read the file in 64 KiB chunks, N-buffered.
	// if reading from Zip, inflate while reading the next block.
	//

	const int MAX_IOS = 4;
	IOSlot ios[MAX_IOS] = { {0} };


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
			// get next free IO slot in ring buffer
			IOSlot* slot = &ios[head];
			memset(slot, 0, sizeof(IOSlot));
			head = (head + 1) % MAX_IOS;
			pending_ios++;

			off_t issue_ofs = (off_t)(actual_ofs + issue_cnt);

			void* buf = (temp)? 0 : (char*)actual_buf + issue_cnt;
			ssize_t issued = block_issue(f, slot, issue_ofs, buf);
#ifdef PARANOIA
			debug_out("file_io: block_issue: %d\n", issued);
#endif
			if(issued < 0)
				err = issued;
				// transfer failed - loop will now terminate after
				// waiting for all pending transfers to complete.

			issue_cnt += issued;
			if(issue_cnt >= actual_size)
				all_issued = true;

		}
		// IO pending: wait for it to complete, and process it.
		else if(pending_ios)
		{
			IOSlot* slot = &ios[tail];
			tail = (tail + 1) % MAX_IOS;
			pending_ios--;

			void* block = slot->cached_block;
			size_t size = BLOCK_SIZE;
			// wasn't in cache; it was issued, so wait for it
			bool from_cache;
			if(block)
				from_cache = true;
			else
			{
				from_cache = false;

				int ret = file_wait_io(&slot->io, block, size);
				if(ret < 0)
					err = (ssize_t)ret;
			}

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



// we have useable data from a previous temp buffer,
// but it needs to be copied into the user's buffer
if(from_cache && !temp)
	memcpy((char*)data_buf+raw_transferred_cnt, data, size);


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

			if(!from_cache)
				file_discard_io(&slot->io);

			if(temp)
			{
				// adding is allowed and we didn't take this from the cache already: add
				if(!slot->cached_block)
					block_add(slot->block_id, slot->temp_buf);
			}

		}
		// (all issued OR error) AND no pending transfers - done.
		else
			break;
	}

#ifdef PARANOIA
	debug_out("file_io: err=%d, actual_transferred_cnt=%d\n", err, actual_transferred_cnt);
#endif

	// failed (0 means callback reports it's finished)
	if(err < 0)
		return err;

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
int file_map(File* f, void*& p, size_t& size)
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
int file_unmap(File* f)
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
	void* p = f->mapping;
	f->mapping = 0;
	// don't clear f->size - the file is still open.

	return munmap(p, f->size);
}
