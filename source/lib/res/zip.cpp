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

#include <assert.h>

// provision for removing all ZLib code (all inflate calls will fail).
// used for checking DLL dependency; might also simulate corrupt Zip files.
//#define NO_ZLIB

#ifndef NO_ZLIB
# define ZLIB_WINAPI
# include <zlib.h>

# ifdef _MSC_VER
#  ifdef NDEBUG
#   pragma comment(lib, "zlib1.lib")
#  else
#   pragma comment(lib, "zlib1d.lib")
#  endif
# endif
#endif

#include <map>


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

	// why csize?
	// file I/O may be N-buffered, so it's good to know when the raw data
	// stops (or else we potentially overshoot by N-1 blocks),
	// but not critical, since Zip files are compressed individually.
	// (if we read too much, it's ignored by inflate).
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
const size_t CDFH_SIZE = 46;
const size_t LFH_SIZE  = 30;
const size_t ECDR_SIZE = 22;


static inline int z_validate(const void* const file, const size_t size)
{
	// make sure it's big enough to check the header and for
	// z_find_ecdr to succeed (if smaller, it's obviously bogus).
	if(size < 22)
		return -1;

	// check "header" (first LFH) signature
	return (*(u32*)file == *(u32*)&lfh_id)? 0 : -1;
}


