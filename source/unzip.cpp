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

#ifdef _MSC_VER
#pragma comment(lib, "zlib.lib")
#endif


static const char ecdr_id[] = "PK\5\6";	// End of Central Directory Header identifier
static const char cdfh_id[] = "PK\1\2";	// Central File Header identifier
static const char lfh_id[]  = "PK\3\4";	// Local File Header identifier


// Location and size of an archived file
struct File
{
	u32 ofs;
	u32 csize;			// bit 31 = compression method (1: deflate; 0: stored)
	u32 ucsize;
};


struct ZIP
{
	int fd;

	// file lookup
	u32 num_files;
	u32* fn_hashs;		// split for more efficient search
	File* files;
	u32 last_file;		// index of last file we found (speed up lookups of sequential files)
};


static void zip_dtor(HDATA* hd)
{
	ZIP* const z = (ZIP*)hd->internal;

	close(z->fd);
	z->fd = -1;
}


// open and return a handle to the zip archive indicated by <fn>
Handle zopen(const char* const fn)
{
	const u32 fn_hash = fnv_hash(fn, strlen(fn));

	// already loaded?
	HDATA* hd;
	Handle h = h_find(fn_hash, RES_ZIP, hd);
	if(h)
		return h;

	// file size
	struct stat stat_buf;
	if(stat(fn, &stat_buf))
		return 0;
	const size_t size = stat_buf.st_size;
	if(size < 4)	// ECDR scan below would overrun
		return 0;

	// map zip file (easy access while getting list of files)
	const int fd = open(fn, O_RDONLY);
	if(fd < 0)
		return 0;
	u8* zfile = (u8*)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if(zfile == MAP_FAILED)
		return 0;

	// find end of central dir record
	// by scanning last 66000 bytes of file for ecdr_id magic
	// (zip comment <= 65535 bytes, sizeof(ECDR) = 22, add some for safety)
	// if the zip file is < 66000 bytes, scan the whole file

	u32 bytes_left = min(size, 66000);
	u8* ecdr = zfile + size - bytes_left;

	while(bytes_left)
	{
		if(*(u32*)ecdr == *(u32*)&ecdr_id)
			break;

		// check next 4 bytes (non aligned!!)
		ecdr++;
		bytes_left--;
	}

	{
	// reached EOF and still haven't found the ECDR identifier
	if(bytes_left == 0)
		goto fail;

	// read ECDR
	u16 num_files = read_le16(ecdr+10);
	u32 cd_ofs    = read_le32(ecdr+16);

	h = h_alloc(fn_hash, RES_ZIP, zip_dtor, hd);
	if(!h || !hd)
		goto fail;

	ZIP* const zip = (ZIP*)hd->internal;
	zip->fd = fd;
	zip->fn_hashs = ( u32*)mem_alloc(num_files * sizeof(u32 ), MEM_HEAP, 64);
	zip->files    = (File*)mem_alloc(num_files * sizeof(File), MEM_HEAP, 64);
	if(!zip->fn_hashs || !zip->files)
	{
		h_free(h, RES_ZIP);
		mem_free(zip->fn_hashs);
		mem_free(zip->files);
		goto fail;
	}
	zip->num_files = num_files;
	zip->last_file = 0;

	// cache file list for faster lookups
	// currently linear search, comparing filename hash.
	// if too slow, use hash table
	const u8* cdfh = zfile+cd_ofs;
	u32* hs = zip->fn_hashs;
	File* f = zip->files;
	for(uint i = 0; i < zip->num_files; i++)
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

		// read LFH
		const u8* const lfh = zfile + lfh_ofs;
		if(*(u32*)lfh != *(u32*)lfh_id)
			continue;
		const u16 lfh_fn_len = read_le16(lfh+26);
		const u16 lfh_e_len  = read_le16(lfh+28);
		const char* lfh_fn   = (const char*)lfh+30;

		*hs++ = fnv_hash(lfh_fn, lfh_fn_len);
		f->ofs = lfh_ofs + 30 + lfh_fn_len + lfh_e_len;
		f->csize = csize | ((method == 8)? BIT(31) : 0);
		f->ucsize = ucsize;
		f++;

		(int&)cdfh += 46 + fn_len + e_len + c_len;
	}
	}

