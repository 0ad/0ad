// ZIP archiving (on top of ZLib)
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

#include <cassert>
#include <cstring>
#include <cstdlib>

#include "zip.h"
#include "posix.h"
#include "misc.h"
#include "res.h"
#include "mem.h"
#include "vfs.h"

#ifdef _MSC_VER
#pragma comment(lib, "zlib.lib")
#endif


// H_ZARCHIVE
// information about a Zip archive
struct ZARCHIVE
{
	Handle hm;			// archive in memory (usually mapped)

	const char* fn;		// archive filename

	// file lookup
	u16 num_files;
	u16 last_file;		// index of last file we found (speed up lookups of sequential files)
	u32* fn_hashs;		// split for more efficient search
	ZFILE* files;
};

#include <zlib.h>

static const char ecdr_id[] = "PK\5\6";	// End of Central Directory Header identifier
static const char cdfh_id[] = "PK\1\2";	// Central File Header identifier
static const char lfh_id[]  = "PK\3\4";	// Local File Header identifier




static void zarchive_dtor(void* p)
{
	ZARCHIVE* za = (ZARCHIVE*)p;

	mem_free(za->hm);
	mem_free((void*)za->fn);
	mem_free(za->fn_hashs);	// both fn_hashs[] and files[]
}


// open and return a handle to the zip archive indicated by <fn>
Handle zip_open(const char* const fn)
{
	const u8* ecdr;	// declared here to avoid goto scope problems

	const u32 fn_hash = fnv_hash(fn, strlen(fn));

	// allocate a handle
	ZARCHIVE* za;
	Handle ha = h_alloc(fn_hash, H_ZARCHIVE, zarchive_dtor, (void**)&za);
	// .. failed
	if(!ha)
		return 0;
	// .. archive already loaded
	if(za->hm)
		return ha;

	// map the Zip file
	const u8* zfile;
	size_t size;
	Handle hm = vfs_load(fn, (void*&)zfile, size);
	if(!hm)
		goto fail;

{
	// find end of central dir record
	// by scanning last 66000 bytes of file for ecdr_id magic
	// (zip comment <= 65535 bytes, sizeof(ECDR) = 22, add some for safety)
	// if the zip file is < 66000 bytes, scan the whole file

	size_t bytes_left = min(size, 66000);
	ecdr = zfile + size - bytes_left;

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
	h_free(ha, H_ZARCHIVE);
	mem_free(hm);
	return 0;

found_ecdr:
{
	// read ECDR
	const u16 num_files = read_le16(ecdr+10);
	const u32 cd_ofs    = read_le32(ecdr+16);

	// memory for fn_hash and File arrays
	void* mem = mem_alloc(num_files * (sizeof(u32) + sizeof(ZFILE)), 4*KB, MEM_HEAP);
	if(!mem)
		goto fail;

	za->hm = hm;
	za->fn_hashs = (u32*)mem;
	za->files = (ZFILE*)((u8*)mem + sizeof(u32)*num_files);
	za->last_file = 0;

	za->fn = (const char*)mem_alloc(strlen(fn)+1);
	strcpy((char*)za->fn, fn);

	// cache file list for faster lookups
	// currently linear search, comparing filename hash.
	// TODO: if too slow, use hash table.
	const u8* cdfh = zfile+cd_ofs;
	u32* hs = za->fn_hashs;
	ZFILE* f = za->files;
	uint i;
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
		const u8* const lfh = zfile + lfh_ofs;
		if(*(u32*)lfh != *(u32*)lfh_id)
			continue;
		const u16 lfh_fn_len = read_le16(lfh+26);
		const u16 lfh_e_len  = read_le16(lfh+28);
		const char* lfh_fn   = (const char*)lfh+30;

		*hs++ = fnv_hash(lfh_fn, lfh_fn_len);
		f->ofs = lfh_ofs + 30 + lfh_fn_len + lfh_e_len;
		f->csize = csize;
		f->ucsize = ucsize;
		f++;

		(int&)cdfh += 46 + fn_len + e_len + c_len;
	}

	za->num_files = i;
}

	return ha;
}


int zip_close(Handle ha)
{
	return h_free(ha, H_ZARCHIVE);
}


ZFILE* zip_lookup(Handle ha, const char* fn)
{
	ZARCHIVE* za = (ZARCHIVE*)h_user_data(ha, H_ZARCHIVE);
	if(!za)
		return 0;

	// find its File descriptor
	const u32 fn_hash = fnv_hash(fn, strlen(fn));
	uint i = za->last_file+1;
	if(i >= za->num_files || za->fn_hashs[i] != fn_hash)
	{
		for(i = 0; i < za->num_files; i++)
			if(za->fn_hashs[i] == fn_hash)
				break;
		if(i == za->num_files)
			return 0;

		za->last_file = i;
	}

	return &za->files[i];
}


int zip_stat(Handle ha, const char* fn, struct stat* s)
{
	ZFILE* zf = zip_lookup(ha, fn);
	if(!zf)
		return -1;

	s->st_size = zf->ucsize;
	return 0;
}


int zip_archive_info(Handle ha, char* fn, Handle* hm)
{
	ZARCHIVE* za = (ZARCHIVE*)h_user_data(ha, H_ZARCHIVE);
	if(!za)
		return -1;

	if(fn)
		strcpy(fn, za->fn);

	if(hm)
		*hm = za->hm;
	return 0;
}


void* zip_inflate_start(void* out, size_t out_size)
{
	z_stream* stream = (z_stream*)mem_alloc(sizeof(z_stream), 4, MEM_POOL);
	memset(stream, 0, sizeof(stream));
	
	if(inflateInit2(stream, -MAX_WBITS) != Z_OK)
		return 0;
		// -MAX_WBITS indicates no zlib header present

	stream->next_out = (Bytef*)out;
	stream->avail_out = out_size;
	
	return stream;
}


int zip_inflate_process(void* ctx, void* p, size_t size)
{
	z_stream* stream = (z_stream*)ctx;

	stream->next_in = (Bytef*)p;
	stream->avail_in = size;
	return inflate(stream, 0);
}


int zip_inflate_end(void* ctx)
{
	z_stream* stream = (z_stream*)ctx;
	inflateEnd(stream);

	// didn't read everything
	if(stream->avail_out || stream->avail_in)
		return -1;
	return 0;
}

