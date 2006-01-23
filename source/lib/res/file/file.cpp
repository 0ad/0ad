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
#include "../res.h"
#include "detect.h"
#include "adts.h"
#include "sysdep/sysdep.h"
#include "byte_order.h"
#include "lib/allocators.h"
#include "file.h"
#include "file_internal.h"

#include <vector>
#include <algorithm>

#include <string>

// reasonable guess. if too small, aio will do alignment.
const size_t SECTOR_SIZE = 4*KiB;

FileStats stats;


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



// convenience "class" that simplifies successively appending a filename to
// its parent directory. this avoids needing to allocate memory and calling
// strlen/strcat. used by wdetect and dir_next_ent.
// we want to maintain C compatibility, so this isn't a C++ class.

// write the given directory path into our buffer and set end/chars_left
// accordingly. <dir> need and should not end with a directory separator.
//
// note: <dir> and the filename set via pp_append_file are separated by
// '/'. this is to allow use on portable paths; the function otherwise
// does not care if paths are relative/portable/absolute.
LibError pp_set_dir(PathPackage* pp, const char* dir)
{
	// note: use / instead of DIR_SEP because pp->path is portable.
	const int len = snprintf(pp->path, ARRAY_SIZE(pp->path), "%s/", dir);
	// (need len below and must return an error code, not -1)
	if(len < 0)
		WARN_RETURN(ERR_PATH_LENGTH);
	pp->end = pp->path+len;
	pp->chars_left = ARRAY_SIZE(pp->path)-len;

	// check if <dir> actually did end with '/' (this will cause problems
	// when attempting to vfs_open the file).
	if(len >= 2)	// protect against underrun
		debug_assert(pp->end[-2] != '/' && pp->end[-2] != DIR_SEP);
	return ERR_OK;
}


// append the given filename to the directory established by the last
// pp_set_dir on this package. the whole path is accessible at pp->path.
LibError pp_append_file(PathPackage* pp, const char* fn)
{
	CHECK_ERR(strcpy_s(pp->end, pp->chars_left, fn));
	return ERR_OK;
}

//-----------------------------------------------------------------------------


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

static LibError convert_path(char* dst, const char* src, Conversion conv = TO_NATIVE)
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
			WARN_RETURN(ERR_PATH_LENGTH);

		char c = *s++;

		if(c == from)
			c = to;

		*d++ = c;

		// end of string - done
		if(c == '\0')
			return ERR_OK;
	}
}


// set by file_set_root_dir
static char n_root_dir[PATH_MAX];
static size_t n_root_dir_len;


// return the native equivalent of the given relative portable path
// (i.e. convert all '/' to the platform's directory separator)
// makes sure length < PATH_MAX.
LibError file_make_native_path(const char* path, char* n_path)
{
	return convert_path(n_path, path, TO_NATIVE);
}

// return the portable equivalent of the given relative native path
// (i.e. convert the platform's directory separators to '/')
// makes sure length < PATH_MAX.
LibError file_make_portable_path(const char* n_path, char* path)
{
	return convert_path(path, n_path, TO_PORTABLE);
}


// return the native equivalent of the given portable path
// (i.e. convert all '/' to the platform's directory separator).
// also prepends current directory => n_full_path is absolute.
// makes sure length < PATH_MAX.
LibError file_make_full_native_path(const char* path, char* n_full_path)
{
	debug_assert(path != n_full_path);	// doesn't work in-place

	strcpy_s(n_full_path, PATH_MAX, n_root_dir);
	return convert_path(n_full_path+n_root_dir_len, path, TO_NATIVE);
}

// return the portable equivalent of the given relative native path
// (i.e. convert the platform's directory separators to '/')
// n_full_path is absolute; if it doesn't match the current dir, fail.
// (note: portable paths are always relative to the file root dir).
// makes sure length < PATH_MAX.
LibError file_make_full_portable_path(const char* n_full_path, char* path)
{
	debug_assert(path != n_full_path);	// doesn't work in-place

	if(strncmp(n_full_path, n_root_dir, n_root_dir_len) != 0)
		return ERR_PATH_NOT_FOUND;
	return convert_path(path, n_full_path+n_root_dir_len, TO_PORTABLE);
}


