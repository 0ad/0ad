// Zip archiving on top of ZLib.
//
// Copyright (c) 2004 Jan Wassenberg
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

#include "precompiled.h"

#include "lib.h"
#include "zip.h"
#include "res.h"

#include <assert.h>

// provision for removing all ZLib code (all inflate calls will fail).
// used for checking DLL dependency; might also simulate corrupt Zip files.
//#define NO_ZLIB

#ifndef NO_ZLIB
#include <zlib.h>
#ifdef _MSC_VER
#pragma comment(lib, "zdll.lib")
#endif
#endif

#include <map>


///////////////////////////////////////////////////////////////////////////////
//
// Zip-specific code
// passes list of files in archive to lookup
//
///////////////////////////////////////////////////////////////////////////////


// convenience container for location / size of file in archive.
struct ZFileLoc
{
	off_t ofs;
	off_t csize;	// = 0 if not compressed
	off_t ucsize;

	// why csize?
	// file I/O may be N-buffered, so it's good to know when the raw data
	// stops (or else we potentially overshoot by N-1 blocks),
	// but not critical, since Zip files are compressed individually.
	// (if we read too much, it's ignored by inflate).
	//
	// we also need a way to check if file compressed (e.g. to fail mmap
	// requests if the file is compressed). packing a bit in ofs or
	// ucsize is error prone and ugly (1 bit less won't hurt though).
	// any other way will mess up the nice 2^n byte size anyway, so
	// might as well store csize.
};


static inline int zip_validate(const void* const file, const size_t size)
{
	if(size < 2)
		return -1;

	const u8* p = (const u8*)file;
	if(p[0] != 'P' || p[1] != 'K')
		return -1;

	return 0;
}


// find end of central dir record in file (loaded or mapped).
static int zip_find_ecdr(const void* const file, const size_t size, const u8*& ecdr_)
{
	const char ecdr_id[] = "PK\5\6";	// signature
	const size_t ECDR_SIZE = 22;

	if(size < ECDR_SIZE)
	{
		debug_warn("zip_find_ecdr: size is way too small");
		return -1;
	}
	const u8* ecdr = (const u8*)file + size - ECDR_SIZE;

	// early out: check expected case (ECDR at EOF; no file comment)
	if(*(u32*)ecdr == *(u32*)&ecdr_id)
		goto found_ecdr;

	{

	// scan the last 66000 bytes of file for ecdr_id signature
	// (zip comment <= 65535 bytes, sizeof(ECDR) = 22, add some for safety)
	// if the zip file is < 66000 bytes, scan the whole file.

	size_t bytes_left = MIN(66000, size);
	ecdr = (const u8*)file + size - bytes_left;

	while(bytes_left >= 4)
	{
		if(*(u32*)ecdr == *(u32*)&ecdr_id)
			goto found_ecdr;

		// check next 4 bytes (unaligned!!)
		ecdr++;
		bytes_left--;
	}

	// reached EOF and still haven't found the ECDR identifier.
	ecdr_ = 0;
	return -1;

	}

found_ecdr:
	ecdr_ = ecdr;
	return 0;
}


#ifdef PARANOIA

// make sure the LFH fields match those passed (from the CDFH).
// only used in PARANOIA builds - costs time when opening archives.
static int zip_verify_lfh(const void* const file, const off_t lfh_ofs, const off_t file_ofs)
{
	const char lfh_id[] = "PK\3\4";	// signature
	const size_t LFH_SIZE = 30;

	assert(lfh_ofs < file_ofs);	// header comes before file

	const u8* lfh = (const u8*)file + lfh_ofs;

	if(*(u32*)lfh != *(u32*)lfh_id)
	{
		debug_warn("LFH corrupt! (signature doesn't match)");
		return -1;
	}

	const u16 lfh_fn_len = read_le16(lfh+26);
	const u16 lfh_e_len  = read_le16(lfh+28);

	const off_t lfh_file_ofs = lfh_ofs + LFH_SIZE + lfh_fn_len + lfh_e_len;

	if(file_ofs != lfh_file_ofs)
	{
		debug_warn("warning: CDFH and LFH data differ! normal builds will"\
			        "return incorrect file offsets. check Zip file!");
		return -1;
	}

	return 0;
}

