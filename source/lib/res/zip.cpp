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

#include <cassert>
#include <cstring>
#include <cstdlib>

#include "zip.h"
#include "file.h"
#include "lib.h"
#include "misc.h"
#include "h_mgr.h"
#include "mem.h"
#include "vfs.h"

#include <zlib.h>
#ifdef _MSC_VER
#pragma comment(lib, "zlib.lib")
#endif


//
// low-level in-memory inflate routines on top of ZLib
//


uintptr_t zip_init_ctx()
{
	// allocate ZLib stream
	z_stream* stream = (z_stream*)mem_alloc(round_up(sizeof(z_stream), 32), 32, MEM_ZERO);
	if(inflateInit2(stream, -MAX_WBITS) != Z_OK)
		// -MAX_WBITS indicates no zlib header present
		return 0;

	return (uintptr_t)stream;
}


int zip_start_read(uintptr_t ctx, void* out, size_t out_size)
{
	if(!ctx)
		return ERR_INVALID_PARAM;
	z_stream* stream = (z_stream*)ctx;

	if(stream->next_out || stream->avail_out)
	{
		assert(0 && "zip_start_read: ctx already in use!");
		return -1;
	}
	stream->next_out  = (Bytef*)out;
	stream->avail_out = (uInt)out_size;
	return 0;
}


ssize_t zip_inflate(uintptr_t ctx, void* in, size_t in_size)
{
	if(!ctx)
		return ERR_INVALID_PARAM;
	z_stream* stream = (z_stream*)ctx;

	size_t prev_avail_out = stream->avail_out;

	int err = inflate(stream, Z_SYNC_FLUSH);

	// check+return how much actual data was read
	size_t avail_out = stream->avail_out;
	assert(avail_out <= prev_avail_out);
		// make sure output buffer size didn't magically increase
	size_t nread = prev_avail_out - avail_out;
	if(!nread)
		return (err < 0)? err : 0;
		// try to pass along the ZLib error code, but make sure
		// it isn't treated as 'bytes read', i.e. > 0.

	return nread;
}


int zip_finish_read(uintptr_t ctx)
{
	if(!ctx)
		return ERR_INVALID_PARAM;
	z_stream* stream = (z_stream*)ctx;

	if(stream->avail_in || stream->avail_out)
	{
		assert("zip_finish_read: input or input buffer has space remaining");
		stream->avail_in = stream->avail_out = 0;
		return -1;
	}

	stream->next_in  = 0;
	stream->next_out = 0;

	return 0;
}


int zip_free_ctx(uintptr_t ctx)
{
	if(!ctx)
		return ERR_INVALID_PARAM;
	z_stream* stream = (z_stream*)ctx;

	assert(stream->next_out == 0);

	inflateEnd(stream);
	mem_free(stream);
	return 0;
}


//
// Zip archive
//


static const char ecdr_id[] = "PK\5\6";	// End of Central Directory Header identifier
static const char cdfh_id[] = "PK\1\2";	// Central File Header identifier
static const char lfh_id[]  = "PK\3\4";	// Local File Header identifier

struct ZEnt
{
	size_t ofs;
	size_t csize;		// 0 if not compressed
	size_t ucsize;

	// why csize?
	// file I/O may be N-buffered, so it's good to know when the raw data
	// stops (or else we potentially overshoot by N-1 blocks),
	// but not critical, since Zip files are stored individually.
	//
	// we also need a way to check if file compressed (e.g. to fail mmap
	// requests if the file is compressed). packing a bit in ofs or
	// ucsize is error prone and ugly (1 bit less won't hurt though).
	// any other way will mess up the nice 8 byte size anyway, so might
	// as well store csize.
	//
	// don't worry too much about non-power-of-two size, will probably
	// change to global FS tree instead of linear lookup later anyway.
};

struct ZArchive
{
	File f;

	// file lookup
	u16 num_files;
	u16 last_file;		// index of last file we found (speed up lookups of sequential files)
	u32* fn_hashs;		// split for more efficient search
	ZEnt* ents;
};

H_TYPE_DEFINE(ZArchive)


static void ZArchive_init(ZArchive* za, va_list args)
{
}