// establish the root directory from <rel_path>, which is treated as
// relative to the executable's directory (determined via argv[0]).
// all relative file paths passed to this module will be based from
// this root dir. 
//
// example: executable in "$install_dir/system"; desired root dir is
// "$install_dir/data" => rel_path = "../data".
//
// argv[0] is necessary because the current directory is unknown at startup
// (e.g. it isn't set when invoked via batch file), and this is the
// easiest portable way to find our install directory.
//
// can only be called once, by design (see below). rel_path is trusted.
LibError file_set_root_dir(const char* argv0, const char* rel_path)
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
	if(sys_get_executable_name(n_path, PATH_MAX) < 0)
	{
		// .. failed; use argv[0]
		if(!realpath(argv0, n_path))
		{
			msg = "realpath(argv[0]) failed";
			goto fail;
		}
	}

	// make sure it's valid
	if(access(n_path, X_OK) < 0)
	{
		msg = "ERR_FILE_ACCESS";
		goto fail;
	}

	// strip executable name, append rel_path, convert to native
	char* fn = strrchr(n_path, DIR_SEP);
	if(!fn)
	{
		msg = "realpath returned an invalid path?";
		goto fail;
	}
	RETURN_ERR(file_make_native_path(rel_path, fn+1));

	// get actual root dir - previous n_path may include ".."
	// (slight optimization, speeds up path lookup)
	if(!realpath(n_path, n_root_dir))
		goto fail;
	// .. append DIR_SEP to simplify code that uses n_root_dir
	//    (note: already 0-terminated, since it's static)
	n_root_dir_len = strlen(n_root_dir)+1;	// +1 for trailing DIR_SEP
	n_root_dir[n_root_dir_len-1] = DIR_SEP;
	return ERR_OK;

	}

fail:
	debug_warn("failed");
	if(msg)
	{
		debug_printf("%s: %s\n", __func__, msg);
		return ERR_FAIL;
	}

	return LibError_from_errno();
}


//-----------------------------------------------------------------------------

// layer on top of POSIX opendir/readdir/closedir that handles
// portable -> native path conversion, ignores non-file/directory entries,
// and additionally returns the file status (size and mtime).

// rationale: see DirIterator definition in header.
struct DirIterator_
{
	DIR* os_dir;

	// to support stat(), we need to either chdir or store the complete path.
	// the former is unacceptable because it isn't thread-safe. therefore,
	// we latch dir_open's path and append entry name every dir_next_ent call.
	// this is also the storage to which DirEnt.name points!
	// PathPackage avoids repeated memory allocs and strlen() overhead.
	PathPackage pp;
};

cassert(sizeof(DirIterator_) <= sizeof(DirIterator));


// prepare to iterate (once) over entries in the given directory.
// returns a negative error code or 0 on success, in which case <d> is
// ready for subsequent dir_next_ent calls and must be freed via dir_close.
LibError dir_open(const char* P_path, DirIterator* d_)
{
	DirIterator_* d = (DirIterator_*)d_;

	char n_path[PATH_MAX];
	// HACK: allow calling with a full (absolute) native path.
	// (required by wdetect).
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

	d->os_dir = opendir(n_path);
	if(!d->os_dir)
		CHECK_ERR(LibError_from_errno());

	RETURN_ERR(pp_set_dir(&d->pp, n_path));
	return ERR_OK;
}


// return ERR_DIR_END if all entries have already been returned once,
// another negative error code, or 0 on success, in which case <ent>
// describes the next (order is unspecified) directory entry.
LibError dir_next_ent(DirIterator* d_, DirEnt* ent)
{
	DirIterator_* d = (DirIterator_*)d_;

get_another_entry:
	errno = 0;
	struct dirent* os_ent = readdir(d->os_dir);
	if(!os_ent)
	{
		if(errno)
			debug_warn("readdir failed");
		return ERR_DIR_END;
	}

	// copy os_ent.name[]; we need it for stat() #if !OS_WIN and
	// return it as ent.name (since os_ent.name[] is volatile).
	pp_append_file(&d->pp, os_ent->d_name);
	const char* name = d->pp.end;

	// get file information (mode, size, mtime)
	struct stat s;
#if OS_WIN
	// .. wposix readdir has enough information to return dirent
	//    status directly (much faster than calling stat).
	CHECK_ERR(readdir_stat_np(d->os_dir, &s));
#else
	// .. call regular stat().
	//    we need the full pathname for this. don't use vfs_path_append because
	//    it would unnecessarily call strlen.

	CHECK_ERR(stat(d->pp.path, &s));
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
	return ERR_OK;
}


// indicate the directory iterator is no longer needed; all resources it
// held are freed.
LibError dir_close(DirIterator* d_)
{
	DirIterator_* d = (DirIterator_*)d_;
	WARN_ERR(closedir(d->os_dir));
	return ERR_OK;
}


static bool dirent_less(const DirEnt* d1, const DirEnt* d2)
{
	return strcmp(d1->name, d2->name) < 0;
}


