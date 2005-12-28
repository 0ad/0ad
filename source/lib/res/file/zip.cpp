// Zip archiving on top of ZLib.
//
// Copyright (c) 2003 Jan Wassenberg
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

// components:
// - za_*: Zip archive handling
//   passes the list of files in an archive to lookup.
// - lookup_*: file lookup
//   per archive: return file info (e.g. offset, size), given filename.
// - ZArchive_*: Handle-based container for archive info
//   owns archive file and its lookup mechanism.
// - inf_*: in-memory inflate routines (zlib wrapper)
//   decompresses blocks from file_io callback.
// - zip_*: file from Zip archive
//   uses lookup to get file information; holds inflate state.
// - sync and async I/O
//   uses file_* and inf_*.
// - file mapping


#include "precompiled.h"

#include <map>

#include <time.h>

#include "lib.h"
#include "zip.h"
#include "../res.h"
#include "byte_order.h"
#include "allocators.h"

#include "timer.h"


// provision for removing all ZLib code (all inflate calls will fail).
// used for checking DLL dependency; might also simulate corrupt Zip files.
//#define NO_ZLIB

#ifndef NO_ZLIB
# define ZLIB_DLL
# include <zlib.h>

# if MSC_VERSION
#  ifdef NDEBUG
#   pragma comment(lib, "zlib1.lib")
#  else
#   pragma comment(lib, "zlib1d.lib")
#  endif
# endif
#endif


///////////////////////////////////////////////////////////////////////////////
//
// za_*: Zip archive handling
// passes the list of files in an archive to lookup.
//
///////////////////////////////////////////////////////////////////////////////

static const off_t LFH_FIXUP = BIT(31);

// convenience container for location / size of file in archive.
// separate from ZFile to minimize size of file table.
struct ZLoc
{
	off_t ofs;		// bit 31 set if fixup needed
	off_t csize;	// = 0 if not compressed

	// these are returned by zip_stat:
	off_t ucsize;
	time_t mtime;

