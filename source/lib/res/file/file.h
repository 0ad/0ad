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

#ifndef FILE_H
#define FILE_H

#include "posix.h"		// struct stat

extern LibError file_init();

// convenience "class" that simplifies successively appending a filename to
// its parent directory. this avoids needing to allocate memory and calling
// strlen/strcat. used by wdetect and dir_next_ent.
// we want to maintain C compatibility, so this isn't a C++ class.

struct PathPackage
{
	char* end;
	size_t chars_left;
	char path[PATH_MAX];
};

// write the given directory path into our buffer and set end/chars_left
// accordingly. <dir> need and should not end with a directory separator.
//
// note: <dir> and the filename set via pp_append_file are separated by
// '/'. this is to allow use on portable paths; the function otherwise
// does not care if paths are relative/portable/absolute.
extern LibError pp_set_dir(PathPackage* pp, const char* dir);

// append the given filename to the directory established by the last
// pp_set_dir on this package. the whole path is accessible at pp->path.
extern LibError pp_append_file(PathPackage* pp, const char* file);


// is s2 a subpath of s1, or vice versa? used by VFS and wdir_watch.
// works for portable and native paths.
extern bool file_is_subpath(const char* s1, const char* s2);


//
// path conversion functions (native <--> portable),
// for external libraries that require the real filename.
//
// replaces '/' with platform's directory separator and vice versa.
// verifies path length < PATH_MAX (otherwise return ERR_PATH_LENGTH).
//

// relative paths (relative to root dir)
extern LibError file_make_native_path(const char* path, char* n_path);
extern LibError file_make_portable_path(const char* n_path, char* path);

// as above, but with full native paths (portable paths are always relative).
// prepends current directory, resp. makes sure it matches the given path.
extern LibError file_make_full_native_path(const char* path, char* n_full_path);
extern LibError file_make_full_portable_path(const char* n_full_path, char* path);


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
extern LibError file_set_root_dir(const char* argv0, const char* rel_path);


// allocate a copy of P_fn in our string pool. strings are equal iff
// their addresses are equal, thus allowing fast comparison.
// fn_len can be 0 to indicate P_fn is a null-terminated C string
// (normal case) or the string length [characters].
extern const char* file_make_unique_fn_copy(const char* P_fn, size_t fn_len);


//
// dir_next_ent
//

// layer on top of POSIX opendir/readdir/closedir that handles
// portable -> native path conversion, ignores non-file/directory entries,
// and additionally returns the file status (size and mtime).

// directory state initialized by dir_open.
// rationale: some private storage apart from opendir's DIR* is required
// to support stat(). we prefer having the caller reserve room (on the stack)
// rather than allocating dynamically (less efficient or more complicated).
//
// this is an opaque struct to avoid exposing our internals and insulate
// user code against changes; we verify at compile-time that the
// public/private definitions match.
// note: cannot just typedef to DirIterator_ because other modules
// instantiate this.
struct DirIterator
{
	char opaque[PATH_MAX+32];
};

// information about a directory entry filled by dir_next_ent.
struct DirEnt
{
	// we want to keep this as small as possible because
	// file_enum allocates one copy for each file in the directory.

	// store only required stat fields (in VC's order of decl)
	off_t  size;
	time_t mtime;

	// name (not including path!) of this entry.
	// valid until a subsequent dir_next_ent or dir_close call for the
	// current dir state.
	// rationale: we don't want to return a pointer to a copy because
	// users would have to free it (won't happen).
	const char* name;
};

// return [bool] indicating whether the given DirEnt* (filled by
// dir_next_ent) represents a directory.
#define DIRENT_IS_DIR(p_ent) ((p_ent)->size == -1)

// prepare to iterate (once) over entries in the given directory.
// returns a negative error code or 0 on success, in which case <d> is
// ready for subsequent dir_next_ent calls and must be freed via dir_close.
extern LibError dir_open(const char* P_path, DirIterator* d);

// return ERR_DIR_END if all entries have already been returned once,
// another negative error code, or 0 on success, in which case <ent>
// describes the next (order is unspecified) directory entry.
extern LibError dir_next_ent(DirIterator* d, DirEnt* ent);

// indicate the directory iterator is no longer needed; all resources it
// held are freed.
extern LibError dir_close(DirIterator* d);


// called by file_enum for each entry in the directory.
// name doesn't include path!
// return INFO_CB_CONTINUE to continue calling; anything else will cause
// file_enum to abort and immediately return that value.
typedef LibError (*FileCB)(const char* name, const struct stat* s, uintptr_t memento, const uintptr_t user);

// call <cb> for each file and subdirectory in <dir> (alphabetical order),
// passing the entry name (not full path!), stat info, and <user>.
//
// first builds a list of entries (sorted) and remembers if an error occurred.
// if <cb> returns non-zero, abort immediately and return that; otherwise,
// return first error encountered while listing files, or 0 on success.
extern LibError file_enum(const char* dir, FileCB cb, uintptr_t user);