// call <cb> for each file and subdirectory in <dir> (alphabetical order),
// passing the entry name (not full path!), stat info, and <user>.
//
// first builds a list of entries (sorted) and remembers if an error occurred.
// if <cb> returns non-zero, abort immediately and return that; otherwise,
// return first error encountered while listing files, or 0 on success.
//
// rationale:
//   this makes file_enum and zip_enum slightly incompatible, since zip_enum
//   returns the full path. that's necessary because VFS zip_cb
//   has no other way of determining what VFS dir a Zip file is in,
//   since zip_enum enumerates all files in the archive (not only those
//   in a given dir). no big deal though, since add_ent has to
//   special-case Zip files anyway.
//   the advantage here is simplicity, and sparing callbacks the trouble
//   of converting from/to native path (we just give 'em the dirent name).
LibError file_enum(const char* P_path, const FileCB cb, const uintptr_t user)
{
	// pointer to DirEnt: faster sorting, but more allocs.
	typedef std::vector<const DirEnt*> DirEnts;
	typedef DirEnts::const_iterator DirEntCIt;
	typedef DirEnts::reverse_iterator DirEntRIt;
	// all entries are enumerated (adding to this container),
	// std::sort-ed, then all passed to cb.
	DirEnts dirents;
	dirents.reserve(125);	// preallocate for efficiency

	LibError stat_err = ERR_OK;	// first error encountered by stat()
	LibError cb_err   = ERR_OK;	// first error returned by cb

	DirIterator d;
	CHECK_ERR(dir_open(P_path, &d));

	DirEnt ent;
	for(;;)	// instead of while() to avoid warnings
	{
		LibError ret = dir_next_ent(&d, &ent);
		if(ret == ERR_DIR_END)
			break;
		if(!stat_err)
			stat_err = ret;

		const size_t size = sizeof(DirEnt)+strlen(ent.name)+1;
		DirEnt* p_ent = (DirEnt*)malloc(size);
		if(!p_ent)
		{
			stat_err = ERR_NO_MEM;
			goto fail;
		}
		p_ent->size  = ent.size;
		p_ent->mtime = ent.mtime;
		p_ent->name  = (const char*)p_ent + sizeof(DirEnt);
		strcpy((char*)p_ent->name, ent.name);	// safe
		dirents.push_back(p_ent);
	}

	std::sort(dirents.begin(), dirents.end(), dirent_less);

	// call back for each entry (now sorted)
	{
	struct stat s;
	memset(&s, 0, sizeof(s));
	const uintptr_t memento = 0;	// there is nothing we
	for(DirEntCIt it = dirents.begin(); it != dirents.end(); ++it)
	{
		const DirEnt* ent = *it;
		s.st_mode  = (ent->size == -1)? S_IFDIR : S_IFREG;
		s.st_size  = ent->size;
		s.st_mtime = ent->mtime;
		LibError ret = cb(ent->name, &s, memento, user);
		if(ret != INFO_CB_CONTINUE)
		{
			cb_err = ret;	// first error (since we now abort)
			break;
		}
	}
	}

fail:
	WARN_ERR(dir_close(&d));

	// free all memory (can't do in loop above because it may be aborted).
	for(DirEntRIt rit = dirents.rbegin(); rit != dirents.rend(); ++rit)
		free((void*)(*rit));

	if(cb_err != ERR_OK)
		return cb_err;
	return stat_err;
}


