/**
 * =========================================================================
 * File        : file.cpp
 * Project     : 0 A.D.
 * Description : file layer on top of POSIX. avoids the need for
 *             : absolute paths.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2006 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"

#include "lib/lib.h"
#include "lib/adts.h"
#include "lib/sysdep/sysdep.h"
#include "lib/byte_order.h"
#include "lib/allocators.h"
#include "file_internal.h"

#include <vector>
#include <algorithm>

#include <string>


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
//   decompressed and decoded (e.g. TGA colour swapping) data; mmap would
//   keep the original, compressed data in memory, which doesn't help.
//   we bypass the OS file cache via aio, and store partial blocks here (*);
//   higher level routines will cache the actual useful data.
//   * requests for part of a block are usually followed by another.



// layer on top of POSIX opendir/readdir/closedir that handles
// portable -> native path conversion, ignores non-file/directory entries,
// and additionally returns the file status (size and mtime).

// rationale: see DirIterator definition in header.
struct PosixDirIterator
{
	DIR* os_dir;

	// to support stat(), we need to either chdir or store the complete path.
	// the former is unacceptable because it isn't thread-safe. therefore,
	// we latch dir_open's path and append entry name every dir_next_ent call.
	// this is also the storage to which DirEnt.name points!
	// PathPackage avoids repeated memory allocs and strlen() overhead.
	//
	// it can't be stored here directly because then the struct would
	// no longer fit in HDATA; we'll allocate it separately.
	PathPackage* pp;
};

cassert(sizeof(PosixDirIterator) <= DIR_ITERATOR_OPAQUE_SIZE);

static SingleAllocator<PathPackage> pp_allocator;


// prepare to iterate (once) over entries in the given directory.
// if INFO_OK is returned, <d> is ready for subsequent dir_next_ent calls and
// must be freed via dir_close.
LibError dir_open(const char* P_path, DirIterator* di)
{
	PosixDirIterator* pdi = (PosixDirIterator*)di->opaque;

	char n_path[PATH_MAX];
	// HACK: allow calling with a full (absolute) native path.
	// (required by wdll_ver).
#if OS_WIN
	if(P_path[1] == ':' && P_path[2] == '\\')
		strcpy_s(n_path, ARRAY_SIZE(n_path), P_path);
	else
#endif
	{
		// note: copying to n_path and then pp.path is inefficient but
		// more clear/robust. this is only called a few hundred times anyway.
		RETURN_ERR(file_make_full_native_path(P_path, n_path));
	}

	pdi->pp = pp_allocator.alloc();
	if(!pdi->pp)
		WARN_RETURN(ERR_NO_MEM);

	errno = 0;
	pdi->os_dir = opendir(n_path);
	if(!pdi->os_dir)
		return LibError_from_errno();

	(void)path_package_set_dir(pdi->pp, n_path);
	return INFO_OK;
}


// return ERR_DIR_END if all entries have already been returned once,
// another negative error code, or INFO_OK on success, in which case <ent>
// describes the next (order is unspecified) directory entry.
LibError dir_next_ent(DirIterator* di, DirEnt* ent)
{
	PosixDirIterator* pdi = (PosixDirIterator*)di->opaque;

get_another_entry:
	errno = 0;
	struct dirent* os_ent = readdir(pdi->os_dir);
	if(!os_ent)
	{
		// no error, just no more entries to return
		if(!errno)
			return ERR_DIR_END;	// NOWARN
		return LibError_from_errno();
	}

	// copy os_ent.name[]; we need it for stat() #if !OS_WIN and
	// return it as ent.name (since os_ent.name[] is volatile).
	path_package_append_file(pdi->pp, os_ent->d_name);
	const char* name = pdi->pp->end;

	// get file information (mode, size, mtime)
	struct stat s;
#if OS_WIN
	// .. wposix readdir has enough information to return dirent
	//    status directly (much faster than calling stat).
	CHECK_ERR(readdir_stat_np(pdi->os_dir, &s));
#else
	// .. call regular stat().
	//    we need the full pathname for this. don't use path_append because
	//    it would unnecessarily call strlen.

	CHECK_ERR(stat(pdi->pp->path, &s));
#endif

	// skip "undesirable" entries that POSIX readdir returns:
	if(S_ISDIR(s.st_mode))
	{
		// .. dummy directory entries ("." and "..")
		if(name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))
			goto get_another_entry;

		s.st_size = -1;	// our way of indicating it's a directory
	}
	// .. neither dir nor file
	else if(!S_ISREG(s.st_mode))
		goto get_another_entry;

	ent->size  = s.st_size;
	ent->mtime = s.st_mtime;
	ent->name  = name;
	return INFO_OK;
}


// indicate the directory iterator is no longer needed; all resources it
// held are freed.
LibError dir_close(DirIterator* di)
{
	PosixDirIterator* pdi = (PosixDirIterator*)di->opaque;
	pp_allocator.release(pdi->pp);

	errno = 0;
	if(closedir(pdi->os_dir) < 0)
		return LibError_from_errno();
	return INFO_OK;
}


LibError dir_create(const char* P_path, mode_t mode)
{
	char N_path[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(P_path, N_path));

	struct stat s;
	int ret = stat(N_path, &s);
	if(ret == 0)
		return INFO_ALREADY_EXISTS;

	errno = 0;
	ret = mkdir(N_path, mode);
	return LibError_from_posix(ret);
}



// get file information. output param is zeroed on error.
static LibError file_stat_impl(const char* fn, struct stat* s, bool warn_if_failed = true)
{
	memset(s, 0, sizeof(struct stat));

	char N_fn[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(fn, N_fn));

	errno = 0;
	int ret = stat(N_fn, s);
	return LibError_from_posix(ret, warn_if_failed);
}

LibError file_stat(const char* fn, struct stat* s)
{
	return file_stat_impl(fn, s);
}

// does the given file exist? (implemented via file_stat)
bool file_exists(const char* fn)
{
	struct stat s;
	const bool warn_if_failed = false;
	return file_stat_impl(fn, &s, warn_if_failed) == INFO_OK;
}


// permanently delete the file. be very careful with this!
LibError file_delete(const char* fn)
{
	char N_fn[PATH_MAX+1];
	RETURN_ERR(file_make_full_native_path(fn, N_fn));

	errno = 0;
	int ret = unlink(N_fn);
	return LibError_from_posix(ret);
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


struct PosixFile
{
	int fd;

	// for reference counted memory-mapping
	void* mapping;
	uint map_refs;
};
cassert(sizeof(PosixFile) < FILE_OPAQUE_SIZE);

int file_fd_from_PosixFile(File* f)
{
	const PosixFile* pf = (const PosixFile*)f->opaque;
	return pf->fd;
}


LibError file_validate(const File* f)
{
	if(!f)
		WARN_RETURN(ERR_INVALID_PARAM);
	const PosixFile* pf = (PosixFile*)f->opaque;
	if(pf->fd < 0)
		WARN_RETURN(ERR_1);
	// mapped but refcount is invalid
	else if((pf->mapping != 0) ^ (pf->map_refs != 0))
		WARN_RETURN(ERR_2);
	// note: don't check atom_fn - that complains at the end of
	// file_open if flags & FILE_DONT_SET_FN and has no benefit, really.

	return INFO_OK;
}


LibError file_open(const char* P_fn, uint flags, File* f)
{
	// zero output param in case we fail below.
	memset(f, 0, sizeof(*f));

	if(flags > FILE_FLAG_ALL)
		WARN_RETURN(ERR_INVALID_PARAM);

	char N_fn[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(P_fn, N_fn));

	// don't stat if opening for writing - the file may not exist yet
	off_t size = 0;

	int oflag = O_RDONLY;
	if(flags & FILE_WRITE)
		oflag = O_WRONLY|O_CREAT|O_TRUNC;
	// read access requested
	else
	{
		// get file size
		struct stat s;
		if(stat(N_fn, &s) < 0)
			WARN_RETURN(ERR_TNODE_NOT_FOUND);
		size = s.st_size;

		// note: despite increased overhead, the AIO read method is still
		// significantly faster, even with small files.
		// we therefore don't automatically disable AIO.
		// notes:
		// - up to 32KB can be read by one SCSI request.
		// - flags are stored below and will influence file_io.
		//if(size <= 32*KiB)
		//	flags |= FILE_NO_AIO;

		// make sure <N_fn> is a regular file
		if(!S_ISREG(s.st_mode))
			WARN_RETURN(ERR_TNODE_WRONG_TYPE);
	}

#if OS_WIN
	if(flags & FILE_TEXT)
		oflag |= O_TEXT_NP;
	else
		oflag |= O_BINARY_NP;

	// if AIO is disabled at user's behest, so inform wposix.
	if(flags & FILE_NO_AIO)
		oflag |= O_NO_AIO_NP;
#endif

	int fd = open(N_fn, oflag, S_IRWXO|S_IRWXU|S_IRWXG);
	if(fd < 0)
		WARN_RETURN(ERR_FILE_ACCESS);

	f->flags = flags;
	f->size  = size;
	// see FILE_DONT_SET_FN decl.
	if(!(flags & FILE_DONT_SET_FN))
		f->atom_fn = file_make_unique_fn_copy(P_fn);
	PosixFile* pf = (PosixFile*)f->opaque;
	pf->mapping  = 0;
	pf->map_refs = 0;
	pf->fd       = fd;
	CHECK_FILE(f);

	return INFO_OK;
}


LibError file_close(File* f)
{
	CHECK_FILE(f);
	PosixFile* pf = (PosixFile*)f->opaque;

	// make sure the mapping is actually freed,
	// regardless of how many references remain.
	if(pf->map_refs > 1)
		pf->map_refs = 1;
	if(pf->mapping)	// only free if necessary (unmap complains if not mapped)
		file_unmap(f);

	// return final file size (required by VFS after writing files).
	// this is much easier than updating when writing, because we'd have
	// to add accounting code to both (sync and async) paths.
	f->size = lseek(pf->fd, 0, SEEK_END);

	// (check fd to avoid BoundsChecker warning about invalid close() param)
	if(pf->fd != -1)
	{
		close(pf->fd);
		pf->fd = -1;
	}

	// wipe out any cached blocks. this is necessary to cover the (rare) case
	// of file cache contents predating the file write.
	if(f->flags & FILE_WRITE)
		file_cache_invalidate(f->atom_fn);

	return INFO_OK;
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
LibError file_map(File* f, void*& p, size_t& size)
{
	p = 0;
	size = 0;

	CHECK_FILE(f);
	PosixFile* pf = (PosixFile*)f->opaque;

	const int prot = (f->flags & FILE_WRITE)? PROT_WRITE : PROT_READ;

	// already mapped - increase refcount and return previous mapping.
	if(pf->mapping)
	{
		// prevent overflow; if we have this many refs, should find out why.
		if(pf->map_refs >= MAX_MAP_REFS)
			WARN_RETURN(ERR_LIMIT);
		pf->map_refs++;
		goto have_mapping;
	}

	// don't allow mapping zero-length files (doesn't make sense,
	// and BoundsChecker warns about wposix mmap failing).
	// then again, don't complain, because this might happen when mounting
	// a dir containing empty files; each is opened as a Zip file.
	if(f->size == 0)
		return ERR_FAIL;	// NOWARN

	errno = 0;
	pf->mapping = mmap(0, f->size, prot, MAP_PRIVATE, pf->fd, (off_t)0);
	if(pf->mapping == MAP_FAILED)
		return LibError_from_errno();

	pf->map_refs = 1;

have_mapping:
	p = pf->mapping;
	size = f->size;
	return INFO_OK;
}


// decrement the reference count for the mapping belonging to file <f>.
// fail if there are no references; remove the mapping if the count reaches 0.
//
// the mapping will be removed (if still open) when its file is closed.
// however, map/unmap calls should still be paired so that the mapping
// may be removed when no longer needed.
LibError file_unmap(File* f)
{
	CHECK_FILE(f);
	PosixFile* pf = (PosixFile*)f->opaque;

	// file is not currently mapped
	if(pf->map_refs == 0)
		WARN_RETURN(ERR_NOT_MAPPED);

	// still more than one reference remaining - done.
	if(--pf->map_refs > 0)
		return INFO_OK;

	// no more references: remove the mapping
	void* p = pf->mapping;
	pf->mapping = 0;
	// don't clear f->size - the file is still open.

	errno = 0;
	int ret = munmap(p, f->size);
	return LibError_from_posix(ret);
}


LibError file_init()
{
	path_init();
	file_cache_init();
	file_io_init();

	// convenience
	file_sector_size = sys_max_sector_size();

	return INFO_OK;
}

LibError file_shutdown()
{
	stats_dump();
	path_shutdown();
	file_io_shutdown();
	return INFO_OK;
}