	const char* fn;

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


// Zip file data structures and signatures
static u32 cdfh_magic = FOURCC_LE('P','K','\1','\2');
static u32  lfh_magic = FOURCC_LE('P','K','\3','\4');
static u32 ecdr_magic = FOURCC_LE('P','K','\5','\6');

const size_t CDFH_SIZE = 46;
const size_t  LFH_SIZE = 30;
const size_t ECDR_SIZE = 22;

enum ZipCompressionMethod
{
	Z_CM_STORED  = 0,		// no compression
	Z_CM_DEFLATE = 8
};

struct LFH
{
	u32 magic;
	u16 x1;			// version needed
	u16 flags;
	u16 method;
	u32 mtime;		// last modified time (DOS FAT format)
	u32 crc;
	u32 csize;
	u32 ucsize;
	u16 fn_len;
	u16 extra_len;
};


struct CDFH
{
	u32 magic;
	u32 x1;			// versions
	u16 flags;
	u16 method;
	u32 mtime;		// last modified time (DOS FAT format)
	u32 crc;
	u32 csize;
	u32 ucsize;
	u16 fn_len;
	u16 e_len;
	u16 c_len;
	u32 x2;			// spanning
	u32 x3;			// attributes
	u32 lfh_ofs;
};


struct ECDR
{
	u32 magic;
	u8 x1[6];	// multiple-disk support
	u16 cd_entries;
	u32 cd_size;
	u32 cd_ofs;
	u16 comment_len;
};












// return false if file is obviously not a valid Zip archive,
// otherwise true. used as early-out test in lookup_init (see call site).
static inline bool za_is_header(const u8* file, size_t size)
{
	// make sure it's big enough to check the header and for
	// za_find_ecdr to succeed (if smaller, it's definitely bogus).
	if(size < ECDR_SIZE)
		return false;

	// check "header" (first LFH) signature
	return ((LFH*)file)->magic == lfh_magic;
}


// scan for and return a pointer to a Zip record, or 0 if not found.
// <start> is the expected position; we scan from there until EOF for
// the given ID (fourcc). <record_size> (includes ID field) bytes must
// remain before EOF - this makes sure the record is completely in the file.
// used by z_find_ecdr and z_extract_cdfh.
static const u8* za_find_id(const u8* file, size_t size, const u8* start, u32 magic, size_t record_size)
{
	ssize_t bytes_left = (ssize_t)((file+size) - start - record_size);

	const u8* p = start;
		// don't increment function argument directly,
		// so we can warn the user if we had to scan.

	while(bytes_left-- >= 0)
	{
		// found it
		if(*(u32*)p == magic)
		{
#ifndef NDEBUG
			if(p != start)
				debug_warn("archive damaged, but still found next record.");
#endif
			return p;
		}

		p++;
			// be careful not to increment before comparison;
			// magic may already be found at <start>.
	}

	// passed EOF, didn't find it.
	debug_warn("archive corrupted, next record not found.");
	return 0;
}


// find "End of Central Dir Record" in file.
// z_is_header has made sure size >= ECDR_SIZE.
// return -1 on failure (output param invalid), otherwise 0.
static LibError za_find_ecdr(const u8* file, size_t size, const ECDR*& ecdr_)
{
	// early out: check expected case (ECDR at EOF; no file comment)
	const ECDR* ecdr = (const ECDR*)(file + size - ECDR_SIZE);
	if(ecdr->magic == ecdr_magic)
		goto found_ecdr;

	// goto scoping
	{
		// scan the last 66000 bytes of file for ecdr_id signature
		// (the Zip archive comment field, up to 64k, may follow ECDR).
		// if the zip file is < 66000 bytes, scan the whole file.
		const u8* start = file + size - MIN(66000u, size);
		ecdr = (const ECDR*)za_find_id(file, size, start, ecdr_magic, ECDR_SIZE);
		if(!ecdr)
			return ERR_CORRUPTED;
	}

found_ecdr:
	ecdr_ = ecdr;
	return ERR_OK;
}


//
// date conversion from DOS to Unix
//
///////////////////////////////////////////////////////////////////////////////

static time_t time_t_from_FAT(u32 fat_timedate)
{
	const uint fat_time = bits(fat_timedate, 0, 15);
	const uint fat_date = bits(fat_timedate, 15, 31);

	struct tm t;							// struct tm format:
	t.tm_sec   = bits(fat_time, 0,4) * 2;	// [0,59]
	t.tm_min   = bits(fat_time, 5,10);		// [0,59]
	t.tm_hour  = bits(fat_time, 11,15);		// [0,23]
	t.tm_mday  = bits(fat_date, 0,4);		// [1,31]
	t.tm_mon   = bits(fat_date, 5,8)-1;		// [0,11]
	t.tm_year  = bits(fat_date, 9,15) + 80;	// since 1900
	t.tm_isdst = -1;	// unknown - let libc determine

	debug_assert(t.tm_year < 138);
		// otherwise: totally bogus, and at the limit of 32-bit time_t

	time_t ret = mktime(&t);
	if(ret == (time_t)-1)
		debug_warn("mktime failed");
	return ret;
}


static u32 FAT_from_time_t(time_t time)
{
	struct tm* t = gmtime(&time);

	uint fat_time = 0;
	fat_time |= (t->tm_sec/2);
	fat_time |= (t->tm_min) << 5;
	fat_time |= (t->tm_hour) << 11;

	uint fat_date = 0;
	fat_date |= (t->tm_mday);
	fat_date |= (t->tm_mon+1) << 5;
	fat_date |= (t->tm_year-80) << 9;

	u32 fat_timedate = (fat_date << 16) | fat_time;
	return fat_timedate;
}


///////////////////////////////////////////////////////////////////////////////


static bool za_cdfh_is_valid_file(u16 method, u32 csize, u32 ucsize)
{
	// compression method is unknown/unsupported
	if(method != Z_CM_STORED && method != Z_CM_DEFLATE)
		return false;

	// it's a directory entry (we only want files)
	if(!csize && !ucsize)
		return false;

	return true;
}


enum z_extract_cdfh_ret
{
	Z_CDFH_FILE_OK =  0,	// valid file; add to lookup.
	Z_CDFH_SKIPPED =  1		// not valid file, but have next CDFH; continue.
};

// read the current CDFH. if a valid file, return its filename and ZLoc.
// return -1 on error (output params invalid), or 0 on success.
// called by za_enum_files, which passes the output to lookup.
static z_extract_cdfh_ret za_extract_cdfh(const CDFH* cdfh,
	const char*& fn, size_t& fn_len, ZLoc* loc,	size_t& ofs_to_next_cdfh)
{
	// extract fields from CDFH
	const u16 method    = read_le16(&cdfh->method);
	const u32 fat_mtime = read_le32(&cdfh->mtime);
	const u32 csize     = read_le32(&cdfh->csize);
	const u32 ucsize    = read_le32(&cdfh->ucsize);
	const u16 fn_len_   = read_le16(&cdfh->fn_len);
	const u16 e_len     = read_le16(&cdfh->e_len);
	const u16 c_len     = read_le16(&cdfh->c_len);
	const u32 lfh_ofs   = read_le32(&cdfh->lfh_ofs);
	const char* fn_ = (const char*)cdfh+CDFH_SIZE;
		// not 0-terminated!

	// return offset to where next CDFH should be (caller will scan for it)
	ofs_to_next_cdfh = CDFH_SIZE + fn_len_ + e_len + c_len;

	if(!za_cdfh_is_valid_file(method, csize, ucsize))
		return Z_CDFH_SKIPPED;

	// write out entry data
	fn     = fn_;
	fn_len = fn_len_;
	loc->ofs    = lfh_ofs | LFH_FIXUP;
	loc->csize  = (method != Z_CM_STORED)? csize : 0;
	loc->ucsize = (off_t)ucsize;
	loc->mtime  = time_t_from_FAT(fat_mtime);

	return Z_CDFH_FILE_OK;
}


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
typedef LibError (*CDFH_CB)(uintptr_t user, i32 idx, const char* fn, size_t fn_len, const ZLoc* loc);


static LibError za_enum_files(const u8* file, const size_t size, const CDFH_CB cb, const uintptr_t user)
{
	// find "End of Central Directory Record"
	const ECDR* ecdr;
	CHECK_ERR(za_find_ecdr(file, size, ecdr));

	// call back with number of entries in archives (an upper bound
	// for valid files; we're not interested in the directory entries).
	// we'd have to scan through the central dir to count them out; we'll
	// just skip them and waste a bit of preallocated memory.
	const i32 num_entries = read_le16(&ecdr->cd_entries);
	// .. callback expects -num_entries < 0.
	//    if it's 0, the callback would treat it as an index => crash.
	//    ERR_FAIL means we'll no longer be called.
	if(!num_entries)
		return ERR_FAIL;
	CHECK_ERR(cb(user, -num_entries, 0, 0, 0));

	// iterate through CDFH
	const u32 cd_ofs = read_le32(&ecdr->cd_ofs);
	const CDFH* cdfh = (const CDFH*)(file + cd_ofs);
	i32 idx = 0;
		// only incremented when valid, so we don't leave holes
		// in lookup's arrays (bad locality).


	for(i32 i = 0; i < num_entries; i++)
	{
		// scan for next CDFH (at or beyond current cdfh position)
		cdfh = (CDFH*)za_find_id(file, size, (const u8*)cdfh, cdfh_magic, CDFH_SIZE);
		if(!cdfh)					// no (further) CDFH found:
			return ERR_CORRUPTED;	// abort.

		const char* fn;
		size_t fn_len;
		ZLoc loc;
		size_t ofs_to_next_cdfh;

		z_extract_cdfh_ret ret = za_extract_cdfh(cdfh, fn, fn_len, &loc, ofs_to_next_cdfh);
		// valid file
		if(ret == Z_CDFH_FILE_OK)
		{
			LibError cb_ret = cb(user, i, fn, fn_len, &loc);
			if(cb_ret != INFO_CB_CONTINUE)
				return cb_ret;
			idx++;	// see rationale above
		}
		// else: skipping this CDFH (e.g. if directory)

		cdfh = (const CDFH*)((u8*)cdfh + ofs_to_next_cdfh);
	}

	return ERR_OK;
}


static void fixup()
{
/*
	// find corresponding LFH, needed to calculate file offset
	// (its extra field may not match that reported by CDFH!).
	// TODO: this is slow, due to seeking backwards.
	// optimization: calculate only on demand (i.e. open, not mount)?
	const u8* lfh = za_find_id(file, size, (u8*)file+lfh_ofs, lfh_magic, LFH_SIZE);

	// get actual file ofs (see above)
	const u16 lfh_fn_len = read_le16(lfh+26);
	const u16 lfh_e_len  = read_le16(lfh+28);
	const off_t file_ofs = lfh_ofs + LFH_SIZE + lfh_fn_len + lfh_e_len;
	// LFH doesn't have a comment field!
*/
}


struct ZipArchive
{
	File f;
	off_t cur_file_size;