// get file information. output param is zeroed on error.
LibError file_stat(const char* path, struct stat* s)
{
	memset(s, 0, sizeof(struct stat));

	char n_path[PATH_MAX+1];
	CHECK_ERR(file_make_full_native_path(path, n_path));

	errno = 0;
	return LibError_from_posix(stat(n_path, s));
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


LibError file_validate(const File* f)
{
	if(!f)
		return ERR_INVALID_PARAM;
	else if(f->fd < 0)
		return ERR_1;
	// mapped but refcount is invalid
	else if((f->mapping != 0) ^ (f->map_refs != 0))
		return ERR_2;
	// atom_fn not set
#ifndef NDEBUG
	else if(!f->fc.atom_fn)
		return ERR_3;
#endif

	return ERR_OK;
}

// rationale: we want a constant-time IsAtomFn(string pointer) lookup:
// this avoids any overhead of calling file_make_unique_fn_copy on
// already-atomized strings. that requires allocating from one contiguous
// arena, which is also more memory-efficient than the heap (no headers).
static Pool atom_pool;

// allocate a copy of P_fn in our string pool. strings are equal iff
// their addresses are equal, thus allowing fast comparison.
const char* file_make_unique_fn_copy(const char* P_fn, size_t fn_len)
{
/*
const char* slash = strrchr(P_fn, '/');
if(slash&&!stricmp(slash+1, "proptest.PMD"))
debug_break();
*/
	// early out: if already an atom, return immediately.
	if(pool_contains(&atom_pool, (void*)P_fn))
		return P_fn;

	// allow for Pascal-style strings (e.g. from Zip file header)
	if(!fn_len)
		fn_len = strlen(P_fn);

	const char* unique_fn;

	// check if already allocated; return existing copy if so.
	//
	// rationale: the entire storage could be done via container,
	// rather than simply using it as a lookup mapping.
	// however, DynHashTbl together with Pool (see above) is more efficient.
	typedef DynHashTbl<const char*, const char*> AtomMap;
	static AtomMap atom_map;
	unique_fn = atom_map.find(P_fn);
	if(unique_fn)
	{
debug_assert(!strcmp(P_fn, unique_fn));
		return unique_fn;
	}

	unique_fn = (const char*)pool_alloc(&atom_pool, fn_len+1);
	if(!unique_fn)
		return 0;
	memcpy2((void*)unique_fn, P_fn, fn_len);
	((char*)unique_fn)[fn_len] = '\0';

	atom_map.insert(unique_fn, unique_fn);

	FILE_STATS_NOTIFY_UNIQUE_FILE();
	return unique_fn;
}

static inline void atom_init()
{
	pool_create(&atom_pool, 8*MiB, POOL_VARIABLE_ALLOCS);
}

static inline void atom_shutdown()
{
	(void)pool_destroy(&atom_pool);
}



LibError file_open(const char* P_fn, const uint flags, File* f)
{
	// zero output param in case we fail below.
	memset(f, 0, sizeof(*f));

	if(flags > FILE_FLAG_MAX)
		return ERR_INVALID_PARAM;

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
			return ERR_FILE_NOT_FOUND;
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
			return ERR_NOT_FILE;
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
		return ERR_FILE_ACCESS;

	f->fc.flags = flags;
	f->fc.size  = size;
	f->fc.atom_fn  = file_make_unique_fn_copy(P_fn, 0);
	f->mapping  = 0;
	f->map_refs = 0;
	f->fd       = fd;
	CHECK_FILE(f);

	return ERR_OK;
}


LibError file_close(File* f)
{
	CHECK_FILE(f);

	// make sure the mapping is actually freed,
	// regardless of how many references remain.
	if(f->map_refs > 1)
		f->map_refs = 1;
	if(f->mapping)	// only free if necessary (unmap complains if not mapped)
		file_unmap(f);

	// return final file size (required by VFS after writing files).
	// this is much easier than updating when writing, because we'd have
	// to add accounting code to both (sync and async) paths.
	f->fc.size = lseek(f->fd, 0, SEEK_END);

	// (check fd to avoid BoundsChecker warning about invalid close() param)
	if(f->fd != -1)
	{
		close(f->fd);
		f->fd = -1;
	}

	// wipe out any cached blocks. this is necessary to cover the (rare) case
	// of file cache contents predating the file write.
	if(f->fc.flags & FILE_WRITE)
		file_cache_invalidate(f->fc.atom_fn);

	return ERR_OK;
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

	const int prot = (f->fc.flags & FILE_WRITE)? PROT_WRITE : PROT_READ;

	// already mapped - increase refcount and return previous mapping.
	if(f->mapping)
	{
		// prevent overflow; if we have this many refs, should find out why.
		if(f->map_refs >= MAX_MAP_REFS)
			WARN_RETURN(ERR_LIMIT);
		f->map_refs++;
		goto have_mapping;
	}

	// don't allow mapping zero-length files (doesn't make sense,
	// and BoundsChecker warns about wposix mmap failing).
	// then again, don't complain, because this might happen when mounting
	// a dir containing empty files; each is opened as a Zip file.
	if(f->fc.size == 0)
		return ERR_FAIL;

	errno = 0;
	void* start = 0;	// system picks start address
	f->mapping = mmap(start, f->fc.size, prot, MAP_PRIVATE, f->fd, (off_t)0);
	if(f->mapping == MAP_FAILED)
		return LibError_from_errno();

	f->map_refs = 1;

have_mapping:
	p = f->mapping;
	size = f->fc.size;
	return ERR_OK;
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

	// file is not currently mapped
	if(f->map_refs == 0)
	{
		debug_warn("not currently mapped");
		return ERR_FAIL;
	}

	// still more than one reference remaining - done.
	if(--f->map_refs > 0)
		return ERR_OK;

	// no more references: remove the mapping
	void* p = f->mapping;
	f->mapping = 0;
	// don't clear f->fc.size - the file is still open.

	errno = 0;
	return LibError_from_posix(munmap(p, f->fc.size));
}


LibError file_init()
{
	atom_init();
	file_cache_init();
	return ERR_OK;
}

LibError file_shutdown()
{
	FILE_STATS_DUMP();
	atom_shutdown();
	file_io_shutdown();
	return ERR_OK;
}