static void ZArchive_dtor(ZArchive* za)
{
	file_close(&za->f);
	mem_free(za->fn_hashs);	// both fn_hashs[] and files[]
}


static int ZArchive_reload(ZArchive* za, const char* fn)
{
	const u8* ecdr;	// declare here to avoid goto scope problems

	int err = file_open(fn, 0, &za->f);
	if(err < 0)
		return err;

	void* p;
	size_t size;
	err = file_map(&za->f, p, size);
	if(err < 0)
		return err;

{
	// find end of central dir record
	// by scanning last 66000 bytes of file for ecdr_id magic
	// (zip comment <= 65535 bytes, sizeof(ECDR) = 22, add some for safety)
	// if the zip file is < 66000 bytes, scan the whole file

	size_t bytes_left = 66000;	// min(66k, size) - avoid stupid warning
	if(bytes_left > size)
		bytes_left = size;

	ecdr = (const u8*)p + size - 22;
	if(*(u32*)ecdr == *(u32*)&ecdr_id)
		goto found_ecdr;

	ecdr = (const u8*)p + size - bytes_left;

	while(bytes_left-3 > 0)
	{
		if(*(u32*)ecdr == *(u32*)&ecdr_id)
			goto found_ecdr;

		// check next 4 bytes (non aligned!!)
		ecdr++;
		bytes_left--;
	}

	// reached EOF and still haven't found the ECDR identifier
}

fail:
	file_unmap(&za->f);
	file_close(&za->f);
	return -1;

found_ecdr:
{
	// read ECDR
	const u16 num_files = read_le16(ecdr+10);
	const u32 cd_ofs    = read_le32(ecdr+16);

	// memory for fn_hash and Ent arrays
	void* file_list_mem = mem_alloc(num_files * (sizeof(u32) + sizeof(ZEnt)), 4*KB);
	if(!file_list_mem)
		goto fail;
	u32* fn_hashs = (u32*)file_list_mem;
	ZEnt* ents = (ZEnt*)((u8*)file_list_mem + num_files*sizeof(u32));

	// cache file list for faster lookups
	// currently linear search, comparing filename hash.
	// TODO: if too slow, use hash table.
	const u8* cdfh = (const u8*)p+cd_ofs;
	u32* hs = fn_hashs;
	ZEnt* ent = ents;
	u16 i;
	for(i = 0; i < num_files; i++)
	{
		// read CDFH
		if(*(u32*)cdfh != *(u32*)cdfh_id)
			continue;
		const u32 csize   = read_le32(cdfh+20);
		const u32 ucsize  = read_le32(cdfh+24);
		const u16 fn_len  = read_le16(cdfh+28);
		const u16 e_len   = read_le16(cdfh+30);
		const u16 c_len   = read_le16(cdfh+32);
		const u32 lfh_ofs = read_le32(cdfh+42);
		const u8 method   = cdfh[10];

		if(method & ~8)	// neither deflated nor stored
			continue;

		// read LFH
		const u8* const lfh = (const u8*)p + lfh_ofs;
		if(*(u32*)lfh != *(u32*)lfh_id)
			continue;
		const u16 lfh_fn_len = read_le16(lfh+26);
		const u16 lfh_e_len  = read_le16(lfh+28);
		const char* lfh_fn   = (const char*)lfh+30;

		*hs++ = fnv_hash(lfh_fn, lfh_fn_len);
		ent->ofs = lfh_ofs + 30 + lfh_fn_len + lfh_e_len;
		ent->csize = csize;
		ent->ucsize = ucsize;
		ent++;

		(uintptr_t&)cdfh += 46 + fn_len + e_len + c_len;
	}

	za->num_files = i;
	za->last_file = 0;
	za->fn_hashs = fn_hashs;
	za->ents = ents;
}	// scope

	return 0;
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


//
// file from Zip archive
//


static int lookup(Handle ha, const char* fn, const ZEnt*& ent)
{
	H_DEREF(ha, ZArchive, za);

	// find its File descriptor
	const u32 fn_hash = fnv_hash(fn, strlen(fn));
	u16 i = za->last_file+1;
	if(i >= za->num_files || za->fn_hashs[i] != fn_hash)
	{
		for(i = 0; i < za->num_files; i++)
			if(za->fn_hashs[i] == fn_hash)
				break;
		if(i == za->num_files)
			return ERR_FILE_NOT_FOUND;

		za->last_file = i;
	}

	ent = &za->ents[i];
	return 0;
}


// marker for ZFile struct, to make sure it's valid
static const u32 ZFILE_MAGIC = FOURCC('Z','F','I','L');


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
	else if(zf->magic != FILE_MAGIC)
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
	assert(0 && "zfile_validate failed");
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



int zip_open(const Handle ha, const char* fn, ZFile* zf)
{
	memset(zf, 0, sizeof(ZFile));

	if(!zf)
		goto invalid_zf;
	// jump to CHECK_ZFILE post-check, which will handle this.

{
	const ZEnt* ze;
	int err = lookup(ha, fn, ze);
	if(err < 0)
		return err;

#ifdef PARANOIA
	zf->magic = ZFILE_MAGIC;
#endif

	zf->ofs      = ze->ofs;
	zf->csize    = ze->csize;
	zf->ucsize   = ze->ucsize;
	zf->ha       = ha;
	zf->read_ctx = zip_init_ctx();
}

invalid_zf:
	CHECK_ZFILE(zf)

	return 0;
}


int zip_close(ZFile* zf)
{
	CHECK_ZFILE(zf)

	// remaining fields don't need to be freed/cleared
	return zip_free_ctx(zf->read_ctx);
}


// return file information for <fn> in archive <ha>
int zip_stat(Handle ha, const char* fn, struct stat* s)
{
	const ZEnt* ze;
	int err = lookup(ha, fn, ze);
	if(err < 0)
		return err;

	s->st_size = (off_t)ze->ucsize;
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


// note: we go to a bit of trouble to make sure the buffer we allocated
// (if p == 0) is freed when the read fails.
ssize_t zip_read(ZFile* zf, size_t raw_ofs, size_t size, void*& p)
{
	CHECK_ZFILE(zf)

	ssize_t err = -1;
	ssize_t raw_bytes_read;
		
	ZArchive* za = H_USER_DATA(zf->ha, ZArchive);
	if(!za)
		return ERR_INVALID_HANDLE;

	void* our_buf = 0;		// buffer we allocated (if necessary)
	if(!p)
	{
		p = our_buf = mem_alloc(size);
		if(!p)
			return ERR_NO_MEM;
	}

	const size_t ofs = zf->ofs + raw_ofs;

	// not compressed - just pass it on to file_io
	// (avoid the Zip inflate start/finish stuff below)
	if(!is_compressed(zf))
		return file_io(&za->f, ofs, size, &p);
			// no need to set last_raw_ofs - only checked if compressed.

	// compressed

	// make sure we continue where we left off
	// (compressed data must be read in one stream / sequence)
	//
	// problem: partial reads 
	if(raw_ofs != zf->last_raw_ofs)
	{
		assert(0 && "zip_read: compressed read offset is non-continuous");
		goto fail;
	}

	err = (ssize_t)zip_start_read(zf->read_ctx, p, size);
	if(err < 0)
	{
fail:
		// we allocated it, so free it now
		if(our_buf)
		{
			mem_free(our_buf);
			p = 0;
		}
		return err;
	}

	// read blocks from the archive's file starting at ofs and pass them to
	// zip_inflate, until all compressed data has been read, or it indicates
	// the desired output amount has been reached.
	const size_t raw_size = zf->csize;
	raw_bytes_read = file_io(&za->f, ofs, raw_size, (void**)0, zip_inflate, zf->read_ctx);

	err = zip_finish_read(zf->read_ctx);
	if(err < 0)
		goto fail;

	err = raw_bytes_read;

	// failed - make sure buffer is freed
	if(err <= 0)
		goto fail;

	return err;
}


int zip_map(ZFile* zf, void*& p, size_t& size)
{
	CHECK_ZFILE(zf)

	// doesn't really make sense to map compressed files, so disallow it.
	if(is_compressed(zf))
	{
		assert(0 && "mapping a compressed file from archive. why?");
		return -1;
	}

	H_DEREF(zf->ha, ZArchive, za)
	return file_map(&za->f, p, size);
}
