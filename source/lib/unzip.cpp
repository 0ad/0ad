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

#include <zlib.h>

#include "unzip.h"
#include "posix.h"
#include "misc.h"
#include "res.h"
#include "mem.h"
#include "vfs.h"

#ifdef _MSC_VER
#pragma comment(lib, "zlib.lib")
#endif


static const char ecdr_id[] = "PK\5\6";	// End of Central Directory Header identifier
static const char cdfh_id[] = "PK\1\2";	// Central File Header identifier
static const char lfh_id[]  = "PK\3\4";	// Local File Header identifier


// H_ZFILE handle
// location and size of an archived file
// no destructor
struct ZFILE
{
	u32 ofs;
	u32 csize;			// bit 31 = compression method (1: deflate; 0: stored)
	u32 ucsize;
};


// H_ZARCHIVE handle
// information about a ZIP archive
struct ZARCHIVE
{
	Handle hf;			// actual ZIP file (H_VFILE)

	// file lookup
	u32 num_files;
	u32* fn_hashs;		// split for more efficient search
	ZFILE* files;
	u32 last_file;		// index of last file we found (speed up lookups of sequential files)
};


static void zarchive_dtor(void* p)
{
	ZARCHIVE* za = (ZARCHIVE*)p;

	vfs_close(za->hf);
	za->hf = 0;
}


// open and return a handle to the zip archive indicated by <fn>
Handle zopen(const char* const fn)
{
	const u32 fn_hash = fnv_hash(fn, strlen(fn));

	// already loaded?
	HDATA* hd;
	Handle h = h_find(fn_hash, H_ZFILE, &hd);
	if(h)
		return h;

	// map the ZIP file
	Handle hf = vfs_map(fn);
	hd = h_data(hf, 0);
	if(!hd)
		return 0;
	const size_t size = hd->size;
	const u8* zfile = (const u8*)hd->p;

	// find end of central dir record
	// by scanning last 66000 bytes of file for ecdr_id magic
	// (zip comment <= 65535 bytes, sizeof(ECDR) = 22, add some for safety)
	// if the zip file is < 66000 bytes, scan the whole file

	size_t bytes_left = min(size, 66000);
	const u8* ecdr = zfile + size - bytes_left;

	while(bytes_left-3 > 0)
	{
		if(*(u32*)ecdr == *(u32*)&ecdr_id)
			goto found_ecdr;

		// check next 4 bytes (non aligned!!)
		ecdr++;
		bytes_left--;
	}

	// reached EOF and still haven't found the ECDR identifier

fail:
	vfs_close(hf);
	return 0;

	{
found_ecdr:
	// read ECDR
	const u16 num_files = read_le16(ecdr+10);
	const u32 cd_ofs    = read_le32(ecdr+16);

	// memory for fn_hash and File arrays
	void* mem = mem_alloc(num_files * (4 + sizeof(ZFILE)), MEM_HEAP, 4*KB);
	if(!mem)
		goto fail;

	h = h_alloc(fn_hash, H_ZFILE, zarchive_dtor, &hd);
	if(!h)
		goto fail;

	ZARCHIVE* const za = (ZARCHIVE*)hd->user;
	za->hf = hf;
	za->fn_hashs = (u32*)mem;
	za->files = (ZFILE*)((u8*)mem + 4*num_files);
	za->last_file = 0;

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

	return h;
}


void zclose(const Handle h)
{
	if(h_data(h, H_ZFILE))
		h_free(h, H_ZFILE);
	else
		h_free(h, H_ZARCHIVE);
}


Handle zopen(Handle hz, const char* fn)
{
	HDATA* hzd = h_data(hz, H_ZFILE);
	if(!hzd)
		return 0;
	ZARCHIVE* za = (ZARCHIVE*)hzd->user;

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
	const ZFILE* const f = &za->files[i];

	//
	HDATA* hfd;
	Handle hf = h_alloc(fn_hash, H_ZFILE, 0, &hfd);
	return hf;
}


int zread(Handle hf, void*& p, size_t& size, size_t ofs)
{
	HDATA* hfd = h_data(hf, 0);
	if(!hfd)
		return -1;

	ZFILE* zf = (ZFILE*)hfd->user;
	const size_t ucsize = zf->ucsize;
	const size_t csize = zf->csize;

	if(ofs > ucsize)
		return -1;

bool deflated = ucsize != csize;

	// TODO: async read for uncompressed files?
	if(!deflated)
	{
		p = (u8*)hfd->p + zf->ofs + ofs;
		size = hfd->size - ofs;
		return 0;
	}

	// double-buffered
	u32 slots[2];
	int active_read = 0;
	void* out_pos = 0;

	void* out_mem = mem_alloc(ucsize, MEM_HEAP, 64*KB);
	if(!out_mem)
		return -1;

	// inflating
	z_stream stream;
	if(deflated)
	{
		memset(&stream, 0, sizeof(stream));
		if(inflateInit2(&stream, -MAX_WBITS) != Z_OK)
			return -1;
			// -MAX_WBITS indicates no zlib header present

		stream.next_out = (u8*)out_mem;
		stream.avail_out = ucsize;
	}
	// read directly into output buffer
	else
		out_pos = out_mem;

	bool first = true;
	bool done = false;

	for(;;)
	{
		// start reading next block
		if(!done)
			slots[active_read] = vfs_start_read(hf, ofs, &out_pos);

		active_read ^= 1;

		// process block read in previous iteration
		if(!first)
		{
			void* p;
			size_t bytes_read;
			vfs_finish_read(slots[active_read], p, bytes_read);

			// inflate what we read
			if(deflated)
			{
				stream.next_in = (u8*)p;
				stream.avail_in = bytes_read;
				inflate(&stream, 0);
			}
		}

		first = false;
		// one more iteration to process the last pending block
		if(done)
			break;
		if(ofs >= csize)
			done = true;
	}

	if(deflated)
	{
		inflateEnd(&stream);

		if(stream.total_in != csize || stream.total_out != ucsize)
		{
			mem_free(out_mem);
			return 0;
		}
	}

	p = out_mem;
	size = ucsize;

	return 0;
}


//1 cache per zip - res layer doesnt know our internals (cache entry=ofs, csize,ucsize)
//cache=array (allow finding next file quickly; need some mechanism for faster random access?)
//sync is not a problem - zip file won't be updated during gameplay because it's still open