// unmap file and return 0
fail:

	munmap(zfile, size);
		// actual reading is done with aio

	return h;
}


// close the zip file zd
void zclose(const Handle h)
{
	h_free(h, RES_ZIP);
}





// make file <fn> in zip <zd> available
// returns a pointer to the data, and optionally its size (0 on error)
//
// returns 0 on error.
int zread(Handle h, const char* fn, void*& p, size_t& size, size_t ofs)
{
	// size = 0 if we fail
	size = 0;

	HDATA* hd = h_data(h, RES_ZIP);
	if(!hd)
		return 0;
	ZIP* zip = (ZIP*)hd->internal;

	// find its File descriptor
	u32 fn_hash = fnv_hash(fn, strlen(fn));
	uint i = zip->last_file+1;
	if(i >= zip->num_files || zip->fn_hashs[i] != fn_hash)
	{
		for(i = 0; i < zip->num_files; i++)
			if(zip->fn_hashs[i] == fn_hash)
				break;
		if(i == zip->num_files)
			return 0;

		zip->last_file = i;
	}
	const File* const f = &zip->files[i];

	const bool deflated = (f->csize & BIT(31)) != 0;
	ofs += f->ofs;
	const u32 csize = f->csize & ~BIT(31);
	const u32 ucsize = f->ucsize;

	aiocb cbs[2];
	memset(cbs, 0, sizeof(cbs));
	cbs[0].aio_fildes = cbs[1].aio_fildes = zip->fd;

	// decompress only:
	void* read_bufs = 0;
	const int BLOCK_SIZE = 64*KB;
	int active_read = 0;
	z_stream stream;

	if(deflated)
	{
		memset(&stream, 0, sizeof(stream));
		if(inflateInit2(&stream, -MAX_WBITS) != Z_OK)
			return -1;
			// -MAX_WBITS indicates no zlib header present

		read_bufs = mem_alloc(BLOCK_SIZE*2, MEM_HEAP, 64*KB);
		if(!read_bufs)
			return -1;

		cbs[0].aio_buf = read_bufs;
		cbs[1].aio_buf = (u8*)read_bufs + BLOCK_SIZE;
	}

	void* out_mem = mem_alloc(ucsize, MEM_HEAP, 64*KB);

	// pos in output buffer when reading uncompressed data
	u8* out_pos = (u8*)out_mem;

	stream.next_out = (u8*)out_mem;
	stream.avail_out = ucsize;


	long cbytes_left = csize;

	bool first = true;
	bool done = false;

	for(;;)
	{
		aiocb* cb = &cbs[active_read];

		// start reading next block
		if(cbytes_left)
		{
			// align to 64 KB for speed
			u32 rsize = min(64*KB - (ofs & 0xffff), cbytes_left);

			// if uncompressed, read directly into output buffer
			if(!deflated)
			{
				cb->aio_buf = out_pos;
				out_pos += rsize;
			}

			cb->aio_offset = ofs;
			cb->aio_nbytes = rsize;
			aio_read(cb);

			ofs += rsize;
			cbytes_left -= rsize;
			assert(cbytes_left >= 0);	// read size clamped => never negative
		}

		active_read ^= 1;

		// process block read in previous iteration
		if(first)
			first = false;
		else
		{
			// wait for read to complete
			cb = &cbs[active_read];
			while(aio_error(cb) == -EINPROGRESS)
				aio_suspend(&cb, 1, 0);

			ssize_t bytes_read = aio_return(cb);

			// inflate
			if(deflated)
			{
				stream.next_in = (u8*)cb->aio_buf;
				stream.avail_in = bytes_read;
				inflate(&stream, 0);
			}
		}

		// one more iteration to process the last pending block
		if(done)
			break;
		if(!cbytes_left)
			done = true;
	}

	if(deflated)
	{
		inflateEnd(&stream);
		mem_free(read_bufs);

		if(stream.total_in != csize || stream.total_out != ucsize)
		{
// read_bufs is not yet allocated, or already freed
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