struct FileCommon
{
// keep offset of flags and size members in sync with struct AFile!
// it is accessed by VFS and must be the same for both (union).
// dirty, but necessary because VFile is pushing the HDATA size limit.
	uint flags;
	off_t size;

	// copy of the filename that was passed to file_open;
	// its address uniquely identifies it. used as key for file cache.
	const char* atom_fn;
};

struct File
{
	FileCommon fc;

	int fd;

	// for reference counted memory-mapping
	void* mapping;
	uint map_refs;
};


enum
{
	// write-only access; otherwise, read only.
	//
	// note: only allowing either reads or writes simplifies file cache
	// coherency (need only invalidate when closing a FILE_WRITE file).
	FILE_WRITE        = 0x01,

	// translate newlines: convert from/to native representation when
	// reading/writing. this is useful if files we create need to be
	// edited externally - e.g. Notepad requires \r\n.
	// caveats:
	// - FILE_NO_AIO must be set; translation is done by OS read()/write().
	// - not supported by POSIX, so this currently only has meaning on Win32.
	FILE_TEXT         = 0x02,

	// the file's contents aren't cached at a higher level; do so here.
	// we keep the file open (until the cache is "full enough"). if it
	// is loaded, we keep the buffer there to satisfy later loads.
	FILE_CACHE        = 0x04,

	// random access hint
	//	FILE_RANDOM       = 0x08,

	FILE_NO_AIO       = 0x10,

	FILE_CACHE_BLOCK  = 0x20,

	// sum of all flags above. used when validating flag parameters and
	// by zip.cpp because its flags live alongside these.
	FILE_FLAG_MAX     = 0x3F
};


// get file information. output param is zeroed on error.
extern LibError file_stat(const char* path, struct stat*);

extern LibError file_open(const char* fn, uint flags, File* f);

// note: final file size is calculated and returned in f->size.
// see implementation for rationale.
extern LibError file_close(File* f);

extern LibError file_validate(const File* f);


// remove all blocks loaded from the file <fn>. used when reloading the file.
extern LibError file_cache_invalidate(const char* fn);


//
// asynchronous IO
//

// this is a thin wrapper on top of the system AIO calls.
// IOs are carried out exactly as requested - there is no caching or
// alignment done here. rationale: see source.

struct FileIo
{
	void* cb;
};

// queue the IO; it begins after the previous ones (if any) complete.
//
// rationale: this interface is more convenient than implicitly advancing a
// file pointer because archive.cpp often accesses random offsets.
extern LibError file_io_issue(File* f, off_t ofs, size_t size, void* buf, FileIo* io);

// indicates if the given IO has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
extern int file_io_has_completed(FileIo* io);

// wait for the given IO to complete. passes back its buffer and size.
extern LibError file_io_wait(FileIo* io, const void*& p, size_t& size);

// indicates the IO's buffer is no longer needed and frees that memory.
extern LibError file_io_discard(FileIo* io);

extern LibError file_io_validate(const FileIo* io);


//
// synchronous IO
//

// block := power-of-two sized chunk of a file.
// all transfers are expanded to naturally aligned, whole blocks
// (this makes caching parts of files feasible; it is also much faster
// for some aio implementations, e.g. wposix).
const size_t FILE_BLOCK_SIZE = 16*KiB;


typedef const u8* FileIOBuf;

FileIOBuf* const FILE_BUF_TEMP = (FileIOBuf*)1;
const FileIOBuf FILE_BUF_ALLOC = (FileIOBuf)2;

extern FileIOBuf file_buf_alloc(size_t size, const char* atom_fn);
extern LibError file_buf_free(FileIOBuf buf);


// called by file_io after a block IO has completed.
// *bytes_processed must be set; file_io will return the sum of these values.
// example: when reading compressed data and decompressing in the callback,
// indicate #bytes decompressed.
// return value: INFO_CB_CONTINUE to continue calling; anything else:
//   abort immediately and return that.
// note: in situations where the entire IO is not split into blocks
// (e.g. when reading from cache or not using AIO), this is still called but
// for the entire IO. we do not split into fake blocks because it is
// advantageous (e.g. for decompressors) to have all data at once, if available
// anyway.
typedef LibError (*FileIOCB)(uintptr_t ctx, const void* block, size_t size, size_t* bytes_processed);

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
extern ssize_t file_io(File* f, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb = 0, uintptr_t ctx = 0);

extern ssize_t file_read_from_cache(const char* atom_fn, off_t ofs, size_t size,
	FileIOBuf* pbuf, FileIOCB cb, uintptr_t ctx);

//
// memory mapping
//

// useful for files that are too large to be loaded into memory,
// or if only (non-sequential) portions of a file are needed at a time.


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
extern LibError file_map(File* f, void*& p, size_t& size);

// decrement the reference count for the mapping belonging to file <f>.
// fail if there are no references; remove the mapping if the count reaches 0.
//
// the mapping will be removed (if still open) when its file is closed.
// however, map/unmap calls should still be paired so that the mapping
// may be removed when no longer needed.
extern LibError file_unmap(File* f);


extern LibError file_shutdown();

#endif	// #ifndef FILE_H