	Pool cdfhs;
	uint cd_entries;
};

struct ZipEntry
{
	char path[PATH_MAX];
	size_t ucsize;
	time_t mtime;
	ZipCompressionMethod method;
	size_t csize;
	void* cdata;
};

LibError zip_archive_create(const char* zip_filename, ZipArchive* za)
{
	memset(za, 0, sizeof(*za));
	RETURN_ERR(file_open(zip_filename, 0, &za->f));
	RETURN_ERR(pool_create(&za->cdfhs, 10*MiB, 0));
	return ERR_OK;
}


static inline u32 u32_from_size_t(size_t x)
{
	debug_assert(x <= 0xFFFFFFFF);
	return (u32)(x & 0xFFFFFFFF);
}

static inline u16 u16_from_size_t(size_t x)
{
	debug_assert(x <= 0xFFFF);
	return (u16)(x & 0xFFFF);
}


LibError zip_archive_add(ZipArchive* za, const ZipEntry* ze)
{
	const char* fn      = ze->path;
	const size_t fn_len = strlen(fn);
	const size_t ucsize = ze->ucsize;
	const u32 fat_mtime = FAT_from_time_t(ze->mtime);
	const u16 method    = (u16)ze->method;
	const size_t csize  = ze->csize;
	void* cdata         = ze->cdata;

	const off_t lfh_ofs = za->cur_file_size;

	// write (LFH, filename, file contents) to archive
	const size_t  lfh_size = sizeof( LFH);
	const LFH lfh =
	{
		lfh_magic,
		0,	// x1
		0,	// flags
		method,
		fat_mtime,
		0,	// crc
		u32_from_size_t(csize),
		u32_from_size_t(ucsize),
		u16_from_size_t(fn_len),
		0	// e_len
	};
	file_io(&za->f, lfh_ofs,                          lfh_size, (void*)&lfh);
	file_io(&za->f, lfh_ofs+lfh_size,                 fn_len,   (void*)fn);
	file_io(&za->f, lfh_ofs+(off_t)(lfh_size+fn_len), csize,    (void*)cdata);
	za->cur_file_size += (off_t)(lfh_size+fn_len+csize);

	// append a CDFH to the central dir (in memory)
	const size_t cdfh_size = sizeof(CDFH);
	CDFH* cdfh = (CDFH*)pool_alloc(&za->cdfhs, cdfh_size+fn_len);
	if(cdfh)
	{
		cdfh->magic   = cdfh_magic;
		cdfh->x1      = 0;
		cdfh->flags   = 0;
		cdfh->method  = method;
		cdfh->mtime   = fat_mtime;
		cdfh->crc     = 0;
		cdfh->csize   = u32_from_size_t(csize);
		cdfh->ucsize  = u32_from_size_t(ucsize);
		cdfh->fn_len  = u16_from_size_t(fn_len);
		cdfh->e_len   = 0;
		cdfh->c_len   = 0;
		cdfh->x2      = 0;
		cdfh->x3      = 0;
		cdfh->lfh_ofs = lfh_ofs;
		memcpy2((char*)cdfh+cdfh_size, fn, fn_len);

		za->cd_entries++;
	}

	return ERR_OK;
}


LibError zip_archive_finish(ZipArchive* za)
{
	const size_t cd_size = za->cdfhs.da.pos;

	// append an ECDR to the CDFH list (this allows us to
	// write out both to the archive file in one burst)
	ECDR* ecdr = (ECDR*)pool_alloc(&za->cdfhs, sizeof(ECDR));
	if(!ecdr)
		return ERR_NO_MEM;
	ecdr->magic       = ecdr_magic;
	memset(ecdr->x1, 0, sizeof(ecdr->x1));
	ecdr->cd_entries  = za->cd_entries;
	ecdr->cd_size     = (u32)cd_size;
	ecdr->cd_ofs      = za->cur_file_size;
	ecdr->comment_len = 0;

	file_io(&za->f, za->cur_file_size, za->cdfhs.da.pos, za->cdfhs.da.base);

	(void)file_close(&za->f);
	(void)pool_destroy(&za->cdfhs);
	return ERR_OK;
}





///////////////////////////////////////////////////////////////////////////////
//
// lookup_*: file lookup
// per archive: return file info (e.g. offset, size), given filename.
//
///////////////////////////////////////////////////////////////////////////////


	// rationale:
	// - we don't export a "key" (currently array index) that would allow faster
	//   file lookup. this would only be useful if higher-level code were to
	//   store the key and use it more than once. also, lookup is currently fast
	//   enough. finally, this would also make our file enumerate callback
	//   incompatible with the others (due to the extra key param).
	//
	// - we don't bother with a directory tree to speed up lookup. the above
	//   is fast enough: O(1) if accessed sequentially, otherwise O(log(files)).


///////////////////////////////////////////////////////////////////////////////
//
// ZArchive_*: Handle-based container for archive info
// owns archive file and its lookup mechanism.
//
///////////////////////////////////////////////////////////////////////////////


struct ZArchive
{
	File f;

