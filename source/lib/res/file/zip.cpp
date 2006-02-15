// archive backend for Zip files
//
// Copyright (c) 2003-2006 Jan Wassenberg
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

#include <time.h>

#include "lib.h"
#include "byte_order.h"
#include "allocators.h"
#include "timer.h"
#include "self_test.h"
#include "file_internal.h"


// Zip file data structures and signatures
static const u32 cdfh_magic = FOURCC_LE('P','K','\1','\2');
static const u32  lfh_magic = FOURCC_LE('P','K','\3','\4');
static const u32 ecdr_magic = FOURCC_LE('P','K','\5','\6');

enum ZipCompressionMethod
{
	ZIP_CM_NONE    = 0,
	ZIP_CM_DEFLATE = 8
};

#pragma pack(push, 1)

struct LFH
{
	u32 magic;
	u16 x1;			// version needed
	u16 flags;
	u16 method;
	u32 fat_mtime;	// last modified time (DOS FAT format)
	u32 crc;
	u32 csize;
	u32 ucsize;
	u16 fn_len;
	u16 e_len;
};

const size_t LFH_SIZE = sizeof(LFH);
cassert(LFH_SIZE == 30);


struct CDFH
{
	u32 magic;
	u32 x1;			// versions
	u16 flags;
	u16 method;
	u32 fat_mtime;	// last modified time (DOS FAT format)
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

const size_t CDFH_SIZE = sizeof(CDFH);
cassert(CDFH_SIZE == 46);


struct ECDR
{
	u32 magic;
	u8 x1[6];	// multiple-disk support
	u16 cd_entries;
	u32 cd_size;
	u32 cd_ofs;
	u16 comment_len;
};

const size_t ECDR_SIZE = sizeof(ECDR);
cassert(ECDR_SIZE == 22);

#pragma pack(pop)



//
// timestamp conversion: DOS FAT <-> Unix time_t
//

static time_t time_t_from_FAT(u32 fat_timedate)
{
	const uint fat_time = bits(fat_timedate, 0, 15);
	const uint fat_date = bits(fat_timedate, 16, 31);

	struct tm t;							// struct tm format:
	t.tm_sec   = bits(fat_time, 0,4) * 2;	// [0,59]
	t.tm_min   = bits(fat_time, 5,10);		// [0,59]
	t.tm_hour  = bits(fat_time, 11,15);		// [0,23]
	t.tm_mday  = bits(fat_date, 0,4);		// [1,31]
	t.tm_mon   = bits(fat_date, 5,8) - 1;	// [0,11]
	t.tm_year  = bits(fat_date, 9,15) + 80;	// since 1900
	t.tm_isdst = -1;	// unknown - let libc determine

	// otherwise: totally bogus, and at the limit of 32-bit time_t
	debug_assert(t.tm_year < 138);

	time_t ret = mktime(&t);
	if(ret == (time_t)-1)
		debug_warn("mktime failed");
	return ret;
}


static u32 FAT_from_time_t(time_t time)
{
	// (values are adjusted for DST)
	struct tm* t = localtime(&time);

	u16 fat_time = 0;
	fat_time |= (t->tm_sec/2);		// 5
	fat_time |= (t->tm_min) << 5;	// 6
	fat_time |= (t->tm_hour) << 11;	// 5

	u16 fat_date = 0;
	fat_date |= (t->tm_mday);			// 5
	fat_date |= (t->tm_mon+1) << 5;		// 4
	fat_date |= (t->tm_year-80) << 9;	// 7

	u32 fat_timedate = u32_from_u16(fat_date, fat_time);
	return fat_timedate;
}


//-----------------------------------------------------------------------------

// scan for and return a pointer to a Zip record, or 0 if not found.
// <start> is the expected position; we scan from there until EOF for
// the given ID (fourcc). <record_size> includes ID field) bytes must
// remain before EOF - this makes sure the record is completely in the file.
// used by z_find_ecdr and z_extract_cdfh.
static const u8* za_find_id(const u8* buf, size_t size, const void* start, u32 magic, size_t record_size)
{
	ssize_t bytes_left = (ssize_t)((buf+size) - (u8*)start - record_size);

	const u8* p = (const u8*)start;
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
	// note: do not warn - this happens in the initial ECDR search at
	// EOF if the archive contains a comment field.
	return 0;
}


// search for ECDR in the last <max_scan_amount> bytes of the file.
// if found, fill <dst_ecdr> with an (unprocessed) copy of the record and
// return ERR_OK, otherwise IO error or ERR_CORRUPTED.
static LibError za_find_ecdr_impl(File* f, size_t max_scan_amount, ECDR* dst_ecdr)
{
	// don't scan more than the entire file
	const size_t file_size = f->fc.size;
	const size_t scan_amount = MIN(max_scan_amount, file_size);

	// read desired chunk of file into memory
	const off_t ofs = (off_t)(file_size - scan_amount);
	FileIOBuf buf = FILE_BUF_ALLOC;
	ssize_t bytes_read = file_io(f, ofs, scan_amount, &buf);
	RETURN_ERR(bytes_read);
	debug_assert(bytes_read == (ssize_t)scan_amount);

	// look for ECDR in buffer
	LibError ret = ERR_CORRUPTED;
	const u8* start = (const u8*)buf;
	const ECDR* ecdr = (const ECDR*)za_find_id(start, bytes_read, start, ecdr_magic, ECDR_SIZE);
	if(ecdr)
	{
		*dst_ecdr = *ecdr;
		ret = ERR_OK;
	}

	file_buf_free(buf);
	return ret;
}


// read the current CDFH. if a valid file, return its filename and ZLoc.
// return -1 on error (output params invalid), or 0 on success.
// called by za_enum_files, which passes the output to lookup.
static LibError za_extract_cdfh(const CDFH* cdfh,
	ArchiveEntry* ent, size_t& ofs_to_next_cdfh)
{
	// extract fields from CDFH
	const u16 zip_method = read_le16(&cdfh->method);
	const u32 fat_mtime  = read_le32(&cdfh->fat_mtime);
	const u32 csize      = read_le32(&cdfh->csize);
	const u32 ucsize     = read_le32(&cdfh->ucsize);
	const u16 fn_len     = read_le16(&cdfh->fn_len);
	const u16 e_len      = read_le16(&cdfh->e_len);
	const u16 c_len      = read_le16(&cdfh->c_len);
	const u32 lfh_ofs    = read_le32(&cdfh->lfh_ofs);
	const char* fn_tmp = (const char*)cdfh+CDFH_SIZE;	// not 0-terminated!

	CompressionMethod method;
	switch(zip_method)
	{
	case ZIP_CM_NONE: method = CM_NONE; break;
	case ZIP_CM_DEFLATE: method = CM_DEFLATE; break;
	default: WARN_RETURN(ERR_UNKNOWN_CMETHOD);
	}
		
	// .. it's a directory entry (we only want files)
	if(!csize && !ucsize)
		return ERR_NOT_FILE;	// don't warn - we just ignore these

	// null-terminate fn (must be done before file_make_unique_fn_copy)
	char fn[PATH_MAX];
	memcpy2(fn, fn_tmp, fn_len);
	fn[fn_len] = '\0';

	// write out entry data
	ent->atom_fn = file_make_unique_fn_copy(fn);
	ent->ofs     = lfh_ofs;
	ent->csize   = csize;
	ent->ucsize  = (off_t)ucsize;
	ent->mtime   = time_t_from_FAT(fat_mtime);
	ent->method  = method;
	ent->flags   = ZIP_LFH_FIXUP_NEEDED;

	// offset to where next CDFH should be (caller will scan for it)
	ofs_to_next_cdfh = CDFH_SIZE + fn_len + e_len + c_len;

	return ERR_OK;
}


// analyse an opened Zip file; call back into archive.cpp to
// populate the Archive object with a list of the files it contains.
// returns ERR_OK on success, ERR_UNKNOWN_FORMAT if not a Zip file
// (see below) or another negative LibError code.
//
// fairly slow - must read Central Directory from disk
// (size ~= 60 bytes*num_files); observed time ~= 80ms.
LibError zip_populate_archive(File* f, Archive* a)
{
	LibError ret;

	// sanity check: file size must be > header size.
	// (this speeds up determining if the file is a Zip file at all)
	const size_t file_size = f->fc.size;
	if(file_size < LFH_SIZE+CDFH_SIZE+ECDR_SIZE)
		goto completely_bogus;

	{

	// find "End of Central Dir Record" in file.
	ECDR ecdr;
	// .. early out: check expected case (ECDR at EOF; no file comment)
	ret = za_find_ecdr_impl(f, ECDR_SIZE, &ecdr);
	// .. second try: scan last 66000 bytes of file
	// (the Zip archive comment field - up to 64k - may follow ECDR).
	// if the zip file is < 66000 bytes, scan the whole file.
	if(ret < 0)
	{
		ret = za_find_ecdr_impl(f, 66000u, &ecdr);
		// still failed - not a Zip file
		if(ret < 0)
			goto completely_bogus;
	}
	const uint   cd_entries =   (uint)read_le16(&ecdr.cd_entries);
	const off_t  cd_ofs     =  (off_t)read_le32(&ecdr.cd_ofs);
	const size_t cd_size    = (size_t)read_le32(&ecdr.cd_size);

	// call back with number of entries in archives (an upper bound
	// for valid files; we're not interested in the directory entries).
	// we'd have to scan through the central dir to count them out; we'll
	// just skip them and waste a bit of preallocated memory.
	RETURN_ERR(archive_allocate_entries(a, cd_entries));

	// iterate through Central Directory
	FileIOBuf buf = FILE_BUF_ALLOC;
	RETURN_ERR(file_io(f, cd_ofs, cd_size, &buf));
	ret = ERR_OK;
	const CDFH* cdfh = (const CDFH*)buf;
	size_t ofs_to_next_cdfh = 0;
	for(uint i = 0; i < cd_entries; i++)
	{
		// scan for next CDFH (at or beyond current cdfh position)
		cdfh = (const CDFH*)((u8*)cdfh + ofs_to_next_cdfh);
		cdfh = (CDFH*)za_find_id((const u8*)buf, cd_size, (const u8*)cdfh, cdfh_magic, CDFH_SIZE);
		if(!cdfh)	// no (further) CDFH found:
		{
			ret = ERR_CORRUPTED;
			break;
		}

		ArchiveEntry ent;
		if(za_extract_cdfh(cdfh, &ent, ofs_to_next_cdfh) == ERR_OK)
		{
			ret = archive_add_file(a, &ent);
			if(ret != ERR_OK)
				break;
		}
	}
	file_buf_free(buf);

	return ret;

	}

	// this file is definitely not a valid Zip file.
	// note: the VFS blindly opens files when mounting; it needs to open
	// all archives, but doesn't know their extension (e.g. ".pk3").
	// therefore, do not warn user.
completely_bogus:
	return ERR_UNKNOWN_FORMAT;
}


//-----------------------------------------------------------------------------

// this code grabs an LFH struct from file block(s) that are
// passed to the callback. usually, one call copies the whole thing,
// but the LFH may straddle a block boundary.
//
// rationale: this allows using temp buffers for zip_fixup_lfh,
// which avoids involving the file buffer manager and thus
// unclutters the trace and cache contents.

struct LFH_Copier
{
	u8* lfh_dst;
	size_t lfh_bytes_remaining;
};

static LibError lfh_copier_cb(uintptr_t ctx, const void* block, size_t size, size_t* bytes_processed)
{
	LFH_Copier* p = (LFH_Copier*)ctx;

	debug_assert(size <= p->lfh_bytes_remaining);
	memcpy2(p->lfh_dst, block, size);
	p->lfh_dst += size;
	p->lfh_bytes_remaining -= size;

	*bytes_processed = size;
	return INFO_CB_CONTINUE;
}


// ensures <ent.ofs> points to the actual file contents; it is initially
// the offset of the LFH. we cannot use CDFH filename and extra field
// lengths to skip past LFH since that may not mirror CDFH (has happened).
//
// this is called at file-open time instead of while mounting to
// reduce seeks: since reading the file will typically follow, the
// block cache entirely absorbs the IO cost.
void zip_fixup_lfh(File* f, ArchiveEntry* ent)
{
	// already fixed up - done.
	if(!(ent->flags & ZIP_LFH_FIXUP_NEEDED))
		return;

	// performance note: this ends up reading one file block, which is
	// only in the block cache if the file starts in the same block as a
	// previously read file (i.e. both are small).
	LFH lfh;
	LFH_Copier params = { (u8*)&lfh, sizeof(LFH) };
	ssize_t ret = file_io(f, ent->ofs, LFH_SIZE, FILE_BUF_TEMP, lfh_copier_cb, (uintptr_t)&params);
	debug_assert(ret == sizeof(LFH));

	debug_assert(lfh.magic == lfh_magic);
	const size_t fn_len = read_le16(&lfh.fn_len);
	const size_t  e_len = read_le16(&lfh.e_len);

	ent->ofs += (off_t)(LFH_SIZE + fn_len + e_len);
	// LFH doesn't have a comment field!

	ent->flags &= ~ZIP_LFH_FIXUP_NEEDED;
}


//-----------------------------------------------------------------------------
// archive builder backend
//-----------------------------------------------------------------------------

// rationale: don't support partial adding, i.e. updating archive with
// only one file. this would require overwriting parts of the Zip archive,
// which is annoying and slow. also, archives are usually built in
// seek-optimal order, which would break if we start inserting files.
// while testing, loose files can be used, so there's no loss.

// we don't want to expose ZipArchive to callers,
// (would require defining File, Pool and CDFH)
// so allocate the storage here and return opaque pointer.
struct ZipArchive
{
	File f;
	off_t cur_file_size;