#endif	// #ifdef PARANOIA


// extract information from the current Central Directory File Header;
// advance cdfh to point to the next; return -1 on unrecoverable error,
// 0 on success (<==> output fields are valid), > 0 if file is invalid.
static int zip_read_cdfh(const u8*& cdfh, const char*& fn, size_t& fn_len, ZFileLoc* const loc)
{
	const char cdfh_id[] = "PK\1\2";	// signature
	const size_t CDFH_SIZE = 46;
	const size_t LFH_SIZE  = 30;

	if(*(u32*)cdfh != *(u32*)cdfh_id)
	{
		debug_warn("CDFH corrupt! (signature doesn't match)");
		goto skip_file;
	}

	{

	const u8  method  = cdfh[10];
	const u32 csize_  = read_le32(cdfh+20);
	const u32 ucsize_ = read_le32(cdfh+24);
	const u16 fn_len_ = read_le16(cdfh+28);
	const u16 e_len   = read_le16(cdfh+30);
	const u16 c_len   = read_le16(cdfh+32);
	const u32 lfh_ofs = read_le32(cdfh+42);

	const char* fn_   = (const char*)cdfh+CDFH_SIZE;
		// not 0-terminated!

	// compression method: neither deflated nor stored
	if(method & ~8)
	{
		debug_warn("warning: unknown compression method");
		goto skip_file;
	}

	fn     = fn_;
	fn_len = fn_len_;

	loc->ofs    = (off_t)(lfh_ofs + LFH_SIZE + fn_len_ + e_len);
	loc->csize  = (off_t)csize_;
	loc->ucsize = (off_t)ucsize_;

	// performance issue: want to avoid seeking between LFHs and central dir.
	// would be safer to calculate file offset from the LFH, since its
	// filename / extra data fields may differ WRT the CDFH version.
	// we don't bother checking for this in normal builds: if they were
	// to be different, we'd notice: headers of files would end up corrupted.
#ifdef PARANOIA
	if(!zip_verify_lfh(file, lfh_ofs, file_ofs))
		goto skip_file;
#endif

	cdfh += CDFH_SIZE + fn_len + e_len + c_len;
	return 0;

	}

// file was invalid somehow; try to seek forward to the next CDFH
skip_file:
	// scan for next CDFH (look for signature)
	for(int i = 0; i < 256; i++)
	{
		if(*(u32*)cdfh == *(u32*)cdfh_id)
			goto found_next_cdfh;
		cdfh++;
	}

	// next CDFH not found. caller must abort
	return -1;

// file was skipped, but we have the next CDFH
found_next_cdfh:
	return 1;
}


// fn (filename) is not necessarily 0-terminated!
// loc is only valid during the callback! must be copied or saved.
typedef int(*ZipCdfhCB)(const uintptr_t user, const i32 idx, const char* const fn, const size_t fn_len, const ZFileLoc* const loc);