	ZLoc* ents;
	// number of valid entries in above array (see lookup_add_file_cb)
	i32 num_files;

	Bucket fn_storage;

	// note: we need to keep track of what resources reload() allocated,
	// so the dtor can free everything correctly.
	uint is_open : 1;
	uint is_mapped : 1;
	uint is_loaded : 1;
};

H_TYPE_DEFINE(ZArchive);




// look up ZLoc, given filename (untrusted!).
static LibError archive_get_file_info(ZArchive* za, const char* fn, uintptr_t memento, ZLoc*& loc)
{
	if(memento)
	{
		loc = (ZLoc*)memento;
		return ERR_OK;
	}
	else
	{
		for(i32 i = 0; i < za->num_files; i++)
			if(!strcmp(za->ents[i].fn, fn))
			{
				loc = &za->ents[i];
				return ERR_OK;
			}
	}

	return ERR_FILE_NOT_FOUND;
}



// add file <fn> to the lookup data structure.
// called from za_enum_files in order (0 <= idx < num_entries).
// the first call notifies us of # entries, so we can allocate memory.
//
// notes:
// - fn (filename) is not necessarily 0-terminated!
// - loc is only valid during the callback! must be copied or saved.
static LibError archive_add_file_cb(uintptr_t user, i32 i,
									const char* fn, size_t fn_len, const ZLoc* loc)
{
	ZArchive* za = (ZArchive*)user;

	// HACK: on first call, i is negative and tells us how many
	// entries are in the archive (so we can allocate memory).
	// see za_enum_files for why it's done this way.
	if(i < 0)
	{
		const i32 num_entries = -i;

		za->ents = (ZLoc*)mem_alloc(num_entries * sizeof(ZLoc), 32);
		if(!za->ents)
			return ERR_NO_MEM;
		return INFO_CB_CONTINUE;
	}

	// adding a regular file.

	ZLoc* ent = &za->ents[i];
	*ent = *loc;
	// .. copy filename (needs to be 0-terminated)
	//    note: Zip paths only have '/' terminators; no need to convert.
	char* fn_copy = (char*)bucket_alloc(&za->fn_storage, fn_len+1);
	if(!fn_copy)
		return ERR_NO_MEM;
	memcpy2(fn_copy, fn, fn_len);
	fn_copy[fn_len] = '\0';
	ent->fn = fn_copy;

	za->num_files++;
	return INFO_CB_CONTINUE;
}





static void ZArchive_init(ZArchive*, va_list)
{
}

static void ZArchive_dtor(ZArchive* za)
{
	if(za->is_loaded)
	{
		(void)mem_free(za->ents);
		bucket_free_all(&za->fn_storage);

		za->is_loaded = 0;
	}
	if(za->is_mapped)
	{
		(void)file_unmap(&za->f);
		za->is_mapped = 0;
	}
	if(za->is_open)
	{
		(void)file_close(&za->f);
		za->is_open = 0;
	}
}

static LibError ZArchive_reload(ZArchive* za, const char* fn, Handle)
{
	// (note: don't warn on failure - this happens when
	// vfs_mount blindly zip_archive_opens a dir)
	RETURN_ERR(file_open(fn, FILE_CACHE_BLOCK, &za->f));
	za->is_open = 1;

	void* file_; size_t size;
	RETURN_ERR(file_map(&za->f, file_, size));
	const u8* file = (const u8*)file_;
	za->is_mapped = 1;

	// check if it's even a Zip file.
	// the VFS blindly opens files when mounting; it needs to open
	// all archives, but doesn't know their extension (e.g. ".pk3").
	if(!za_is_header(file, size))
		return ERR_UNKNOWN_FORMAT;

	za->is_loaded = 1;
	RETURN_ERR(za_enum_files(file, size, archive_add_file_cb, (uintptr_t)za));

	// we map the file only for convenience when loading;
	// extraction is via aio (faster, better mem use).
	(void)file_unmap(&za->f);
	za->is_mapped = 0;

	return ERR_OK;
}

static LibError ZArchive_validate(const ZArchive* za)
{
	RETURN_ERR(file_validate(&za->f));

	if(debug_is_pointer_bogus(za->ents))
		return ERR_1;
	if(za->num_files < 0)
		return ERR_2;

	return ERR_OK;
}

static LibError ZArchive_to_string(const ZArchive* za, char* buf)
{
	snprintf(buf, H_STRING_LEN, "(%d files)", za->num_files);
	return ERR_OK;
}



// open and return a handle to the zip archive indicated by <fn>.
// somewhat slow - each file is added to an internal index.
Handle zip_archive_open(const char* fn)
{
TIMER("zip_archive_open");
	return h_alloc(H_ZArchive, fn);
}


// close the archive <ha> and set ha to 0
LibError zip_archive_close(Handle& ha)
{
	return h_free(ha, H_ZArchive);
}


// successively call <cb> for each valid file in the archive <ha>,
// passing the complete path and <user>.
// if it returns a nonzero value, abort and return that, otherwise 0.
LibError zip_enum(const Handle ha, const FileCB cb, const uintptr_t user)
{
	H_DEREF(ha, ZArchive, za);

	struct stat s;
	memset(&s, 0, sizeof(s));

	for(i32 i = 0; i < za->num_files; i++)
	{
		const ZLoc* ent = &za->ents[i];

		s.st_mode  = S_IFREG;
		s.st_size  = (off_t)ent->ucsize;
		s.st_mtime = ent->mtime;

		LibError ret = cb(ent->fn, &s, user);
		if(ret != INFO_CB_CONTINUE)
			return ret;
	}

	return ERR_OK;
}





///////////////////////////////////////////////////////////////////////////////
//
// inf_*: in-memory inflate routines (zlib wrapper)
// decompresses blocks from file_io callback.
//
///////////////////////////////////////////////////////////////////////////////

static LibError LibError_from_zlib(int err)
{
	switch(err)
	{
	case Z_OK:
		return ERR_OK;
	case Z_STREAM_END:
		return ERR_EOF;
	case Z_MEM_ERROR:
		return ERR_NO_MEM;
	case Z_DATA_ERROR:
		return ERR_CORRUPTED;
	case Z_STREAM_ERROR:
		return ERR_INVALID_PARAM;
	default:
		return ERR_FAIL;
	}
	UNREACHABLE;
}

enum ZLibContextType
{
	COMPRESSION,
	DECOMPRESSION
};

enum DecompressMode
{
	DM_ZLIB,
	DM_MEMCPY
};

// must be dynamically allocated - need one for every open ZFile,
// and z_stream is large.
struct ZLibContext
{
	z_stream zs;