// find end of central dir record in file (loaded or mapped).
// z_validate ensures size >= 22.
static int z_find_ecdr(const void* const file, const size_t size, const u8*& ecdr_)
{
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
static int z_verify_lfh(const void* const file, const off_t lfh_ofs, const off_t file_ofs)
{
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
static int z_read_cdfh(const u8*& cdfh, const char*& fn, size_t& fn_len, ZLoc* const loc)
{
	if(*(u32*)cdfh != *(u32*)cdfh_id)
	{
		debug_warn("CDFH corrupt! (signature doesn't match)");
		goto skip_file;
	}

	{

	const u8  method  = cdfh[10];
	      u32 csize_  = read_le32(cdfh+20);
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
	// tell zfile_compressed that the file is uncompressed,
	// by setting csize_ to 0.
	if(method == 0)
		csize_ = 0;

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
	if(!z_verify_lfh(file, lfh_ofs, file_ofs))
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
typedef int(*CDFH_CB)(const uintptr_t user, const i32 idx, const char* const fn, const size_t fn_len, const ZLoc* const loc);

// go through central directory of the Zip file (loaded or mapped into memory);
// call back for each file.
//
// HACK: call back with negative index the first time; its abs. value is
// the number of files in the archive. lookup needs to know this so it can
// allocate memory. having lookup_init call zip_get_num_files and then
// zip_enum_files would require passing around a ZipInfo struct,
// or searching for the ECDR twice - both ways aren't nice.
static int z_enum_files(const void* const file, const size_t size, const CDFH_CB cb, const uintptr_t user)
{
	// find End of Central Directory Record
	const u8* ecdr;
	CHECK_ERR(z_find_ecdr(file, size, ecdr));

	// call back with number of files in archive
	const i32 num_files = read_le16(ecdr+10);
	// .. callback expects -num_files < 0.
	//    if it's 0, the callback would treat it as an index => crash.
	if(!num_files)
		return -1;
	CHECK_ERR(cb(user, -num_files, 0, 0, 0));

	// call back for each (valid) CDFH entry
	const u32 cd_ofs = read_le32(ecdr+16);
	const u8* cdfh = (const u8*)file + cd_ofs;
		// pointer is advanced in zip_read_cdfh
	for(i32 idx = 0; idx < num_files; idx++)
	{
		const char* fn;
		size_t fn_len;
		ZLoc loc;
		int err = z_read_cdfh(cdfh, fn, fn_len, &loc);
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
//   is currently fast enough, and will be O(1) if the files are arranged
//   in order of access (which would also reduce seeks).
//   this could easily be added though, if need be; Zip files include a CDFH
//   entry for each dir.


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

	i32 num_files;

	i32 next_file;
		// for last-file-opened optimization.
		// we store index of next file instead of the last one opened
		// to avoid trouble on first call (don't want last == -1).

	// don't know size of std::map, and this struct is
	// included in a control block (ZArchive).
	// allocate dynamically to be safe.
	LookupIdx* idx;
};


// support for case-insensitive filenames: the FNV hash of each
// filename string is saved in lookup_add_file_cb and searched for by
// lookup_get_file_info. in both cases, we convert a temporary to
// lowercase before hashing it.
static void strcpy_lower(char* dst, const char* src)
{
	int c;
	do
	{
		c = *src++;
		*dst++ = tolower(c);
	}
	while(c != '\0');
}


// add file <fn> to the lookup data structure.
// called from z_enum_files in order (0 <= idx < num_files).
// the first call notifies us of # files, so we can allocate memory.
//
// notes:
// - fn (filename) is not necessarily 0-terminated!
// - loc is only valid during the callback! must be copied or saved.
static int lookup_add_file_cb(const uintptr_t user, const i32 idx, const char* const fn, const size_t fn_len, const ZLoc* const loc)
{
	LookupInfo* li = (LookupInfo*)user;

	// HACK: on first call, idx is negative and tells us how many
	// files are in the archive (so we can allocate memory).
	// see z_enum_files for why it's done this way.
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

	char lc_fn[PATH_MAX];
	strcpy_lower(lc_fn, fn);
	FnHash fn_hash = fnv_hash(lc_fn, fn_len);

	(*li->idx)[fn_hash] = idx;
	li->fn_hashes[idx] = fn_hash;

	// valid file - write out its info.
	if(loc)
	{
		// copy filename, so we can 0-terminate it
		ent->fn = (const char*)malloc(fn_len+1);
		if(!ent->fn)
			return ERR_NO_MEM;
		strncpy((char*)ent->fn, fn, fn_len);

		// 0-terminate and strip trailing '/'
		char* end = (char*)ent->fn + fn_len-1;
		if(*end != '/')
			end++;
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
	int err;

	// check if it's even a Zip file.
	// the VFS blindly opens files when mounting; it needs to open
	// all archives, but doesn't know their extension (e.g. ".pk3").
	err = z_validate(file, size);
	if(err < 0)		// don't CHECK_ERR - this can happen often.
		return err;

	// all other fields are initialized in lookup_add_file_cb
	li->next_file = 0;

	li->idx = new LookupIdx;

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
static int lookup_free(LookupInfo* const li)
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


// return file information of file <fn>.
static int lookup_get_file_info(LookupInfo* const li, const char* fn, ZLoc* const loc)
{
	char lc_fn[PATH_MAX];
	strcpy_lower(lc_fn, fn);
	const FnHash fn_hash = fnv_hash(lc_fn);

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
		assert(0 <= i && i < li->num_files);
	}
	
	li->next_file = i+1;

	const ZEnt* const ent = &li->ents[i];
	fn   = ent->fn;
	*loc = ent->loc;
	return 0;
}


static int lookup_enum_files(LookupInfo* const li, FileCB cb, uintptr_t user)
{
	const ZEnt* ent = li->ents;
	for(i32 i = 0; i < li->num_files; i++, ent++)
	{
		ssize_t size = (ssize_t)ent->loc.ucsize;
		if(size == 0)	// it's a directory
			size = -1;

		CHECK_ERR(cb(ent->fn, size, user));
			// pass in complete path (see file_enum rationale).
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
	//   invalid (wasn't open). this happens if e.g. vfs_mount blindly
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

	// don't complain here either; this happens when vfs_mount
	// zip_archive_opens an invalid file that's in a mount point dir.
	return err;
}


// open and return a handle to the zip archive indicated by <fn>
inline Handle zip_archive_open(const char* const fn)
{
	return h_alloc(H_ZArchive, fn);
}


// close the archive <ha> and set ha to 0
inline int zip_archive_close(Handle& ha)
{
	return h_free(ha, H_ZArchive);
}


// call <cb>, passing <user>, for all files in archive <ha>
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


// we will later provide data that is to be unzipped into <out>.
int inf_set_dest(uintptr_t ctx, void* out, size_t out_size)
{
#ifdef NO_ZLIB
	return -1;
#else
	if(!ctx)
		return ERR_INVALID_PARAM;
	z_stream* stream = (z_stream*)ctx;

	if(stream->next_out || stream->avail_out)
	{
		debug_warn("zip_set_dest: ctx already in use!");
		return -1;
	}
	stream->next_out  = (Byte*)out;
	stream->avail_out = (uInt)out_size;
	return 0;
#endif
}


// unzip into output buffer. returns bytes written
// (may be 0, if not enough data is passed in), or < 0 on error.
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
	//
	// note: zlib may not always output data, e.g. if passed very little
	// data in one block (due to misalignment). return 0 ("no data output"),
	// which doesn't abort the read.
	size_t avail_out = stream->avail_out;
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


// unzip complete; all input and output data should have been consumed.
// do not release the ctx yet: the user may be reading a file in chunks,
// calling inf_finish after each.
int inf_finish(uintptr_t ctx)
{
#ifdef NO_ZLIB
	return -1;
#else
	if(!ctx)
		return ERR_INVALID_PARAM;
	z_stream* stream = (z_stream*)ctx;

	if(stream->avail_in || stream->avail_out)
	{
		debug_warn("zip_finish: input or output buffer has space remaining");
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
	free(stream);
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




// return file information for <fn> in archive <ha>
int zip_stat(Handle ha, const char* fn, struct stat* s)
{
	// zero output param in case we fail below.
	memset(s, 0, sizeof(struct stat));

	H_DEREF(ha, ZArchive, za);
	LookupInfo* li = &za->li;

	ZLoc loc;
	CHECK_ERR(lookup_get_file_info(li, fn, &loc));

	s->st_size = loc.ucsize;
	return 0;
}


int zip_open(const Handle ha, const char* fn, ZFile* zf)
{
	H_DEREF(ha, ZArchive, za);
	LookupInfo* li = (LookupInfo*)&za->li;

	// zero output param in case we fail below.
	memset(zf, 0, sizeof(ZFile));

	if(!zf)
		goto invalid_zf;
		// jump to CHECK_ZFILE post-check, which will handle this.

{
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
	zf->inf_ctx = inf_init_ctx();
}

invalid_zf:
	CHECK_ZFILE(zf);

	return 0;
}


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


struct IOCBParams
{
	uintptr_t inf_ctx;

	FileIOCB user_cb;
	uintptr_t user_ctx;
};


static ssize_t io_cb(uintptr_t ctx, void* buf, size_t size)
{
	IOCBParams* p = (IOCBParams*)ctx;

	ssize_t ret = inf_inflate(p->inf_ctx, buf, size);

	if(p->user_cb)
		return p->user_cb(p->user_ctx, buf, size);

	return ret;
}


// note: we go to a bit of trouble to make sure the buffer we allocated
// (if p == 0) is freed when the read fails.
ssize_t zip_read(ZFile* zf, off_t raw_ofs, size_t size, void** p, FileIOCB cb, uintptr_t ctx)
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
	if(!zfile_compressed(zf))
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

	void* buf;
	bool free_buf = true;

	// user-specified buf
	if(*p)
	{
		buf = *p;
		free_buf = false;
	}
	// we're going to allocate
	else
	{
		buf = mem_alloc(size, 4096);
		if(!buf)
			return ERR_NO_MEM;
		*p = buf;
	}

	err = (ssize_t)inf_set_dest(zf->inf_ctx, buf, size);
	if(err < 0)
	{
fail:
		// we allocated it, so free it now
		if(free_buf)
		{
			mem_free(buf);
			*p = 0;
		}
		return err;
	}

	const IOCBParams params = { zf->inf_ctx, cb, ctx };

	// read blocks from the archive's file starting at ofs and pass them to
	// inf_inflate, until all compressed data has been read, or it indicates
	// the desired output amount has been reached.
	const size_t raw_size = zf->csize;
	raw_bytes_read = file_io(&za->f, ofs, raw_size, (void**)0, io_cb, (uintptr_t)&params);

	zf->last_raw_ofs = raw_ofs + (off_t)raw_bytes_read;

	err = inf_finish(zf->inf_ctx);
	if(err < 0)
		goto fail;

	err = raw_bytes_read;

	// failed - make sure buffer is freed
	if(err <= 0)
		goto fail;

	return err;
}


///////////////////////////////////////////////////////////////////////////////


// rationale for not supporting aio for compressed files:
// would complicate things considerably (could no longer just
// return the file I/O handle, since we have to decompress in wait_io),
// yet it isn't really useful - aio is used to stream music,
// which is already compressed.


// begin transferring <size> bytes, starting at <ofs>. get result
// with zip_wait_io; when no longer needed, free via zip_discard_io.
int zip_start_io(ZFile* const zf, off_t ofs, size_t size, void* buf, FileIO* io)
{
	CHECK_ZFILE(zf);
	if(zfile_compressed(zf))
	{
		debug_warn("Zip aio doesn't currently support compressed files (see rationale above)");
		return -1;
	}

	H_DEREF(zf->ha, ZArchive, za);
	return file_start_io(&za->f, zf->ofs+ofs, size, buf, io);
}


// wait until the transfer <hio> completes, and return its buffer.
// output parameters are zeroed on error.
inline int zip_wait_io(FileIO* io, void*& p, size_t& size)
{
	return file_wait_io(io, p, size);
}


// finished with transfer <hio> - free its buffer (returned by vfs_wait_io)
inline int zip_discard_io(FileIO* io)
{
	return file_discard_io(io);
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
int zip_unmap(ZFile* const zf)
{
	CHECK_ZFILE(zf);

	// make sure archive mapping refcount remains balanced:
	// don't allow multiple unmaps.
	if(!(zf->flags & ZF_HAS_MAPPING))
		return -1;
	zf->flags &= ~ZF_HAS_MAPPING;

	H_DEREF(zf->ha, ZArchive, za);
	return file_unmap(&za->f);
}
