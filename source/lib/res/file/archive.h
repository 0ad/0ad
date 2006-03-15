// Zip archiving on top of ZLib.
//
// Copyright (c) 2003-2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef ARCHIVE_H__
#define ARCHIVE_H__

#include "../handle.h"
#include "file.h"			// FileCB for afile_enum
#include "compression.h"	// CompressionMethod


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

struct AFile
{
	FileCommon fc;

	off_t ofs;	// in archive
	off_t csize;
	CompressionMethod method;

	off_t last_cofs;	// in compressed file

	Handle ha;
	uintptr_t ctx;

	// this AFile has been successfully afile_map-ped, i.e. reference
	// count of the archive's mapping has been increased.
	// we need to undo that when closing it.
	uint is_mapped : 1;
};

// get file status (size, mtime). output param is zeroed on error.
extern LibError afile_stat(Handle ha, const char* fn, struct stat* s);

// open file, and fill *zf with information about it.
// return < 0 on error (output param zeroed). 
extern LibError afile_open(Handle ha, const char* fn, uintptr_t memento, int flags, AFile* af);

// close file.
extern LibError afile_close(AFile* af);

extern LibError afile_validate(const AFile* af);


//
// asynchronous read
//

struct AFileIo
{
	FileIo io;

	uintptr_t ctx;

	size_t max_output_size;
	void* user_buf;
};

// begin transferring <size> bytes, starting at <ofs>. get result
// with afile_io_wait; when no longer needed, free via afile_io_discard.
extern LibError afile_io_issue(AFile* af, off_t ofs, size_t size, void* buf, AFileIo* io);

// indicates if the IO referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
extern int afile_io_has_completed(AFileIo* io);

// wait until the transfer <io> completes, and return its buffer.
// output parameters are zeroed on error.
extern LibError afile_io_wait(AFileIo* io, void*& p, size_t& size);

// finished with transfer <io> - free its buffer (returned by afile_io_wait)
extern LibError afile_io_discard(AFileIo* io);

extern LibError afile_io_validate(const AFileIo* io);


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
extern ssize_t afile_read(AFile* af, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb = 0, uintptr_t ctx = 0);


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
extern LibError afile_map(AFile* af, void*& p, size_t& size);

// remove the mapping of file <zf>; fail if not mapped.
//
// the mapping will be removed (if still open) when its archive is closed.
// however, map/unmap calls should be paired so that the archive mapping
// may be removed when no longer needed.
extern LibError afile_unmap(AFile* af);


//
// archive creation
//

// array of pointers to VFS filenames (including path), terminated by a
// NULL entry.
typedef const char** Filenames;

// rationale: this is fairly lightweight and simple, so we don't bother
// making it opaque.
struct ArchiveBuildState
{
	ZipArchive* za;
	uintptr_t ctx;
	Filenames V_fns;
	size_t num_files;	// number of filenames in V_fns (excluding final 0)
	size_t i;
};

extern LibError archive_build_init(const char* P_archive_filename, Filenames V_fns,
	ArchiveBuildState* ab);

// create an archive (overwriting previous file) and fill it with the given
// files. compression method is chosen intelligently based on extension and
// file entropy / achieved compression ratio.
extern int archive_build_continue(ArchiveBuildState* ab);

extern void archive_build_cancel(ArchiveBuildState* ab);

extern LibError archive_build(const char* P_archive_filename, Filenames V_fns);


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
// note: AFile (state of a currently open file) is separate because
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
// return INFO_CB_CONTINUE to continue calling; anything else will cause
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