	ZLibContextType type;

	DecompressMode mode;

	// 0 until zlib_feed_decompressor called with free_in_buf = true.
	// mem_free-d after consumed by zlib_feed_decompressor, or by inf_free.
	// note: necessary; can't just use next_in-total_in, because
	// we may inflate in chunks.
	//
	// can't have this owned (i.e. allocated) by inf_, because
	// there can be several IOs in-flight and therefore buffers of
	// compressed data. we'd need a list if stored here; having the
	// IOs store them and pass them to us is more convenient.
	void* in_buf;
};


static ZLibContext single_ctx;
static uintptr_t single_ctx_in_use;


// convenience - both zlib_feed_decompressor and inf_free use this.
static void free_in_buf(ZLibContext* ctx)
{
	mem_free(ctx->in_buf);
	ctx->in_buf = 0;
}


static uintptr_t zlib_create_ctx(ZLibContextType type)
{
#ifdef NO_ZLIB
	return 0;
#else
	ZLibContext* ctx = (ZLibContext*)single_calloc(&single_ctx, &single_ctx_in_use, sizeof(single_ctx));
	if(!ctx)
		return 0;

	ctx->type = type;

	z_stream* zs = &ctx->zs;
	zs->next_in = 0;
	zs->zalloc = 0;
	zs->zfree  = 0;
	zs->opaque = 0;

	const int windowBits = -MAX_WBITS;	// max window size; omit ZLib header
	int err;

	if(type == COMPRESSION)
	{
		const int level    = Z_BEST_COMPRESSION;
		const int memLevel = 8;			// default; total mem ~= 256KiB
		const int strategy = Z_DEFAULT_STRATEGY;	// normal data - not RLE
		err = deflateInit2(&ctx->zs, level, Z_DEFLATED, windowBits, memLevel, strategy);
	}
	else
	{
		err = inflateInit2(zs, windowBits);
	}

	if(err != Z_OK)
	{
		debug_warn("failed");
		single_free(&single_ctx, &single_ctx_in_use, ctx);
		return 0;
	}

	return (uintptr_t)ctx;
#endif
}


static void zlib_destroy_ctx(uintptr_t zlib_ctx)
{
#ifdef NO_ZLIB
	return ERR_NOT_IMPLEMENTED;
#else
	ZLibContext* ctx = (ZLibContext*)zlib_ctx;
	z_stream* zs = &ctx->zs;
	int err;

	if(ctx->type == COMPRESSION)
	{
		err = deflateEnd(zs);
	}
	else
	{
		free_in_buf(ctx);

		// can have both input or output data remaining
		// (if not all data in uncompressed stream was needed)

		err = inflateEnd(zs);
	}

	if(err != Z_OK)
		debug_warn("in/deflateEnd reports error");

	single_free(&single_ctx, &single_ctx_in_use, ctx);
#endif
}


//-----------------------------------------------------------------------------

static LibError zlib_prepare_compress(uintptr_t zlib_ctx, size_t total_ucsize)
{
#ifdef NO_ZLIB
	return ERR_NOT_IMPLEMENTED;
#else
	ZLibContext* ctx = (ZLibContext*)zlib_ctx;
	z_stream* zs = &ctx->zs;
	int err;

	err = deflateReset(zs);
	debug_assert(err == Z_OK);

	size_t max_csize = (size_t)deflateBound(zs, (uLong)total_ucsize);
	void* cdata = mem_alloc(max_csize, 32*KiB);
	if(!cdata)
		return ERR_NO_MEM;

	zs->next_out = (Byte*)cdata;
	zs->avail_out = (uInt)max_csize;

	return ERR_OK;
#endif
}


static LibError zlib_feed_compressor(uintptr_t zlib_ctx, void* in, size_t in_size)
{
#ifdef NO_ZLIB
	return ERR_NOT_IMPLEMENTED;
#else
	ZLibContext* ctx = (ZLibContext*)zlib_ctx;
	z_stream* zs = &ctx->zs;

	// since output buffer is guaranteed to be big enough,
	// no input data should 'survive' the deflate call.
	if(zs->avail_in)
		debug_warn("previous input buffer remains");

	zs->avail_in = (uInt)in_size;
	zs->next_in = (Byte*)in;

	const size_t prev_avail_out = zs->avail_out;
	int err = deflate(zs, 0);
	const size_t avail_out = zs->avail_out;

	// check how many bytes were output.
	//
	// note: zlib may not always output data, e.g. if passed very little
	// data in one block due to misalignment. in that case, return 0
	// ("no data output"), which doesn't cause caller to abort.
	debug_assert(avail_out <= prev_avail_out);
	const ssize_t nread = (ssize_t)(prev_avail_out - avail_out);
	if(!nread && err != Z_OK)
		return ERR_FAIL;
// TODO: return zlib error
	return ERR_OK;
#endif
}


static LibError zlib_finish_compress(uintptr_t zlib_ctx, void** cdata, size_t* csize)
{
#ifdef NO_ZLIB
	return ERR_NOT_IMPLEMENTED;
#else
	ZLibContext* ctx = (ZLibContext*)zlib_ctx;
	z_stream* zs = &ctx->zs;
	int err;

	// notify zlib that no more data is forthcoming and have it flush output.
	// our output buffer has enough space due to use of deflateBound;
	// therefore, deflate must return Z_STREAM_END.
	err = deflate(zs, Z_FINISH);
	if(err != Z_STREAM_END)
		debug_warn("deflate: unexpected Z_FINISH behavior");

	*cdata = zs->next_out - zs->total_out;
	*csize = zs->total_out;
	return ERR_OK;
#endif
}


//-----------------------------------------------------------------------------


// subsequent calls to zlib_feed_decompressor will unzip into <out>.
static LibError zlib_prepare_decompress(uintptr_t zlib_ctx, DecompressMode mode, void* out, size_t out_size)
{
#ifdef NO_ZLIB
	return ERR_NOT_IMPLEMENTED;
#else
	ZLibContext* ctx = (ZLibContext*)zlib_ctx;
	z_stream* zs = &ctx->zs;

	ctx->mode = mode;

	if(zs->next_out || zs->avail_out)
	{
		debug_warn("ctx already in use!");
		return ERR_LOGIC;
	}
	zs->next_out  = (Byte*)out;
	zs->avail_out = (uInt)out_size;
	return ERR_OK;
#endif
}


TIMER_ADD_CLIENT(tc_zip_inflate);
TIMER_ADD_CLIENT(tc_zip_memcpy);

// unzip into output buffer. returns bytes written
// (may be 0, if not enough data is passed in), or < 0 on error.
static ssize_t zlib_feed_decompressor(uintptr_t _ctx, void* in, size_t in_size, bool free_in_buf = false)
{
#ifdef NO_ZLIB
	return ERR_NOT_IMPLEMENTED;
#else

	ZLibContext* ctx = (ZLibContext*)_ctx;
	z_stream* zs = &ctx->zs;


	size_t prev_avail_out = zs->avail_out;

	if(in)
	{
		if(ctx->in_buf)
			debug_warn("previous input buffer not empty");
		zs->avail_in = (uInt)in_size;
		zs->next_in = (Byte*)in;

		if(free_in_buf)
			ctx->in_buf = in;
	}

	LibError err = ERR_OK;

	if(ctx->mode == DM_ZLIB)
	{
		TIMER_ACCRUE(tc_zip_inflate);
		int ret = inflate(zs, Z_SYNC_FLUSH);
		err = LibError_from_zlib(ret);
		// sanity check: if ZLib reports end of stream, all input data
		// must have been consumed.
		if(err == ERR_EOF)
		{
			debug_assert(zs->avail_in == 0);
			err = ERR_OK;
		}
	}
	else
	{
		TIMER_ACCRUE(tc_zip_memcpy);
		memcpy2(zs->next_out, zs->next_in, zs->avail_in);
		uInt size = MIN(zs->avail_in, zs->avail_out);
		zs->avail_out -= size;
		zs->avail_in -= size;	// => = 0
		zs->next_in += size;
		zs->next_out += size;
		zs->total_in += size;
		zs->total_out += size;
	}

	// check+return how much actual data was read
	//
	// note: zlib may not always output data, e.g. if passed very little
	// data in one block (due to misalignment). return 0 ("no data output"),
	// which doesn't abort the read.
	size_t avail_out = zs->avail_out;

	debug_assert(avail_out <= prev_avail_out);
	// make sure output buffer size didn't magically increase
	ssize_t nread = (ssize_t)(prev_avail_out - avail_out);
	if(!nread)
		return (err < 0)? err : 0;
	// try to pass along the ZLib error code, but make sure
	// it isn't treated as 'bytes output', i.e. > 0.

	return nread;
#endif
}




//-----------------------------------------------------------------------------





//-----------------------------------------------------------------------------
// archive builder
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static LibError trace_get_next_file(void* trace, uint i, const char* path)
{
	return ERR_DIR_END;
}


static ssize_t compress_cb(uintptr_t ctx, void* buf, size_t buf_size)
{
	uintptr_t zlib_ctx = ctx;

	(void)zlib_feed_compressor(zlib_ctx, buf, buf_size);
// TODO: echo into second buffer, in case compression isnt working out
	return (ssize_t)buf_size;
}

static LibError read_and_compress_file(uintptr_t zlib_ctx, ZipEntry* ze)
{
	const char* fn = ze->path;

// TODO: decide if compressible

	struct stat s;
	RETURN_ERR(file_stat(fn, &s));
	const size_t ucsize = s.st_size;

	RETURN_ERR(zlib_prepare_compress(zlib_ctx, ucsize));

	File f;
	RETURN_ERR(file_open(fn, 0, &f));
	ssize_t ucbytes_read = file_io(&f, 0, ucsize, 0, compress_cb, zlib_ctx);
	UNUSED2(ucbytes_read);
	(void)file_close(&f);

	void* cdata; size_t csize;
	(void)zlib_finish_compress(zlib_ctx, &cdata, &csize);

	ze->ucsize = ucsize;
	ze->mtime  = s.st_mtime;
	ze->method = Z_CM_DEFLATE;
	ze->csize  = csize;
	ze->cdata  = cdata;

	return ERR_OK;
}

static void build_optimized_archive(const char* zip_filename, void* trace)
{
	ZipArchive za;
	zip_archive_create(zip_filename, &za);

	uintptr_t zlib_ctx = zlib_create_ctx(COMPRESSION);
	uint trace_i = 0;
	uint queued_files = 0, committed_files = 0;

	for(;;)
	{
		ZipEntry ze; // TODO: QUEUE
		const int max_readqueue_depth = 1;
		for(uint i = 0; i < max_readqueue_depth; i++)
		{
			LibError ret = trace_get_next_file(trace, trace_i, ze.path);
			if(ret == ERR_DIR_END)
				break;

			WARN_ERR(read_and_compress_file(zlib_ctx, &ze));
			queued_files++;
		}

		if(committed_files == queued_files)
			break;
		zip_archive_add(&za, &ze);
		committed_files++;
	}


	zlib_destroy_ctx(zlib_ctx);
	

	zip_archive_finish(&za);
}





///////////////////////////////////////////////////////////////////////////////
//
// zip_*: file from Zip archive
// uses lookup to get file information; holds inflate state.
//
///////////////////////////////////////////////////////////////////////////////

// convenience function, allows implementation change in ZFile.
// note that size == ucsize isn't foolproof, and adding a flag to
// ofs or size is ugly and error-prone.
// no error checking - always called from functions that check zf.
static inline bool zfile_compressed(ZFile* zf)
{
	return zf->csize != 0;
}




// get file status (size, mtime). output param is zeroed on error.
LibError zip_stat(Handle ha, const char* fn, struct stat* s)
{
	// zero output param in case we fail below.
	memset(s, 0, sizeof(struct stat));

	H_DEREF(ha, ZArchive, za);

	ZLoc* loc;
	CHECK_ERR(archive_get_file_info(za, fn, 0, loc));

	s->st_size  = loc->ucsize;
	s->st_mtime = loc->mtime;
	return ERR_OK;
}




LibError zip_validate(const ZFile* zf)
{
	if(!zf)
		return ERR_INVALID_PARAM;
	// note: don't check zf->ha - it may be freed at shutdown before
	// its files. TODO: revisit once dependency support is added.
	if(!zf->ucsize)
		return ERR_1;
	else if(!zf->inf_ctx)
		return ERR_2;

	return ERR_OK;
}

#define CHECK_ZFILE(zf) CHECK_ERR(zip_validate(zf))


// open file, and fill *zf with information about it.
// return < 0 on error (output param zeroed). 
LibError zip_open(const Handle ha, const char* fn, int flags, ZFile* zf)
{
	// zero output param in case we fail below.
	memset(zf, 0, sizeof(*zf));

	H_DEREF(ha, ZArchive, za);

	ZLoc* loc;
		// don't want ZFile to contain a ZLoc struct -
		// its ucsize member must be 'loose' for compatibility with File.
		// => need to copy ZLoc fields into ZFile.
	RETURN_ERR(archive_get_file_info(za, fn, 0, loc));

	zf->flags     = flags;
	zf->ucsize    = loc->ucsize;
	zf->ofs       = loc->ofs;
	zf->csize     = loc->csize;
	zf->ha        = ha;
	zf->inf_ctx   = 0;
	zf->is_mapped = 0;
	CHECK_ZFILE(zf);
	return ERR_OK;
}


// close file.
LibError zip_close(ZFile* zf)
{
	CHECK_ZFILE(zf);
	// other ZFile fields don't need to be freed/cleared
	zlib_destroy_ctx(zf->inf_ctx);
	return ERR_OK;
}



///////////////////////////////////////////////////////////////////////////////
//
// sync and async I/O
// uses file_* and inf_*.
//
///////////////////////////////////////////////////////////////////////////////


// rationale for not supporting aio for compressed files:
// would complicate things considerably (could no longer just
// return the file I/O context, since we have to decompress in wait_io),
// yet it isn't really useful - the main application is streaming music,
// which is already compressed.


static const size_t CHUNK_SIZE = 16*KiB;

// begin transferring <size> bytes, starting at <ofs>. get result
// with zip_io_wait; when no longer needed, free via zip_io_discard.
LibError zip_io_issue(ZFile* zf, off_t user_ofs, size_t max_output_size, void* user_buf, ZipIo* io)
{
	// not needed, since ZFile tells us the last read offset in the file.
	UNUSED2(user_ofs);

	// zero output param in case we fail below.
	memset(io, 0, sizeof(ZipIo));

	CHECK_ZFILE(zf);
	H_DEREF(zf->ha, ZArchive, za);

	// transfer params that differ if compressed
	size_t size = max_output_size;
	void* buf = user_buf;

	const off_t ofs = zf->ofs + zf->last_read_ofs;
	// needed before align check below

	if(zfile_compressed(zf))
	{
		io->inf_ctx = zf->inf_ctx;
		io->max_output_size = max_output_size;
		io->user_buf = user_buf;

		// if there's anything left in the inf_ctx buffer, return that.
		// required! if data remaining in buffer expands to fill max output,
		// we must not read more cdata - nowhere to store it.
		CHECK_ERR(zlib_prepare_decompress(io->inf_ctx, DM_ZLIB, io->user_buf, io->max_output_size));
		ssize_t bytes_inflated = zlib_feed_decompressor(io->inf_ctx, 0, 0);
		CHECK_ERR(bytes_inflated);
		if(bytes_inflated == (ssize_t)max_output_size)
		{
			io->already_inflated = true;
			io->max_output_size = bytes_inflated;
			return ERR_OK;
		}

		// read up to next chunk (so that the next read is aligned -
		// less work for aio) or up to EOF.
		const ssize_t left_in_chunk = CHUNK_SIZE - (ofs % CHUNK_SIZE);
		const ssize_t left_in_file = zf->csize - ofs;
		size = MIN(left_in_chunk, left_in_file);

		// note: only need to clamp if compressed

		buf = mem_alloc(size, 4*KiB);
	}
	// else: not compressed; we'll just read directly from the archive file.
	// no need to clamp to EOF - that's done already by the VFS.
	{
		io->inf_ctx = 0;
	}

	zf->last_read_ofs += (off_t)size;

	CHECK_ERR(file_io_issue(&za->f, ofs, size, buf, &io->io));

	return ERR_OK;
}


// indicates if the IO referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
int zip_io_has_completed(ZipIo* io)
{
	if(io->already_inflated)
		return 1;
	return file_io_has_completed(&io->io);
}


// wait until the transfer <io> completes, and return its buffer.
// output parameters are zeroed on error.
LibError zip_io_wait(ZipIo* io, void*& buf, size_t& size)
{
	buf  = io->user_buf;
	size = io->max_output_size;
	if(io->already_inflated)
		return ERR_OK;

	void* raw_buf;
	size_t raw_size;
	CHECK_ERR(file_io_wait(&io->io, raw_buf, raw_size));

	if(io->inf_ctx)
	{
		zlib_prepare_decompress(io->inf_ctx, DM_ZLIB, buf, size);
		// we allocated the compressed data input buffer and
		// want it freed when it's consumed.
		const bool want_input_buf_freed = true;
		ssize_t bytes_inflated = zlib_feed_decompressor(io->inf_ctx, raw_buf, raw_size, want_input_buf_freed);
		CHECK_ERR(bytes_inflated);
	}
	else
	{
		buf  = raw_buf;
		size = raw_size;
	}

	// TODO update what we return - check LFH and skip tat -------------------------------------------------------------

	return ERR_OK;
}


// finished with transfer <io> - free its buffer (returned by zip_io_wait)
LibError zip_io_discard(ZipIo* io)
{
	if(io->already_inflated)
		return ERR_OK;
	return file_io_discard(&io->io);
}


LibError zip_io_validate(const ZipIo* io)
{
	if(debug_is_pointer_bogus(io->user_buf))
		return ERR_1;
	if(*(u8*)&io->already_inflated > 1)
		return ERR_2;
	// <inf_ctx> and <max_output_size> have no invariants we could check.
	RETURN_ERR(file_io_validate(&io->io));
	return ERR_OK;
}


///////////////////////////////////////////////////////////////////////////////




// allow user-specified callbacks: "chain" them, because file_io's
// callback mechanism is already used to return blocks.

struct CBParams
{
	uintptr_t inf_ctx;

