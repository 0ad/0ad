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
// - z_*: Zip-specific code
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

#include "lib.h"
#include "zip.h"
#include "res.h"

#include <map>

#include <assert.h>

// provision for removing all ZLib code (all inflate calls will fail).
// used for checking DLL dependency; might also simulate corrupt Zip files.
//#define NO_ZLIB

#ifndef NO_ZLIB
# define ZLIB_DLL
# include <zlib.h>

# ifdef _MSC_VER
#  ifdef NDEBUG
#   pragma comment(lib, "zlib1.lib")
#  else
#   pragma comment(lib, "zlib1d.lib")
#  endif
# endif
#endif


///////////////////////////////////////////////////////////////////////////////
//
// z_*: Zip-specific code
// passes the list of files in an archive to lookup.
//
///////////////////////////////////////////////////////////////////////////////


// convenience container for location / size of file in archive.
struct ZLoc
{
	off_t ofs;
	off_t csize;	// = 0 if not compressed
	off_t ucsize;

	time_t mtime;

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
static const char cdfh_id[] = "PK\1\2";
static const char lfh_id[]  = "PK\3\4";
static const char ecdr_id[] = "PK\5\6";
// lengths include the id field!
const size_t CDFH_SIZE = 46;
const size_t LFH_SIZE  = 30;
const size_t ECDR_SIZE = 22;


// return -1 if file is obviously not a valid Zip archive,
// otherwise 0. used as early-out test in lookup_init (see call site).
static inline int z_validate(const u8* file, size_t size)
{
	// make sure it's big enough to check the header and for
	// z_find_ecdr to succeed (if smaller, it's definitely bogus).
	if(size < ECDR_SIZE)
		return ERR_CORRUPTED;

	// check "header" (first LFH) signature
	return (*(u32*)file == *(u32*)&lfh_id)? 0 : -1;
}


// scan for and return a pointer to a Zip record, or 0 if not found.
// <start> is the expected position; we scan from there until EOF for
// the given ID (fourcc). <record_size> (includes ID field) bytes must
// remain before EOF - this makes sure the record is completely in the file.
// used by z_find_ecdr and z_extract_cdfh.
static const u8* z_find_id(const u8* file, size_t size, const u8* start, const char id[5], size_t record_size)
{
	ssize_t bytes_left = (ssize_t)((file+size) - start - record_size);

	const u8* p = start;
		// don't increment function argument directly,
		// so we can warn the user if we had to scan.

	while(bytes_left-- >= 0)
	{
		// found it
		if(*(u32*)p == *(u32*)id)
		{
#ifndef NDEBUG
			if(p != start)
				debug_warn("z_find_id: archive damaged, but still found next record.");
#endif
			return p;
		}

		p++;
			// be careful not to increment before comparison;
			// id may already be at <start>.
	}

	// passed EOF, didn't find it.
	debug_warn("z_find_id: archive corrupted, next record not found.");
	return 0;
}


// find "End of Central Dir Record" in file.
// z_validate has made sure size >= ECDR_SIZE.
// return -1 on failure (output param invalid), otherwise 0.
static int z_find_ecdr(const u8* file, size_t size, const u8*& ecdr_)
{
	// early out: check expected case (ECDR at EOF; no file comment)
	const u8* ecdr = file + size - ECDR_SIZE;
	if(*(u32*)ecdr == *(u32*)&ecdr_id)
		goto found_ecdr;

	// scan the last 66000 bytes of file for ecdr_id signature
	// (the Zip archive comment field, up to 64k, may follow ECDR).
	// if the zip file is < 66000 bytes, scan the whole file.
	const u8* start = file + size - MIN(66000, size);
	ecdr = z_find_id(file, size, start, ecdr_id, ECDR_SIZE);
	if(!ecdr)
		return ERR_CORRUPTED;

found_ecdr:
	ecdr_ = ecdr;
	return 0;
}


//
// date conversion from DOS to Unix
//
///////////////////////////////////////////////////////////////////////////////

static uint bits(uint num, uint lo_idx, uint hi_idx)
{
	uint result = num;
	result >>= lo_idx;
	const uint count = (hi_idx - lo_idx)+1;
	// number of bits to return
	result &= (1u << count)-1;
	return result;
}


static time_t convert_dos_date(u16 fatdate, u16 fattime)
{
	struct tm t;							// struct tm format:
	t.tm_sec   = bits(fattime, 0,4) * 2;	// [0,59]
	t.tm_min   = bits(fattime, 5,10);		// [0,59]
	t.tm_hour  = bits(fattime, 11,15);		// [0,23]
	t.tm_mday  = bits(fatdate, 0,4);		// [1,31]
	t.tm_mon   = bits(fatdate, 5,8)-1;		// [0,11]
	t.tm_year  = bits(fatdate, 9,15) + 80;	// since 1900
	t.tm_isdst = -1;	// unknown - let libc determine

	assert(t.tm_year < 138);
		// otherwise: totally bogus, and at the limit of 32-bit time_t

	time_t ret = mktime(&t);
	if(ret == (time_t)-1)
		debug_warn("convert_dos_date: mktime failed");
	return ret;
}


///////////////////////////////////////////////////////////////////////////////


enum z_extract_cdfh_ret
{
	Z_CDFH_ABORT   = -1,	// next CDFH not found; abort.
	Z_CDFH_FILE_OK =  0,	// valid file; add to lookup.
	Z_CDFH_SKIPPED =  1		// not valid file, but have next CDFH; continue.
};

// read the current CDFH. if a valid file, return its filename and ZLoc.
// finally, advance to next CDFH.
// return -1 on error (output params invalid), or 0 on success.
// called by z_enum_files, which passes the output to lookup.
static int z_extract_cdfh(const u8* file, size_t size,              // in
	const u8*& cdfh, const char*& fn, size_t& fn_len, ZLoc* loc)    // out
{
	// scan for next CDFH (at or beyond current cdfh position)
	cdfh = z_find_id(file, size, cdfh, cdfh_id, CDFH_SIZE);
	if(!cdfh)					// no (further) CDFH found:
		return Z_CDFH_ABORT;	// caller will abort.

	// extract fields from CDFH
	const u16 method  = read_le16(cdfh+10);
	const u16 fattime = read_le16(cdfh+12);
	const u16 fatdate = read_le16(cdfh+14);
	const u32 csize   = read_le32(cdfh+20);
	const u32 ucsize  = read_le32(cdfh+24);
	const u16 fn_len_ = read_le16(cdfh+28);
	const u16 e_len   = read_le16(cdfh+30);
	const u16 c_len   = read_le16(cdfh+32);
	const u32 lfh_ofs = read_le32(cdfh+42);
	const char* fn_ = (const char*)cdfh+CDFH_SIZE;
		// not 0-terminated!

	// find corresponding LFH, needed to calculate file offset
	// (its extra field may not match that reported by CDFH!).
	// TODO: this is slow, due to seeking backwards.
	// optimization: calculate only on demand (i.e. open, not mount)?
	const u8* lfh = z_find_id(file, size, (u8*)file+lfh_ofs, lfh_id, LFH_SIZE);

	// advance CDFH; we now know where the next CDFH entry should be,
	// but will still scan ahead for its id on next call.
	cdfh += CDFH_SIZE + fn_len_ + e_len + c_len;

	// is this entry not a valid file?
	if(
		// compression method is unknown (neither deflated nor stored)
		(method & ~8) ||
		// it's a directory entry (we only want files).
		(!csize && !ucsize) ||
		// LFH signature not found
		(!lfh)
	)
		return Z_CDFH_SKIPPED;

	// get actual file ofs (see above)
	const u16 lfh_fn_len = read_le16(lfh+26);
	const u16 lfh_e_len  = read_le16(lfh+28);
	const off_t file_ofs = lfh_ofs + LFH_SIZE + lfh_fn_len + lfh_e_len;
		// LFH doesn't have a comment field!

	// write out entry data
	fn     = fn_;
	fn_len = fn_len_;
	loc->ofs    = file_ofs;
	loc->csize  = (off_t)(method? csize : 0);
		// if not compressed, csize = 0 (see zfile_compressed)
	loc->ucsize = (off_t)ucsize;
	loc->mtime  = convert_dos_date(fatdate, fattime);

	return Z_CDFH_FILE_OK;
}


// successively call <cb> for each valid file in the archive,
// passing the complete path and <user>.
// if it returns a nonzero value, abort and return that, otherwise 0.
//
// HACK: call back with negative index the first time; its abs. value is
// the number of entries in the archive. lookup needs to know this so it can
// preallocate memory. having lookup_init call z_get_num_files and then
// z_enum_files would require passing around a ZipInfo struct, or searching
// for the ECDR twice - both ways aren't nice. nor is expanding on demand -
// we try to minimize allocations (faster, less fragmentation).

// fn (filename) is not necessarily 0-terminated!
// loc is only valid during the callback! must be copied or saved.
typedef int(*CDFH_CB)(uintptr_t user, i32 idx, const char* fn, size_t fn_len, const ZLoc* loc);

static int z_enum_files(const u8* file, const size_t size, const CDFH_CB cb, const uintptr_t user)
{
	// find "End of Central Directory Record"
	const u8* ecdr;
	CHECK_ERR(z_find_ecdr(file, size, ecdr));

	// call back with number of entries in archives (an upper bound
	// for valid files; we're not interested in the directory entries).
	// we'd have to scan through the central dir to count them out; we'll
	// just skip them and waste a bit of preallocated memory.
	const i32 num_entries = read_le16(ecdr+10);
	// .. callback expects -num_entries < 0.
	//    if it's 0, the callback would treat it as an index => crash.
	if(!num_entries)
		return -1;
	CHECK_ERR(cb(user, -num_entries, 0, 0, 0));

	// iterate through CDFH
	const u32 cd_ofs = read_le32(ecdr+16);
	const u8* cdfh = (const u8*)file + cd_ofs;
	i32 idx = 0;
		// only incremented when valid, so we don't leave holes
		// in lookup's arrays (bad locality).

	for(i32 i = 0; i < num_entries; i++)
	{
		const char* fn;
		size_t fn_len;
		ZLoc loc;
		int ret = z_extract_cdfh(file, size, cdfh, fn, fn_len, &loc);
		// valid file
		if(ret == Z_CDFH_FILE_OK)
		{
			cb(user, idx, fn, fn_len, &loc);
			idx++;	// see rationale above
		}
		// next CDFH not found (Zip archive corrupted)
		else if(ret == Z_CDFH_ABORT)
			return -1;
		// skipping this CDFH (e.g. if directory)
		else
			;
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// lookup_*: file lookup
// per archive: return file info (e.g. offset, size), given filename.
//
///////////////////////////////////////////////////////////////////////////////


// current file-lookup implementation:
// store each file's ZEnt in an array. check the next entry first; if that's
// not what we're looking for, find its index via map<filename_hash, index>.
//
// rationale:
// - we don't export a "key" (currently array index) that would allow faster
//   file lookup. this would only be useful if higher-level code were to
//   store the key and use it more than once. also, lookup is currently fast
//   enough. finally, this would also make our file enumerate callback
//   incompatible with the others (due to the extra key param).
//
// - we don't bother with a directory tree to speed up lookup. the above
//   is fast enough: O(1) if accessed sequentially, otherwise O(log(files)).


struct ZEnt
{
	const char* fn;		// currently allocated individually

	ZLoc loc;
};


typedef std::map<FnHash, i32> LookupIdx;
typedef LookupIdx::iterator LookupIdxIt;

// per-archive information for mapping filename -> file info
struct LookupInfo
{
	ZEnt* ents;
	FnHash* fn_hashes;
		// split out of ZEnt for more efficient search
		// (=> ZEnt is power-of-2, back-to-back fn_hashes)
		//
		// currently both share one memory allocation; only mem_free() ents!

	i32 num_entries;
		// .. in above arrays (used to check indices)

	i32 num_files;
		// actual number of valid files! (see z_enum_files)

	i32 next_file;
		// for last-file-opened optimization.
		// we store index of next file instead of the last one opened
		// to avoid trouble on first call (don't want last == -1).

	// don't know size of std::map, and this struct is
	// included in a control block (ZArchive).
	// allocate dynamically to be safe.
	LookupIdx* idx;
};


// write a lower-case copy of <src> to <dst>, which holds <buf_size> bytes.
// up to buf_size-1 chars are written; we always 0-terminate the output!
//
// this routine is used to convert OS and user-specified filenames
// to lowercase before hashing them and then comparing.
static void copy_lower_case(char* dst, const char* src, size_t buf_size)
{
	assert(buf_size > 0);	// otherwise, no room for trailing '\0'

	int c;
	do
	{
		c = *src++;
		// this is the last remaining byte in the buffer.
		// loop will exit below after writing 0-terminator.
		if(--buf_size == 0)
			c = '\0';
		*dst++ = tolower(c);
	}
	while(c != '\0');
}


// add file <fn> to the lookup data structure.
// called from z_enum_files in order (0 <= idx < num_entries).
// the first call notifies us of # entries, so we can allocate memory.
//
// notes:
// - fn (filename) is not necessarily 0-terminated!
// - loc is only valid during the callback! must be copied or saved.
static int lookup_add_file_cb(uintptr_t user, i32 idx,
	const char* fn, size_t fn_len, const ZLoc* loc)
{
	LookupInfo* li = (LookupInfo*)user;

	// HACK: on first call, idx is negative and tells us how many
	// entries are in the archive (so we can allocate memory).
	// see z_enum_files for why it's done this way.
	if(idx < 0)
	{
		const i32 num_entries = -idx;

		// both arrays in one allocation (more efficient)
		const size_t ents_size = (num_entries * sizeof(ZEnt));
		const size_t array_size = ents_size + (num_entries * sizeof(FnHash));
		void* p = mem_alloc(array_size, 4*KB);
		if(!p)
			return ERR_NO_MEM;

		li->num_entries = num_entries;
		li->num_files = 0;
			// will count below, since some entries aren't files.
		li->ents = (ZEnt*)p;
		li->fn_hashes = (FnHash*)((char*)p + ents_size);
		return 0;
	}

	// adding a regular file.

	assert(idx < li->num_entries);

	// hash (lower case!) filename
	char lc_fn[PATH_MAX];
	size_t max_size = fn_len+1;	// fn not 0-terminated
	if(max_size > PATH_MAX)		// (this avoids stupid min() type warning)
		max_size = PATH_MAX;	// clamp to actual buffer size
	copy_lower_case(lc_fn, fn, max_size);
	FnHash fn_hash = fnv_hash(lc_fn);

	// fill ZEnt
	ZEnt* ent = li->ents + idx;
	ent->loc = *loc;
	// .. copy filename (needs to be 0-terminated)
	//    note: Zip paths only have '/' terminators; no need to convert.
	char* fn_copy = (char*)malloc(fn_len+1);
	if(!fn_copy)
		return ERR_NO_MEM;
	memcpy(fn_copy, fn, fn_len);
	fn_copy[fn_len] = '\0';
	ent->fn = fn_copy;

	li->num_files++;
	li->fn_hashes[idx] = fn_hash;
	(*li->idx)[fn_hash] = idx;

	return 0;
}


// initialize lookup data structure for the given Zip archive:
// adds all files to the index.
static int lookup_init(LookupInfo* li, const u8* file, const size_t size)
{
	int err;

	// check if it's even a Zip file.
	// the VFS blindly opens files when mounting; it needs to open
	// all archives, but doesn't know their extension (e.g. ".pk3").
	err = z_validate(file, size);
	if(err < 0)		// don't CHECK_ERR - this can happen often.
		return err;

	li->next_file = 0;
	li->idx = new LookupIdx;
		// ents, fn_hashes, num_files are initialized in lookup_add_file_cb

	err = z_enum_files(file, size, lookup_add_file_cb, (uintptr_t)li);
	if(err < 0)
	{
		delete li->idx;
		return err;
	}

	return 0;
}


// free lookup data structure.
// (no use-after-free checking - that's handled by the VFS)
static int lookup_free(LookupInfo* li)
{
	// free memory allocated for filenames
	for(i32 i = 0; i < li->num_files; i++)
	{
		free((void*)li->ents[i].fn);
		li->ents[i].fn = 0;
	}

	li->num_files = 0;

	delete li->idx;

	// frees both ents and fn_hashes! (they share an allocation)
	return mem_free(li->ents);
}


// look up ZLoc, given filename (untrusted!).
static int lookup_get_file_info(LookupInfo* li, const char* fn, ZLoc* loc)
{
	// hash (lower case!) filename
	char lc_fn[PATH_MAX];
	copy_lower_case(lc_fn, fn, sizeof(lc_fn));
	const FnHash fn_hash = fnv_hash(lc_fn);

	const FnHash* fn_hashes = li->fn_hashes;
	const i32 num_files = li->num_files;
	i32 i = li->next_file;

	// early-out: check if the next entry is what we want
	if(i < num_files && fn_hashes[i] == fn_hash)
		goto have_idx;

	// .. no - consult index
	{
	LookupIdxIt it = li->idx->find(fn_hash);
	// not found: error
	if(it == li->idx->end())
		return ERR_FILE_NOT_FOUND;

	i = it->second;
	assert(0 <= i && i < li->num_files);
	}

have_idx:

	// indicate that this is the most recent entry touched
	li->next_file = i+1;

	*loc = li->ents[i].loc;
	return 0;
}


// successively call <cb> for each valid file in the index,
// passing the complete path and <user>.
// if it returns a nonzero value, abort and return that, otherwise 0.
static int lookup_enum_files(LookupInfo* li, FileCB cb, uintptr_t user)
{
	struct stat s;
	memset(&s, 0, sizeof(s));

	const ZEnt* ent = li->ents;
	for(i32 i = 0; i < li->num_files; i++, ent++)
	{
		s.st_mode  = S_IFREG;
		s.st_size  = (off_t)ent->loc.ucsize;
		s.st_mtime = ent->loc.mtime;

		int ret = cb(ent->fn, &s, user);
		if(ret != 0)
			return ret;
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// ZArchive_*: Handle-based container for archive info
// owns archive file and its lookup mechanism.
//
///////////////////////////////////////////////////////////////////////////////


struct ZArchive
{
	File f;

	LookupInfo li;

	// problem:
	//   if ZArchive_reload aborts due to file_open failing, ZArchive_dtor
	//   is called by h_alloc, and file_close complains the File is
	//   invalid (wasn't open). this happens e.g. if vfs_mount blindly
	//   tries to open a directory as an archive.
	// workaround:
	//   only free the above if ZArchive_reload succeeds, i.e. is_open.
	// note:
	//   if lookup_init fails after file_open opened the file,
	//   we wouldn't file_close in the dtor,
	//   but it's taken care of by ZArchive_reload.
	bool is_open;
};

H_TYPE_DEFINE(ZArchive);


static void ZArchive_init(ZArchive*, va_list)
{}


static void ZArchive_dtor(ZArchive* za)
{
	if(za->is_open)
	{
		file_close(&za->f);
		lookup_free(&za->li);

		za->is_open = false;
	}
}


static int ZArchive_reload(ZArchive* za, const char* fn, Handle)
{
	int err;

	// open
	err = file_open(fn, FILE_CACHE_BLOCK, &za->f);
	if(err < 0)
		// don't complain - this happens when vfs_mount blindly
		// zip_archive_opens a dir.
		return err;

	// map
	void* file;
	size_t size;
	err = file_map(&za->f, file, size);
	if(err < 0)
		goto exit_close;

	err = lookup_init(&za->li, (u8*)file, size);
	if(err < 0)
		goto exit_unmap_close;

	file_unmap(&za->f);
		// we map the file only for convenience when loading;
		// extraction is via aio (faster, better mem use).

	za->is_open = true;
	return 0;

exit_unmap_close:
	file_unmap(&za->f);
exit_close:
	file_close(&za->f);

	// don't complain here either; this happens when vfs_mount
	// zip_archive_opens an invalid file that's in a mount point dir.
	return err;
}


// open and return a handle to the zip archive indicated by <fn>.
// somewhat slow - each file is added to an internal index.
Handle zip_archive_open(const char* fn)
{
	return h_alloc(H_ZArchive, fn);
}


// close the archive <ha> and set ha to 0
int zip_archive_close(Handle& ha)
{
	return h_free(ha, H_ZArchive);
}


// successively call <cb> for each valid file in the archive <ha>,
// passing the complete path and <user>.
// if it returns a nonzero value, abort and return that, otherwise 0.
int zip_enum(const Handle ha, const FileCB cb, const uintptr_t user)
{
	H_DEREF(ha, ZArchive, za);

	return lookup_enum_files(&za->li, cb, user);
}


///////////////////////////////////////////////////////////////////////////////
//
// inf_*: in-memory inflate routines (zlib wrapper)
// decompresses blocks from file_io callback.
//
///////////////////////////////////////////////////////////////////////////////

// must be dynamically allocated - need one for every open ZFile,
// and z_stream is large.
struct InfCtx
{
	z_stream zs;

	void* in_buf;
		// 0 until inf_inflate called with free_in_buf = true.
		// mem_free-d after consumed by inf_inflate, or by inf_free.
		// note: necessary; can't just use next_in-total_in, because
		// we may inflate in chunks.
		//
		// can't have this owned (i.e. allocated) by inf_, because
		// there can be several IOs in-flight and therefore buffers of
		// compressed data. we'd need a list if stored here; having the
		// IOs store them and pass them to us is more convenient.

	bool compressed;
};

// allocate a new context.
static uintptr_t inf_init_ctx(bool compressed)
{
#ifdef NO_ZLIB
	return 0;
#else
	// allocate ZLib stream
	const size_t size = round_up(sizeof(InfCtx), 32);
		// be nice to allocator
	InfCtx* ctx = (InfCtx*)calloc(size, 1);
	if(inflateInit2(&ctx->zs, -MAX_WBITS) != Z_OK)
		// -MAX_WBITS indicates no zlib header present
		return 0;

	ctx->compressed = compressed;

	return (uintptr_t)ctx;
#endif
}


// convenience - both inf_inflate and inf_free use this.
static void free_in_buf(InfCtx* ctx)
{
	mem_free(ctx->in_buf);
	ctx->in_buf = 0;
}


// subsequent calls to inf_inflate will unzip into <out>.
int inf_set_dest(uintptr_t _ctx, void* out, size_t out_size)
{
#ifdef NO_ZLIB
	return -1;
#else
	InfCtx* ctx = (InfCtx*)_ctx;
	z_stream* zs = &ctx->zs;

	if(zs->next_out || zs->avail_out)
	{
		debug_warn("zip_set_dest: ctx already in use!");
		return -1;
	}
	zs->next_out  = (Byte*)out;
	zs->avail_out = (uInt)out_size;
	return 0;
#endif
}


// unzip into output buffer. returns bytes written
// (may be 0, if not enough data is passed in), or < 0 on error.
ssize_t inf_inflate(uintptr_t _ctx, void* in, size_t in_size, bool free_in_buf = false)
{
#ifdef NO_ZLIB
	return -1;
#else
	InfCtx* ctx = (InfCtx*)_ctx;
	z_stream* zs = &ctx->zs;


	size_t prev_avail_out = zs->avail_out;

	if(in)
	{
		if(zs->avail_in || ctx->in_buf)
			debug_warn("inf_inflate: previous input buffer not empty");
		zs->avail_in = (uInt)in_size;
		zs->next_in = (Byte*)in;

		if(free_in_buf)
			ctx->in_buf = in;
	}

	int err = 0;

	if(ctx->compressed)
		err = inflate(zs, Z_SYNC_FLUSH);
	else
	{
		memcpy(zs->next_out, zs->next_in, zs->avail_in);
		uInt size = zs->avail_in;
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
	assert(avail_out <= prev_avail_out);
		// make sure output buffer size didn't magically increase
	ssize_t nread = (ssize_t)(prev_avail_out - avail_out);
	if(!nread)
		return (err < 0)? err : 0;
		// try to pass along the ZLib error code, but make sure
		// it isn't treated as 'bytes output', i.e. > 0.

	return nread;
#endif
}


// free the given context.
int inf_free_ctx(uintptr_t _ctx)
{
#ifdef NO_ZLIB
	return -1;
#else
	InfCtx* ctx = (InfCtx*)_ctx;
	z_stream* zs = &ctx->zs;

	free_in_buf(ctx);

	// can have both input or output data remaining
	// (if not all data in uncompressed stream was needed)

	inflateEnd(zs);
	free(ctx);
	return 0;
#endif
}


///////////////////////////////////////////////////////////////////////////////
//
// zip_*: file from Zip archive
// uses lookup to get file information; holds inflate state.
//
///////////////////////////////////////////////////////////////////////////////


enum ZFileFlags
{
	// the ZFile has been successfully zip_map-ped.
	// used to make sure the archive's mmap refcount remains balanced,
	// i.e. no one double-frees the mapping.
	ZF_HAS_MAPPING = 0x4000
};

// marker for ZFile struct, to make sure it's valid
#ifdef PARANOIA
static const u32 ZFILE_MAGIC = FOURCC('Z','F','I','L');
#endif


// return 0 <==> ZFile seems valid
static int zfile_validate(uint line, ZFile* zf)
{
	const char* msg = "";
	int err = -1;

	if(!zf)
	{
		msg = "ZFile* parameter = 0";
		err = ERR_INVALID_PARAM;
	}
#ifdef PARANOIA
	else if(zf->magic != ZFILE_MAGIC)
		msg = "ZFile corrupted (magic field incorrect)";
#endif
#ifndef NDEBUG
//	else if(!h_user_data(zf->ha, H_ZArchive))
//		msg = "invalid archive handle";
	// disabled: happens at shutdown because handles are freed out-of order;
	// archive is freed before its files, making its Handle invalid
#endif
	else if(!zf->ucsize)
		msg = "ucsize = 0";
	else if(!zf->inf_ctx)
		msg = "read context invalid";
	// everything is OK
	else
		return 0;

	// failed somewhere - err is the error code,
	// or -1 if not set specifically above.
	debug_out("zfile_validate at line %d failed: %s\n", line, msg);
	debug_warn("zfile_validate failed");
	return err;
}

#define CHECK_ZFILE(f)\
do\
{\
	int err = zfile_validate(__LINE__, f);\
	if(err < 0)\
		return err;\
}\
while(0);


// convenience function, allows implementation change in ZFile.
// note that size == ucsize isn't foolproof, and adding a flag to
// ofs or size is ugly and error-prone.
// no error checking - always called from functions that check zf.
static inline bool zfile_compressed(ZFile* zf)
{
	return zf->csize != 0;
}




// get file status (size, mtime). output param is zeroed on error.
int zip_stat(Handle ha, const char* fn, struct stat* s)
{
	// zero output param in case we fail below.
	memset(s, 0, sizeof(struct stat));

	H_DEREF(ha, ZArchive, za);
	LookupInfo* li = &za->li;

	ZLoc loc;
	CHECK_ERR(lookup_get_file_info(li, fn, &loc));

	s->st_size  = loc.ucsize;
	s->st_mtime = loc.mtime;
	return 0;
}


// open file, and fill *zf with information about it.
// return < 0 on error (output param zeroed). 
int zip_open(const Handle ha, const char* fn, ZFile* zf)
{
	// zero output param in case we fail below.
	memset(zf, 0, sizeof(ZFile));

	H_DEREF(ha, ZArchive, za);
	LookupInfo* li = (LookupInfo*)&za->li;

	ZLoc loc;
		// don't want ZFile to contain a ZEnt struct -
		// its ucsize member must be 'loose' for compatibility with File.
		// => need to copy ZEnt fields into ZFile.
	CHECK_ERR(lookup_get_file_info(li, fn, &loc));

#ifdef PARANOIA
	zf->magic    = ZFILE_MAGIC;
#endif

	zf->ucsize   = loc.ucsize;
	zf->ofs      = loc.ofs;
	zf->csize    = loc.csize;

	zf->ha       = ha;
	zf->inf_ctx  = inf_init_ctx(zfile_compressed(zf));

	CHECK_ZFILE(zf);

	return 0;
}


// close file.
int zip_close(ZFile* zf)
{
	CHECK_ZFILE(zf);

	// remaining ZFile fields don't need to be freed/cleared
	return inf_free_ctx(zf->inf_ctx);
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


static const size_t CHUNK_SIZE = 16*KB;

// begin transferring <size> bytes, starting at <ofs>. get result
// with zip_wait_io; when no longer needed, free via zip_discard_io.
int zip_start_io(ZFile* zf, off_t user_ofs, size_t max_output_size, void* user_buf, ZipIO* io)
{
	// not needed, since ZFile tells us the last read offset in the file.
	UNUSED(user_ofs);

	// zero output param in case we fail below.
	memset(io, 0, sizeof(ZipIO));

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
		CHECK_ERR(inf_set_dest(io->inf_ctx, io->user_buf, io->max_output_size));
		ssize_t bytes_inflated = inf_inflate(io->inf_ctx, 0, 0);
		CHECK_ERR(bytes_inflated);
		if(bytes_inflated == max_output_size)
		{
			io->already_inflated = true;
			io->max_output_size = bytes_inflated;
			return 0;
		}

		// read up to next chunk (so that the next read is aligned -
		// less work for aio) or up to EOF.
		const ssize_t left_in_chunk = CHUNK_SIZE - (ofs % CHUNK_SIZE);
		const ssize_t left_in_file = zf->csize - ofs;
		size = MIN(left_in_chunk, left_in_file);

		// note: only need to clamp if compressed

		buf = mem_alloc(size, 4*KB);
	}
	// else: not compressed; we'll just read directly from the archive file.
	// no need to clamp to EOF - that's done already by the VFS.

	zf->last_read_ofs += (off_t)size;

	CHECK_ERR(file_start_io(&za->f, ofs, size, buf, &io->io));

	return 0;
}


// indicates if the IO referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
int zip_io_complete(ZipIO* io)
{
	if(io->already_inflated)
		return 1;
	return file_io_complete(&io->io);
}


// wait until the transfer <io> completes, and return its buffer.
// output parameters are zeroed on error.
int zip_wait_io(ZipIO* io, void*& buf, size_t& size)
{
	buf  = io->user_buf;
	size = io->max_output_size;
	if(io->already_inflated)
		return 0;

	void* raw_buf;
	size_t raw_size;
	CHECK_ERR(file_wait_io(&io->io, raw_buf, raw_size));

	if(io->inf_ctx)
	{
		inf_set_dest(io->inf_ctx, buf, size);
		ssize_t bytes_inflated = inf_inflate(io->inf_ctx, raw_buf, raw_size, true);
			// true: we allocated the compressed data input buffer, and
			// want it freed when it's consumed.
	}
	else
	{
		buf  = raw_buf;
		size = raw_size;
	}

	return 0;
}


// finished with transfer <io> - free its buffer (returned by zip_wait_io)
int zip_discard_io(ZipIO* io)
{
	if(io->already_inflated)
		return 0;
	return file_discard_io(&io->io);
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

	ssize_t ucsize = inf_inflate(p->inf_ctx, buf, size);

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

#include "timer.h"



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

	const bool compressed = zfile_compressed(zf);

	ZArchive* za = H_USER_DATA(zf->ha, ZArchive);
	if(!za)
		return ERR_INVALID_HANDLE;

	ofs += zf->ofs;

	// pump all previous cdata out of inflate context
	// if that satisfied the request, we're done



	// not compressed - just pass it on to file_io
	// (avoid the Zip inflate start/finish stuff below)
//	if(!compressed)
//		return file_io(&za->f, ofs, csize, p);
		// no need to set last_raw_ofs - only checked if compressed.

	// compressed

	CHECK_ERR(inf_set_dest(zf->inf_ctx, p, size));

	/*
	static bool once = false;
	if(!once)
	{

	once=true;
	uintptr_t xctx = inf_init_ctx();
	size_t xsize = za->f.size;
	void* xbuf=mem_alloc(xsize, 65536);
	inf_set_dest(xctx, xbuf, xsize);
	const IOCBParams xparams = { xctx, false, 0, 0 };
	double t1 = get_time();
	file_io(&za->f,0, xsize, 0, io_cb, (uintptr_t)&xparams);
	double t2 = get_time();
	debug_out("\n\ntime to load whole archive %f\nthroughput %f MB/s\n", t2-t1, xsize / (t2-t1) / 1e6);
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
int zip_map(ZFile* zf, void*& p, size_t& size)
{
	p = 0;
	size = 0;

	CHECK_ZFILE(zf);

	// mapping compressed files doesn't make sense because the
	// compression algorithm is unspecified - disallow it.
	if(zfile_compressed(zf))
	{
		debug_warn("zip_map: file is compressed");
		return -1;
	}

	// note: we mapped the archive in zip_archive_open, but unmapped it
	// in the meantime to save memory in case it wasn't going to be mapped.
	// now we do so again; it's unmapped in zip_unmap (refcounted).
	H_DEREF(zf->ha, ZArchive, za);
	void* archive_p;
	size_t archive_size;
	CHECK_ERR(file_map(&za->f, archive_p, archive_size));

	p = (char*)archive_p + zf->ofs;
	size = zf->ucsize;

	zf->flags |= ZF_HAS_MAPPING;
	return 0;
}


// remove the mapping of file <zf>; fail if not mapped.
//
// the mapping will be removed (if still open) when its archive is closed.
// however, map/unmap calls should be paired so that the archive mapping
// may be removed when no longer needed.
int zip_unmap(ZFile* zf)
{
	CHECK_ZFILE(zf);

	// make sure archive mapping refcount remains balanced:
	// don't allow multiple|"false" unmaps.
	if(!(zf->flags & ZF_HAS_MAPPING))
		return -1;
	zf->flags &= ~ZF_HAS_MAPPING;

	H_DEREF(zf->ha, ZArchive, za);
	return file_unmap(&za->f);
}