// go through central directory of the Zip file (loaded or mapped into memory);
// call back for each file.
//
// HACK: call back with negative index the first time; its abs. value is
// the number of files in the archive. lookup needs to know this so it can
// allocate memory. having lookup_init call zip_get_num_files and then
// zip_enum_files would require passing around a ZipInfo struct,
// or searching for the ECDR twice - both ways aren't nice.
static int zip_enum_files(const void* const file, const size_t size, const ZipCdfhCB cb, const uintptr_t user)
{
	int err;
	ZFileLoc loc;

	// find End of Central Directory Record
	const u8* ecdr;
	err = zip_find_ecdr(file, size, ecdr);
	if(err < 0)
		return err;

	// call back with number of files in archive
	const i32 num_files = read_le16(ecdr+10);
	// .. callback expects -num_files < 0.
	//    if it's 0, the callback would treat it as an index => crash.
	if(!num_files)
		return -1;
	err = cb(user, -num_files, 0, 0, 0);
	if(err < 0)
		return err;

	const u32 cd_ofs = read_le32(ecdr+16);
	const u8* cdfh = (const u8*)file + cd_ofs;
		// pointer is advanced in zip_read_cdfh

	for(i32 idx = 0; idx < num_files; idx++)
	{
		const char* fn;
		size_t fn_len;
		err = zip_read_cdfh(cdfh, fn, fn_len, &loc);
		// .. non-recoverable error reading dir
		if(err < 0)
			return err;
		// .. file was skipped (e.g. invalid compression format)
		//    call back with 0 params; don't skip the file, so that
		//    all indices are valid.
		if(err > 0)
			cb(user, idx, 0, 0, 0);
		// .. file valid.
		else
			cb(user, idx, fn, fn_len, &loc);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// file lookup
// per archive: find file information (e.g. location, size), given filename.
//
///////////////////////////////////////////////////////////////////////////////


// file lookup: store array of files in archive (ZEnt); lookup via linear
//   search for hash of filename.
// optimization: store index of last file opened; check the one after
//   that first, and only then search the whole array. this is a big
//   win if files are opened sequentially (they should be so ordered
//   in the archive anyway, to reduce seeks)
//

// rationale: added index because exposing index is bad (say we change lookup data struct)
// much cleaner to only export handle

// don't bother making a tree structure: first, it's a bit of work
// (Zip files store paths as part of the file name - there's no extra
// directory information); second, the VFS file location DB stores
// handle and file index per file, making its lookup constant-time.

struct ZEnt
{
	const char* fn;		// currently allocated individually

	ZFileLoc loc;
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

	i32 num_files;

	i32 next_file;
		// for last-file-opened optimization.
		// we store index of next file instead of the last one opened
		// to avoid trouble on first call (don't want last == -1).

	// don't know size of std::map, and this is used in a control block.
	// allocate dynamically to save size.
	LookupIdx* idx;
};


// add file <fn> to the lookup data structure.
// callback for zip_enum_files, in order (0 <= idx < num_files).
//
// fn (filename) is not necessarily 0-terminated!
// loc is only valid during the callback! must be copied or saved.
static int lookup_add_file_cb(const uintptr_t user, const i32 idx, const char* const fn, const size_t fn_len, const ZFileLoc* const loc)
{
	LookupInfo* li = (LookupInfo*)user;

	// HACK: on first call, idx is negative and tells us how many
	// files are in the archive (so we can allocate memory).
	// see zip_enum_files for why it's done this way.
	if(idx < 0)
	{
		const i32 num_files = -idx;

		// both arrays in one allocation (more efficient)
		const size_t ents_size = (num_files * sizeof(ZEnt));
		const size_t array_size = ents_size + (num_files * sizeof(FnHash));
		void* p = mem_alloc(array_size, 4*KB);
		if(!p)
			return ERR_NO_MEM;

		li->num_files = num_files;
		li->ents = (ZEnt*)p;
		li->fn_hashes = (FnHash*)((char*)p + ents_size);
		return 0;
	}

	ZEnt* ent = li->ents + idx;

	FnHash fn_hash = fnv_hash(fn, fn_len);

	(*li->idx)[fn_hash] = idx;
	li->fn_hashes[idx] = fn_hash;

	// valid file - write out its info.
	if(loc)
	{
		ent->fn = (const char*)malloc(fn_len+1);
		if(!ent->fn)
			return ERR_NO_MEM;
		strncpy((char*)ent->fn, fn, fn_len);

		// 0-terminate and strip trailing '/'
		char* end = (char*)ent->fn + fn_len-1;
		if(*end != '/')
			end++;
		else
			end = end;
		*end = '\0';

		ent->loc = *loc;
	}
	// invalid file / error reading its dir entry: zero its file info.
	// (don't skip it to make sure all indices are valid).
	else
		memset(ent, 0, sizeof(ZEnt));

	return 0;
}


// initialize lookup data structure for Zip archive <file>
static int lookup_init(LookupInfo* const li, const void* const file, const size_t size)
{
	// all other fields are initialized in lookup_add_file_cb
	li->next_file = 0;

	li->idx = new LookupIdx;

	int err = zip_enum_files(file, size, lookup_add_file_cb, (uintptr_t)li);
	if(err < 0)
	{
		delete li->idx;
		return err;
	}

	return 0;
}


// free lookup data structure. no use-after-free checking.
static int lookup_free(LookupInfo* const li)
{
	for(i32 i = 0; i < li->num_files; i++)
	{
		free((void*)li->ents[i].fn);
		li->ents[i].fn = 0;
	}

	li->num_files = 0;

	delete li->idx;

	return mem_free(li->ents);
}


// return key of file <fn> for use in lookup_get_file_info.
static int lookup_file(LookupInfo* const li, const char* const fn, i32& idx)
{
	const FnHash fn_hash = fnv_hash(fn);
	const FnHash* fn_hashes = li->fn_hashes;

	const i32 num_files = li->num_files;
	i32 i = li->next_file;

	// .. next_file marker is at the end of the array, or
	//    its entry isn't what we're looking for: consult index
	if(i >= num_files || fn_hashes[i] != fn_hash)
	{
		LookupIdxIt it = li->idx->find(fn_hash);
		// not found
		if(it == li->idx->end())
			return ERR_FILE_NOT_FOUND;

		i = it->second;
	}

	li->next_file = i+1;
	idx = i;
	return 0;
}


// return file information, given file key (from lookup_file).
static int lookup_get_file_info(LookupInfo* const li, const i32 idx, const char*& fn, ZFileLoc* const loc)
{
	if(idx < 0 || idx > li->num_files-1)
	{
		debug_warn("lookup_get_file_info: index out of bounds");
		return -1;
	}

	const ZEnt* const ent = &li->ents[idx];
	fn   = ent->fn;
	*loc = ent->loc;
	return 0;
}


typedef ZipFileCB LookupFileCB;

static int lookup_enum_files(LookupInfo* const li, LookupFileCB cb, uintptr_t user)
{
	int err;

	const ZEnt* ent = li->ents;
	for(i32 i = 0; i < li->num_files; i++, ent++)
	{
		int flags = LOC_ZIP;
		if(ent->loc.csize == 0 && ent->loc.ucsize == 0)
			flags |= LOC_DIR;
		err = cb(ent->fn, flags, (ssize_t)ent->loc.ucsize, user);
		if(err < 0)
			return 0;
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// container with handle for archive info
// owns archive file and its lookup mechanism.
//
///////////////////////////////////////////////////////////////////////////////


struct ZArchive
{
	File f;

	LookupInfo li;

	// hack: on first open, file is invalid (fn_hash isn't set),
	// and file validate in file_close fails.
	// workaround: only close if open.
	bool is_open;
};

H_TYPE_DEFINE(ZArchive);


static void ZArchive_init(ZArchive* za, va_list args)
{
	UNUSED(za);
	UNUSED(args);
}


static void ZArchive_dtor(ZArchive* za)
{
	if(za->is_open)
	{
		file_close(&za->f);
		lookup_free(&za->li);

		za->is_open = false;
	}
}


static int ZArchive_reload(ZArchive* za, const char* fn, Handle h)
{
	UNUSED(h);

	int err;

	err = file_open(fn, 0, &za->f);
	if(err < 0)
		return err;

	void* file;
	size_t size;
	err = file_map(&za->f, file, size);
	if(err < 0)
		goto exit_close;

	// early out: check if it's even a Zip file.
	// (VFS checks if a file is an archive for mounting by attempting to
	// open it with zip_archive_open)
	err = zip_validate(file, size);
	if(err < 0)
		goto exit_unmap_close;

	err = lookup_init(&za->li, file, size);
	if(err < 0)
		goto exit_unmap_close;

	// we map the file only for convenience when loading;
	// extraction is via aio (faster, better mem use).
	file_unmap(&za->f);

	za->is_open = true;
	return 0;

exit_unmap_close:
	file_unmap(&za->f);
exit_close:
	file_close(&za->f);
	return err;
}


// open and return a handle to the zip archive indicated by <fn>
Handle zip_archive_open(const char* const fn)
{
	return h_alloc(H_ZArchive, fn);
}


// close the archive <ha> and set ha to 0
int zip_archive_close(Handle& ha)
{
	return h_free(ha, H_ZArchive);
}


// would be nice to pass along a key (allowing for O(1) lookup in archive),
// but then the callback is no longer compatible to file / vfs enum files.
int zip_enum(const Handle ha, const ZipFileCB cb, const uintptr_t user)
{
	H_DEREF(ha, ZArchive, za);

	return lookup_enum_files(&za->li, cb, user);
}


///////////////////////////////////////////////////////////////////////////////
//
// in-memory inflate routines (zlib wrapper)
//
///////////////////////////////////////////////////////////////////////////////


uintptr_t inf_init_ctx()
{
#ifdef NO_ZLIB
	return 0;
#else
	// allocate ZLib stream
	const size_t size = round_up(sizeof(z_stream), 32);
		// be nice to allocator
	z_stream* stream = (z_stream*)calloc(size, 1);
	if(inflateInit2(stream, -MAX_WBITS) != Z_OK)
		// -MAX_WBITS indicates no zlib header present
		return 0;

	return (uintptr_t)stream;
#endif
}


int inf_start_read(uintptr_t ctx, void* out, size_t out_size)
{
#ifdef NO_ZLIB
	return -1;
#else
	if(!ctx)
		return ERR_INVALID_PARAM;
	z_stream* stream = (z_stream*)ctx;

	if(stream->next_out || stream->avail_out)
	{
		debug_warn("zip_start_read: ctx already in use!");
		return -1;
	}
	stream->next_out  = (Byte*)out;
	stream->avail_out = (uInt)out_size;
	return 0;
#endif
}


ssize_t inf_inflate(uintptr_t ctx, void* in, size_t in_size)
{
#ifdef NO_ZLIB
	return -1;
#else
	if(!ctx)
		return ERR_INVALID_PARAM;
	z_stream* stream = (z_stream*)ctx;

	size_t prev_avail_out = stream->avail_out;

	stream->avail_in = (uInt)in_size;
	stream->next_in = (Byte*)in;

	int err = inflate(stream, Z_SYNC_FLUSH);

	// check+return how much actual data was read
	size_t avail_out = stream->avail_out;
	assert(avail_out <= prev_avail_out);
		// make sure output buffer size didn't magically increase
	ssize_t nread = (ssize_t)(prev_avail_out - avail_out);
	if(!nread)
		return (err < 0)? err : 0;
		// try to pass along the ZLib error code, but make sure
		// it isn't treated as 'bytes read', i.e. > 0.

	return nread;
#endif
}


int inf_finish_read(uintptr_t ctx)
{
#ifdef NO_ZLIB
	return -1;
#else
	if(!ctx)
		return ERR_INVALID_PARAM;
	z_stream* stream = (z_stream*)ctx;

	if(stream->avail_in || stream->avail_out)
	{
		debug_warn("zip_finish_read: input or output buffer has space remaining");
		stream->avail_in = stream->avail_out = 0;
		return -1;
	}

	stream->next_in  = 0;
	stream->next_out = 0;
	return 0;
#endif
}


int inf_free_ctx(uintptr_t ctx)
{
#ifdef NO_ZLIB
	return -1;
#else
	if(!ctx)
		return ERR_INVALID_PARAM;
	z_stream* stream = (z_stream*)ctx;

	assert(stream->next_out == 0);

	inflateEnd(stream);
	mem_free(stream);
	return 0;
#endif
}


///////////////////////////////////////////////////////////////////////////////
//
// file from Zip archive
// on top of inflate and lookup
//
///////////////////////////////////////////////////////////////////////////////


enum ZFileFlags
{
	// the ZFile has been successfully zip_map-ped.
	// we store this so that the archive mapping refcount remains balanced.
	ZF_HAS_MAPPING = 0x4000
};

// marker for ZFile struct, to make sure it's valid
#ifdef PARANOIA
static const u32 ZFILE_MAGIC = FOURCC('Z','F','I','L');
#endif


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
	else if(!h_user_data(zf->ha, H_ZArchive))
		msg = "invalid archive handle";
#endif
	else if(!zf->ucsize)
		msg = "ucsize = 0";
	else if(!zf->read_ctx)
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


static int zip_open_idx(const Handle ha, const i32 idx, ZFile* zf)
{
	memset(zf, 0, sizeof(ZFile));

	if(!zf)
		goto invalid_zf;
		// jump to CHECK_ZFILE post-check, which will handle this.

{
	H_DEREF(ha, ZArchive, za);
	LookupInfo* li = (LookupInfo*)&za->li;

	const char* fn;
	ZFileLoc loc;
		// don't want ZFile to contain a ZFileLoc struct -
		// its ucsize member must be 'loose' for compatibility with File.
		// => need to copy ZFileLoc fields into ZFile.
	int err = lookup_get_file_info(li, idx, fn, &loc);
	if(err < 0)
		return err;

#ifdef PARANOIA
	zf->magic    = ZFILE_MAGIC;
#endif

	zf->ucsize = loc.ucsize;
	zf->ofs    = loc.ofs;
	zf->csize  = loc.csize;

	zf->ha       = ha;
	zf->read_ctx = inf_init_ctx();
}

invalid_zf:
	CHECK_ZFILE(zf);

	return 0;
}


int zip_open(const Handle ha, const char* fn, ZFile* zf)
{
	H_DEREF(ha, ZArchive, za);
	LookupInfo* li = (LookupInfo*)&za->li;

	i32 idx;
	int err = lookup_file(li, fn, idx);
	if(err < 0)
		return err;

	return zip_open_idx(ha, idx, zf);
}


int zip_close(ZFile* zf)
{
	CHECK_ZFILE(zf);

	// remaining ZFile fields don't need to be freed/cleared
	return inf_free_ctx(zf->read_ctx);
}


// return file information for <fn> in archive <ha>
int zip_stat(Handle ha, const char* fn, struct stat* s)
{
	H_DEREF(ha, ZArchive, za);
	LookupInfo* li = &za->li;

	i32 idx;
	int err = lookup_file(li, fn, idx);
	if(err < 0)
		return err;

	const char* fn2;	// unused
	ZFileLoc loc;
	lookup_get_file_info(li, idx, fn2, &loc);
		// can't fail - returned valid index above

	s->st_size = loc.ucsize;
	return 0;
}



// convenience function, allows implementation change in ZFile.
// note that size == ucsize isn't foolproof, and adding a flag to
// ofs or size is ugly and error-prone.
// no error checking - always called from functions that check zf.
static inline bool is_compressed(ZFile* zf)
{
	return zf->csize != 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// synchronous I/O
//
///////////////////////////////////////////////////////////////////////////////


// note: we go to a bit of trouble to make sure the buffer we allocated
// (if p == 0) is freed when the read fails.
ssize_t zip_read(ZFile* zf, off_t raw_ofs, size_t size, void** p)
{
	CHECK_ZFILE(zf);

	ssize_t err = -1;
	ssize_t raw_bytes_read;

	ZArchive* za = H_USER_DATA(zf->ha, ZArchive);
	if(!za)
		return ERR_INVALID_HANDLE;

	const off_t ofs = zf->ofs + raw_ofs;

	// not compressed - just pass it on to file_io
	// (avoid the Zip inflate start/finish stuff below)
	if(!is_compressed(zf))
		return file_io(&za->f, ofs, size, p);
			// no need to set last_raw_ofs - only checked if compressed.

	// compressed

	// make sure we continue where we left off
	// (compressed data must be read in one stream / sequence)
	//
	// problem: partial reads 
	if(raw_ofs != zf->last_raw_ofs)
	{
		debug_warn("zip_read: compressed read offset is non-continuous");
		return -1;
	}

	void* our_buf = 0;		// buffer we allocated (if necessary)
	if(!*p)
	{
		*p = our_buf = mem_alloc(size);
		if(!*p)
			return ERR_NO_MEM;
	}

	err = (ssize_t)inf_start_read(zf->read_ctx, *p, size);
	if(err < 0)
	{
fail:
		// we allocated it, so free it now
		if(our_buf)
		{
			mem_free(our_buf);
			*p = 0;
		}
		return err;
	}

	// read blocks from the archive's file starting at ofs and pass them to
	// zip_inflate, until all compressed data has been read, or it indicates
	// the desired output amount has been reached.
	const size_t raw_size = zf->csize;
	raw_bytes_read = file_io(&za->f, ofs, raw_size, (void**)0, inf_inflate, zf->read_ctx);

	zf->last_raw_ofs = raw_ofs + (off_t)raw_bytes_read;

	err = inf_finish_read(zf->read_ctx);
	if(err < 0)
		goto fail;

	err = raw_bytes_read;

	// failed - make sure buffer is freed
	if(err <= 0)
		goto fail;

	return err;
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
int zip_map(ZFile* const zf, void*& p, size_t& size)
{
	p = 0;
	size = 0;

	CHECK_ZFILE(zf);

	// mapping compressed files doesn't make sense because the
	// compression algorithm is unspecified - disallow it.
	if(is_compressed(zf))
	{
		debug_warn("zip_map: file is compressed");
		return -1;
	}

	H_DEREF(zf->ha, ZArchive, za)
	CHECK_ERR(file_map(&za->f, p, size));

	zf->flags |= ZF_HAS_MAPPING;
	return 0;
}


// remove the mapping of file <zf>; fail if not mapped.
//
// the mapping will be removed (if still open) when its archive is closed.
// however, map/unmap calls should be paired so that the archive mapping
// may be removed when no longer needed.
int zip_unmap(ZFile* const zf)
{
	CHECK_ZFILE(zf);

	// make sure archive mapping refcount remains balanced:
	// don't allow multiple unmaps.
	if(!(zf->flags & ZF_HAS_MAPPING))
		return -1;
	zf->flags &= ~ZF_HAS_MAPPING;

	H_DEREF(zf->ha, ZArchive, za)
	return file_unmap(&za->f);
}


///////////////////////////////////////////////////////////////////////////////
//
// asynchronous I/O
//
///////////////////////////////////////////////////////////////////////////////


// rationale for not supporting aio for compressed files:
// would complicate things considerably (could no longer just
// return the file I/O handle, since we have to decompress in wait_io),
// yet it isn't really useful - aio is used to stream music,
// which is already compressed.


// begin transferring <size> bytes, starting at <ofs>. get result
// with zip_wait_io; when no longer needed, free via zip_discard_io.
Handle zip_start_io(ZFile* const zf, off_t ofs, size_t size, void* buf)
{
	CHECK_ZFILE(zf);
	if(is_compressed(zf))
	{
		debug_warn("Zip aio doesn't currently support compressed files (see rationale above)");
		return -1;
	}

	H_DEREF(zf->ha, ZArchive, za);
	return file_start_io(&za->f, zf->ofs+ofs, size, buf);
}


// wait until the transfer <hio> completes, and return its buffer.
// output parameters are zeroed on error.
inline int zip_wait_io(Handle hio, void*& p, size_t& size)
{
	return file_wait_io(hio, p, size);
}


// finished with transfer <hio> - free its buffer (returned by vfs_wait_io)
inline int zip_discard_io(Handle& hio)
{
	return file_discard_io(hio);
}