	FileIOCB user_cb;
	uintptr_t user_ctx;
};


static ssize_t read_cb(uintptr_t ctx, void* buf, size_t size)
{
	CBParams* p = (CBParams*)ctx;

	ssize_t ucsize = zlib_feed_decompressor(p->inf_ctx, buf, size);

	if(p->user_cb)
	{
		ssize_t user_ret = p->user_cb(p->user_ctx, buf, size);
		// only pass on error codes - we need to return number of actual
		// bytes inflated to file_io in the normal case.
		if(user_ret < 0)
			return user_ret;
	}

	return ucsize;
}


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
ssize_t zip_read(ZFile* zf, off_t ofs, size_t size, void* p, FileIOCB cb, uintptr_t ctx)
{
	CHECK_ZFILE(zf);
	H_DEREF(zf->ha, ZArchive, za);

	ofs += zf->ofs;

	// pump all previous cdata out of inflate context
	// if that satisfied the request, we're done


	// not compressed - just pass it on to file_io
	// (avoid the Zip inflate start/finish stuff below)
	//const bool compressed = zfile_compressed(zf);
	//	if(!compressed)
	//		return file_io(&za->f, ofs, csize, p);
	// no need to set last_raw_ofs - only checked if compressed.

	// compressed

	CHECK_ERR(zlib_prepare_decompress(zf->inf_ctx, DM_ZLIB, p, size));

	/*
	static bool once = false;
	if(!once)
	{

	once=true;
	uintptr_t xctx = inf_init_ctx();
	size_t xsize = za->f.size;
	void* xbuf=mem_alloc(xsize, 65536);
	zlib_prepare_decompress(xctx, xbuf, xsize);
	const IOCBParams xparams = { xctx, false, 0, 0 };
	double t1 = get_time();
	file_io(&za->f,0, xsize, 0, io_cb, (uintptr_t)&xparams);
	double t2 = get_time();
	debug_printf("\n\ntime to load whole archive %f\nthroughput %f MiB/s\n", t2-t1, xsize / (t2-t1) / 1e6);
	mem_free(xbuf);
	}
	*/

	const CBParams params = { zf->inf_ctx, cb, ctx };

	// HACK: shouldn't read the whole thing into mem
	size_t csize = zf->csize;
	if(!csize)
		csize = zf->ucsize;	// HACK on HACK: csize = 0 if file not compressed


	ssize_t uc_transferred = file_io(&za->f, ofs, csize, (void**)0, read_cb, (uintptr_t)&params);

	zf->last_read_ofs += (off_t)csize;

	return uc_transferred;
}


///////////////////////////////////////////////////////////////////////////////
//
// file mapping
//
///////////////////////////////////////////////////////////////////////////////


// map the entire file <zf> into memory. mapping compressed files
// isn't allowed, since the compression algorithm is unspecified.
// output parameters are zeroed on failure.
//
// the mapping will be removed (if still open) when its file is closed.
// however, map/unmap calls should still be paired so that the mapping
// may be removed when no longer needed.
LibError zip_map(ZFile* zf, void*& p, size_t& size)
{
	p = 0;
	size = 0;

	CHECK_ZFILE(zf);

	// mapping compressed files doesn't make sense because the
	// compression algorithm is unspecified - disallow it.
	if(zfile_compressed(zf))
		CHECK_ERR(ERR_IS_COMPRESSED);

	// note: we mapped the archive in zip_archive_open, but unmapped it
	// in the meantime to save memory in case it wasn't going to be mapped.
	// now we do so again; it's unmapped in zip_unmap (refcounted).
	H_DEREF(zf->ha, ZArchive, za);
	void* archive_p;
	size_t archive_size;
	CHECK_ERR(file_map(&za->f, archive_p, archive_size));

	p = (char*)archive_p + zf->ofs;
	size = zf->ucsize;

	zf->is_mapped = 1;
	return ERR_OK;
}


// remove the mapping of file <zf>; fail if not mapped.
//
// the mapping will be removed (if still open) when its archive is closed.
// however, map/unmap calls should be paired so that the archive mapping
// may be removed when no longer needed.
LibError zip_unmap(ZFile* zf)
{
	CHECK_ZFILE(zf);

	// make sure archive mapping refcount remains balanced:
	// don't allow multiple|"false" unmaps.
	if(!zf->is_mapped)
		return ERR_FAIL;
	zf->is_mapped = 0;

	H_DEREF(zf->ha, ZArchive, za);
	return file_unmap(&za->f);
}
