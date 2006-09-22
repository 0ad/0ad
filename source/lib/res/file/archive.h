/**
 * =========================================================================
 * File        : archive.h
 * Project     : 0 A.D.
 * Description : provide access to archive "resources". allows
 *             : opening, reading from, and creating them.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ARCHIVE_H__
#define ARCHIVE_H__

#include "lib/res/handle.h"
#include "file.h"			// FileCB for afile_enum
#include "compression.h"	// CompressionMethod

namespace ERR
{
	const LibError IS_COMPRESSED = -110400;
}

// note: filenames are case-insensitive.


//
// archive
//

// open and return a handle to the archive indicated by <fn>.
// somewhat slow - each file is added to an internal index.
extern Handle archive_open(const char* fn);

// close the archive <ha> and set ha to 0
extern LibError archive_close(Handle& ha);

// successively call <cb> for each valid file in the archive <ha>,
// passing the complete path and <user>.
// if it returns a nonzero value, abort and return that, otherwise 0.
//
// FileCB's name parameter will be the full path and unique
// (i.e. returned by file_make_unique_fn_copy).
extern LibError archive_enum(const Handle ha, const FileCB cb, const uintptr_t user);


//
// file
//

// get file status (size, mtime). output param is zeroed on error.
extern LibError afile_stat(Handle ha, const char* fn, struct stat* s);

// open file, and fill *f with information about it.
// return < 0 on error (output param zeroed). 
extern LibError afile_open(Handle ha, const char* fn, uintptr_t memento, uint flags, File* f);

// close file.
extern LibError afile_close(File* f);

extern LibError afile_validate(const File* f);

extern LibError afile_open_vfs(const char* fn, uint flags, File* f, TFile* tf);

//
// asynchronous read
//

// begin transferring <size> bytes, starting at <ofs>. get result
// with afile_io_wait; when no longer needed, free via afile_io_discard.
extern LibError afile_io_issue(File* f, off_t ofs, size_t size, void* buf, FileIo* io);

// indicates if the IO referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
extern int afile_io_has_completed(FileIo* io);

// wait until the transfer <io> completes, and return its buffer.
// output parameters are zeroed on error.
extern LibError afile_io_wait(FileIo* io, void*& p, size_t& size);

// finished with transfer <io> - free its buffer (returned by afile_io_wait)
extern LibError afile_io_discard(FileIo* io);

extern LibError afile_io_validate(const FileIo* io);


//
// synchronous read
//

// read from the (possibly compressed) file <zf> as if it were a normal file.
// starting at the beginning of the logical (decompressed) file,
// skip <ofs> bytes of data; read the next <size> bytes into <buf>.
//
// if non-NULL, <cb> is called for each block read, passing <ctx>.
// if it returns a negative error code,
// the read is aborted and that value is returned.
// the callback mechanism is useful for user progress notification or
// processing data while waiting for the next I/O to complete
// (quasi-parallel, without the complexity of threads).
//
// return bytes read, or a negative error code.
extern ssize_t afile_read(File* f, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb = 0, uintptr_t ctx = 0);


//
// memory mapping
//

// useful for files that are too large to be loaded into memory,
// or if only (non-sequential) portions of a file are needed at a time.
//
// this is of course only possible for uncompressed files - compressed files
// would have to be inflated sequentially, which defeats the point of mapping.


// map the entire file <zf> into memory. mapping compressed files
// isn't allowed, since the compression algorithm is unspecified.
// output parameters are zeroed on failure.
//
// the mapping will be removed (if still open) when its archive is closed.
// however, map/unmap calls should still be paired so that the archive mapping
// may be removed when no longer needed.
extern LibError afile_map(File* f, void*& p, size_t& size);

// remove the mapping of file <zf>; fail if not mapped.
//
// the mapping will be removed (if still open) when its archive is closed.
// however, map/unmap calls should be paired so that the archive mapping
// may be removed when no longer needed.
extern LibError afile_unmap(File* f);



//
// interface for backends
//

// the archive-specific backends call back here for each file;
// this module provides storage for the file table.

enum ArchiveFileFlags
{
	// indicates ArchiveEntry.ofs points to a "local file header"
	// instead of the file data. a fixup routine is called upon
	// file open; it skips past LFH and clears this flag.
	// this is somewhat of a hack, but vital to archive open
	// performance. without it, we'd have to scan through the
	// entire Zip file, which can take *seconds*.
	// (we cannot use the information in CDFH, because its 'extra' field
	// has been observed to differ from that of the LFH)
	// by reading LFH when a file in archive is opened, the block cache
	// absorbs the IO cost because the file will likely be read anyway.
	ZIP_LFH_FIXUP_NEEDED = 1
};

// holds all per-file information extracted from the header.
// this is intended to work for all archive types.
//
// note: File* (state of a currently open file) is separate because
// some of its fields need not be stored here; we'd like to minimize
// size of the file table.
struct ArchiveEntry
{
	// these are returned by afile_stat:
	off_t ucsize;
	time_t mtime;

	// used in IO
	off_t ofs;
	off_t csize;
	CompressionMethod method;
	u32 crc;

	uint flags;	// ArchiveFileFlags

	const char* atom_fn;

	// why csize?
	// file I/O may be N-buffered, so it's good to know when the raw data
	// stops, or else we potentially overshoot by N-1 blocks.
	// if we do read too much though, nothing breaks - inflate would just
	// ignore it, since Zip files are compressed individually.
	//
	// we also need a way to check if a file is compressed (e.g. to fail
	// mmap requests if the file is compressed). packing a bit in ofs or
	// ucsize is error prone and ugly (1 bit less won't hurt though).
	// any other way will mess up the nice 2^n byte size anyway, so
	// might as well store csize.
};

// successively called for each valid file in the archive,
// passing the complete path and <user>.
// return INFO::CB_CONTINUE to continue calling; anything else will cause
// the caller to abort and immediately return that value.
//
// HACK: call back with negative index the first time; its abs. value is
// the number of entries in the archive. lookup needs to know this so it can
// preallocate memory. having lookup_init call z_get_num_files and then
// za_enum_files would require passing around a ZipInfo struct, or searching
// for the ECDR twice - both ways aren't nice. nor is expanding on demand -
// we try to minimize allocations (faster, less fragmentation).

// fn (filename) is not necessarily 0-terminated!
// loc is only valid during the callback! must be copied or saved.
typedef LibError (*CDFH_CB)(uintptr_t user, i32 i, const ArchiveEntry* loc, size_t fn_len);


struct Archive;

extern LibError archive_allocate_entries(Archive* a, size_t num_entries);
extern LibError archive_add_file(Archive* a, const ArchiveEntry* ent);

#endif	// #ifndef ARCHIVE_H__