	Pool cdfhs;
	uint cd_entries;
	CDFH* prev_cdfh;
};

static SingleAllocator<ZipArchive> za_mgr;


// create a new Zip archive and return a pointer for use in subsequent
// zip_archive_add_file calls. previous archive file is overwritten.
LibError zip_archive_create(const char* zip_filename, ZipArchive** pza)
{
	// local za_copy simplifies things - if something fails, no cleanup is
	// needed. upon success, we copy into the newly allocated real za.
	ZipArchive za_copy;
	za_copy.cur_file_size = 0;
	za_copy.cd_entries    = 0;
	za_copy.prev_cdfh     = 0;

	RETURN_ERR(file_open(zip_filename, FILE_WRITE|FILE_NO_AIO, &za_copy.f));
	RETURN_ERR(pool_create(&za_copy.cdfhs, 10*MiB, 0));

	ZipArchive* za = (ZipArchive*)za_mgr.alloc();
	if(!za)
		WARN_RETURN(ERR_NO_MEM);
	*za = za_copy;
	*pza = za;
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

// add a file (described by ArchiveEntry) to the archive. file_contents
// is the actual file data; its compression method is given in ae->method and
// can be CM_NONE.
// IO cost: writes out <file_contents> to disk (we don't currently attempt
// any sort of write-buffering).
LibError zip_archive_add_file(ZipArchive* za, const ArchiveEntry* ae, void* file_contents)
{
	const char* fn      = ae->atom_fn;
	const size_t fn_len = strlen(fn);
	const size_t ucsize = ae->ucsize;
	const u32 fat_mtime = FAT_from_time_t(ae->mtime);
	const size_t csize  = ae->csize;
	const u32 crc32     = ae->crc32;
	u16 zip_method;
	switch(ae->method)
	{
	case CM_NONE: zip_method = ZIP_CM_NONE; break;
	case CM_DEFLATE: zip_method = ZIP_CM_DEFLATE; break;
	default: WARN_RETURN(ERR_UNKNOWN_CMETHOD);
	}

	const off_t lfh_ofs = za->cur_file_size;

	// write (LFH, filename, file contents) to archive
	const LFH lfh =
	{
		to_le32(lfh_magic),
		to_le16(0),	// x1
		to_le16(0),	// flags
		to_le16(zip_method),
		to_le32(fat_mtime),
		to_le32(crc32),
		to_le32(u32_from_size_t(csize)),
		to_le32(u32_from_size_t(ucsize)),
		to_le16(u16_from_size_t(fn_len)),
		to_le16(0)	// e_len
	};
	FileIOBuf buf;
	buf = (FileIOBuf)&lfh;
	file_io(&za->f, lfh_ofs, LFH_SIZE, &buf);
	buf = (FileIOBuf)fn;
	file_io(&za->f, lfh_ofs+LFH_SIZE, fn_len, &buf);
	buf = (FileIOBuf)file_contents;
	file_io(&za->f, lfh_ofs+(off_t)(LFH_SIZE+fn_len), csize, &buf);
	za->cur_file_size += (off_t)(LFH_SIZE+fn_len+csize);

	// append a CDFH to the central dir (in memory)
	// .. note: pool_alloc may round size up for padding purposes.
	const size_t prev_pos = za->cdfhs.da.pos;
	CDFH* cdfh = (CDFH*)pool_alloc(&za->cdfhs, CDFH_SIZE+fn_len);
	if(!cdfh)
		return ERR_NO_MEM;
	const size_t slack = za->cdfhs.da.pos-prev_pos - (CDFH_SIZE+fn_len);
	// .. store header fields
	cdfh->magic     = to_le32(cdfh_magic);
	cdfh->x1        = to_le32(0);
	cdfh->flags     = to_le16(0);
	cdfh->method    = to_le16(zip_method);
	cdfh->fat_mtime = to_le32(fat_mtime);
	cdfh->crc       = to_le32(crc32);
	cdfh->csize     = to_le32(u32_from_size_t(csize));
	cdfh->ucsize    = to_le32(u32_from_size_t(ucsize));
	cdfh->fn_len    = to_le16(u16_from_size_t(fn_len));
	cdfh->e_len     = to_le16(0);
	cdfh->c_len     = to_le16(u16_from_size_t(slack));
	cdfh->x2        = to_le32(0);
	cdfh->x3        = to_le32(0);
	cdfh->lfh_ofs   = to_le32(lfh_ofs);
	memcpy2((char*)cdfh+CDFH_SIZE, fn, fn_len);

	za->cd_entries++;

	return ERR_OK;
}


// write out the archive to disk; only hereafter is it valid.
// frees the ZipArchive instance.
// IO cost: writes out Central Directory to disk (about 70 bytes per file).
LibError zip_archive_finish(ZipArchive* za)
{
	const size_t cd_size = za->cdfhs.da.pos;

	// append an ECDR to the CDFH list (this allows us to
	// write out both to the archive file in one burst)
	ECDR* ecdr = (ECDR*)pool_alloc(&za->cdfhs, ECDR_SIZE);
	if(!ecdr)
		return ERR_NO_MEM;
	ecdr->magic       = ecdr_magic;
	memset(ecdr->x1, 0, sizeof(ecdr->x1));
	ecdr->cd_entries  = za->cd_entries;
	ecdr->cd_size     = (u32)cd_size;
	ecdr->cd_ofs      = za->cur_file_size;
	ecdr->comment_len = 0;

	FileIOBuf buf = za->cdfhs.da.base;
	file_io(&za->f, za->cur_file_size, cd_size+ECDR_SIZE, &buf);

	(void)file_close(&za->f);
	(void)pool_destroy(&za->cdfhs);
#include "nommgr.h"
	za_mgr.free(za);
#include "mmgr.h"
	return ERR_OK;
}


//-----------------------------------------------------------------------------
// built-in self test
//-----------------------------------------------------------------------------

#if SELF_TEST_ENABLED
namespace test {

static void test_fat_timedate_conversion()
{
	// note: FAT time stores second/2, which means converting may
	// end up off by 1 second.

	time_t t, converted_t;
	long dt;

	t = time(0);
	converted_t = time_t_from_FAT(FAT_from_time_t(t));
	dt = converted_t-t;	// disambiguate abs() parameter
	TEST(abs(dt) < 2);

	t++;
	converted_t = time_t_from_FAT(FAT_from_time_t(t));
	dt = converted_t-t;	// disambiguate abs() parameter
	TEST(abs(dt) < 2);
}

static void self_test()
{
	test_fat_timedate_conversion();
}

SELF_TEST_RUN;

}	// namespace test
#endif	// #if SELF_TEST_ENABLED
