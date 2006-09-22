/**
 * =========================================================================
 * File        : zip.cpp
 * Project     : 0 A.D.
 * Description : archive backend for Zip files.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2006 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"

#include <time.h>
#include <limits>

#include "lib/lib.h"
#include "lib/byte_order.h"
#include "lib/allocators.h"
#include "lib/timer.h"
#include "file_internal.h"
#include "lib/res/res.h"


// safe downcasters: cast from any integral type to u32 or u16; 
// issues warning if larger than would fit in the target type.
//
// these are generally useful but included here (instead of e.g. lib.h) for
// several reasons:
// - including implementation in lib.h doesn't work because the definition
//   of debug_assert in turn requires lib.h's STMT.
// - separate compilation of templates via export isn't supported by
//   most compilers.

template<typename T> u32 u32_from_larger(T x)
{
	const u32 max = std::numeric_limits<u32>::max();
	debug_assert((u64)x <= (u64)max);
	return (u32)(x & max);
}

template<typename T> u16 u16_from_larger(T x)
{
	const u16 max = std::numeric_limits<u16>::max();
	debug_assert((u64)x <= (u64)max);
	return (u16)(x & max);
}


//-----------------------------------------------------------------------------
// timestamp conversion: DOS FAT <-> Unix time_t
//-----------------------------------------------------------------------------

// must not be static because these are tested by unit test

time_t time_t_from_FAT(u32 fat_timedate)
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


u32 FAT_from_time_t(time_t time)
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
// Zip file data structures and signatures
//-----------------------------------------------------------------------------

enum ZipCompressionMethod
{
	ZIP_CM_NONE    = 0,
	ZIP_CM_DEFLATE = 8
};

// translate ArchiveEntry.method to zip_method.
static ZipCompressionMethod zip_method_for(CompressionMethod method)
{
	switch(method)
	{
	case CM_NONE:
		return ZIP_CM_NONE;
	case CM_DEFLATE:
		return ZIP_CM_DEFLATE;
	default:
		WARN_ERR(ERR::COMPRESSION_UNKNOWN_METHOD);
		return ZIP_CM_NONE;
	}
}

// translate to (not Zip-specific) CompressionMethod for use in ArchiveEntry.
static CompressionMethod method_for_zip_method(ZipCompressionMethod zip_method)
{
	switch(zip_method)
	{
	case ZIP_CM_NONE:
		return CM_NONE;
	case ZIP_CM_DEFLATE:
		return CM_DEFLATE;
	default:
		WARN_ERR(ERR::COMPRESSION_UNKNOWN_METHOD);
		return CM_UNSUPPORTED;
	}
}


static const u32 cdfh_magic = FOURCC_LE('P','K','\1','\2');
static const u32  lfh_magic = FOURCC_LE('P','K','\3','\4');
static const u32 ecdr_magic = FOURCC_LE('P','K','\5','\6');

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

// convenience (allows writing out LFH and fn in 1 IO).
// must be declared here to avoid any struct padding.
struct LFH_Package
{
	LFH lfh;
	char fn[PATH_MAX];
};


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

// convenience (avoids need for pointer arithmetic)
// must be declared here to avoid any struct padding.
struct CDFH_Package
{
	CDFH cdfh;
	char fn[PATH_MAX];
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

const size_t ECDR_SIZE = sizeof(ECDR);
cassert(ECDR_SIZE == 22);

#pragma pack(pop)


static off_t lfh_total_size(const LFH* lfh_le)
{
	debug_assert(lfh_le->magic == lfh_magic);
	const size_t fn_len = read_le16(&lfh_le->fn_len);
	const size_t  e_len = read_le16(&lfh_le->e_len);
	// note: LFH doesn't have a comment field!

	return (off_t)(LFH_SIZE + fn_len + e_len);
}

static void lfh_assemble(LFH* lfh_le,
	CompressionMethod method, time_t mtime, u32 crc,
	off_t csize, off_t ucsize, size_t fn_len)
{
	const ZipCompressionMethod zip_method = zip_method_for(method);
	const u32 fat_mtime = FAT_from_time_t(mtime);

	lfh_le->magic     = lfh_magic;
	lfh_le->x1        = to_le16(0);
	lfh_le->flags     = to_le16(0);
	lfh_le->method    = to_le16(zip_method);
	lfh_le->fat_mtime = to_le32(fat_mtime);
	lfh_le->crc       = to_le32(crc);
	lfh_le->csize     = to_le32(u32_from_larger(csize));
	lfh_le->ucsize    = to_le32(u32_from_larger(ucsize));
	lfh_le->fn_len    = to_le16(u16_from_larger(fn_len));
	lfh_le->e_len     = to_le16(0);
}


static void cdfh_decompose(const CDFH* cdfh_le,
	CompressionMethod& method, time_t& mtime, off_t& csize, off_t& ucsize,
	const char*& fn, off_t& lfh_ofs, size_t& total_size)
{
	const u16 zip_method = read_le16(&cdfh_le->method);
	const u32 fat_mtime  = read_le32(&cdfh_le->fat_mtime);
	csize         = (off_t)read_le32(&cdfh_le->csize);
	ucsize        = (off_t)read_le32(&cdfh_le->ucsize);
	const u16 fn_len     = read_le16(&cdfh_le->fn_len);
	const u16 e_len      = read_le16(&cdfh_le->e_len);
	const u16 c_len      = read_le16(&cdfh_le->c_len);
	lfh_ofs       = (off_t)read_le32(&cdfh_le->lfh_ofs);

	method = method_for_zip_method((ZipCompressionMethod)zip_method);
	mtime = time_t_from_FAT(fat_mtime);

	// return 0-terminated copy of filename
	const char* fn_src = (const char*)cdfh_le+CDFH_SIZE; // not 0-terminated!
	char fn_buf[PATH_MAX];
	memcpy2(fn_buf, fn_src, fn_len*sizeof(char));
	fn_buf[fn_len] = '\0';
	fn = file_make_unique_fn_copy(fn_buf);

	total_size = CDFH_SIZE + fn_len + e_len + c_len;
}

static void cdfh_assemble(CDFH* dst_cdfh_le,
	CompressionMethod method, time_t mtime, u32 crc,
	size_t csize, size_t ucsize, size_t fn_len, size_t slack, u32 lfh_ofs)
{
	const ZipCompressionMethod zip_method = zip_method_for(method);
	const u32 fat_mtime = FAT_from_time_t(mtime);

	dst_cdfh_le->magic     = cdfh_magic;
	dst_cdfh_le->x1        = to_le32(0);
	dst_cdfh_le->flags     = to_le16(0);
	dst_cdfh_le->method    = to_le16(zip_method);
	dst_cdfh_le->fat_mtime = to_le32(fat_mtime);
	dst_cdfh_le->crc       = to_le32(crc);
	dst_cdfh_le->csize     = to_le32(u32_from_larger(csize));
	dst_cdfh_le->ucsize    = to_le32(u32_from_larger(ucsize));
	dst_cdfh_le->fn_len    = to_le16(u16_from_larger(fn_len));
	dst_cdfh_le->e_len     = to_le16(0);
	dst_cdfh_le->c_len     = to_le16(u16_from_larger(slack));
	dst_cdfh_le->x2        = to_le32(0);
	dst_cdfh_le->x3        = to_le32(0);
	dst_cdfh_le->lfh_ofs   = to_le32(lfh_ofs);
}


static void ecdr_decompose(ECDR* ecdr_le,
	uint& cd_entries, off_t& cd_ofs, size_t& cd_size)
{
	cd_entries = (uint)read_le16(&ecdr_le->cd_entries);
	cd_ofs    = (off_t)read_le32(&ecdr_le->cd_ofs);
	cd_size  = (size_t)read_le32(&ecdr_le->cd_size);
}

static void ecdr_assemble(ECDR* dst_ecdr_le, uint cd_entries, off_t cd_ofs, size_t cd_size)
{
	dst_ecdr_le->magic       = ecdr_magic;
	memset(dst_ecdr_le->x1, 0, sizeof(dst_ecdr_le->x1));
	dst_ecdr_le->cd_entries  = to_le16(u16_from_larger(cd_entries));
	dst_ecdr_le->cd_size     = to_le32(u32_from_larger(cd_size));
	dst_ecdr_le->cd_ofs      = to_le32(u32_from_larger(cd_ofs));
	dst_ecdr_le->comment_len = to_le16(0);
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
// if found, fill <dst_ecdr> with a copy of the (little-endian) ECDR and
// return INFO::OK, otherwise IO error or ERR::CORRUPTED.
static LibError za_find_ecdr(File* f, size_t max_scan_amount, ECDR* dst_ecdr_le)
{
	// don't scan more than the entire file
	const size_t file_size = f->size;
	const size_t scan_amount = MIN(max_scan_amount, file_size);

	// read desired chunk of file into memory
	const off_t ofs = (off_t)(file_size - scan_amount);
	FileIOBuf buf = FILE_BUF_ALLOC;
	ssize_t bytes_read = file_io(f, ofs, scan_amount, &buf);
	RETURN_ERR(bytes_read);
	debug_assert(bytes_read == (ssize_t)scan_amount);

	// look for ECDR in buffer
	LibError ret  = ERR::CORRUPTED;
	const u8* start = (const u8*)buf;
	const ECDR* ecdr_le = (const ECDR*)za_find_id(start, bytes_read, start, ecdr_magic, ECDR_SIZE);
	if(ecdr_le)
	{
		*dst_ecdr_le = *ecdr_le;
		ret = INFO::OK;
	}

	file_buf_free(buf);
	return ret;
}


static LibError za_find_cd(File* f, uint& cd_entries, off_t& cd_ofs, size_t& cd_size)
{
	// sanity check: file size must be > header size.
	// (this speeds up determining if the file is a Zip file at all)
	const size_t file_size = f->size;
	if(file_size < LFH_SIZE+CDFH_SIZE+ECDR_SIZE)
	{
completely_bogus:
		// this file is definitely not a valid Zip file.
		// note: the VFS blindly opens files when mounting; it needs to open
		// all archives, but doesn't know their extension (e.g. ".pk3").
		// therefore, do not warn user.
		return ERR::RES_UNKNOWN_FORMAT;	// NOWARN
	}

	ECDR ecdr_le;
	// expected case: ECDR at EOF; no file comment (=> we only need to
	// read 512 bytes)
	LibError ret = za_find_ecdr(f, ECDR_SIZE, &ecdr_le);
	if(ret == INFO::OK)
	{
have_ecdr:
		ecdr_decompose(&ecdr_le, cd_entries, cd_ofs, cd_size);
		return INFO::OK;
	}
	// last resort: scan last 66000 bytes of file
	// (the Zip archive comment field - up to 64k - may follow ECDR).
	// if the zip file is < 66000 bytes, scan the whole file.
	ret = za_find_ecdr(f, 66000u, &ecdr_le);
	if(ret == INFO::OK)
		goto have_ecdr;

	// both ECDR scans failed - this is not a valid Zip file.
	// now see if the beginning of the file holds a valid LFH:
	const off_t ofs = 0; const size_t scan_amount = LFH_SIZE;
	FileIOBuf buf = FILE_BUF_ALLOC;
	ssize_t bytes_read = file_io(f, ofs, scan_amount, &buf);
	RETURN_ERR(bytes_read);
	debug_assert(bytes_read == (ssize_t)scan_amount);
	const bool has_LFH = (za_find_id(buf, scan_amount, buf, lfh_magic, LFH_SIZE) != 0);
	file_buf_free(buf);
	if(!has_LFH)
		goto completely_bogus;
	// the Zip file is mostly valid but lacking an ECDR. (can happen if
	// user hard-exits while building an archive)
	// notes:
	// - return ERR::CORRUPTED so VFS will not include this file.
	// - we could work around this by scanning all LFHs, but won't bother
	//   because it'd be slow.
	// - do not warn - the corrupt archive will be deleted on next
	//   successful archive builder run anyway.
	return ERR::CORRUPTED;	// NOWARN
}


// analyse an opened Zip file; call back into archive.cpp to
// populate the Archive object with a list of the files it contains.
// returns INFO::OK on success, ERR::CORRUPTED if file is recognizable as
// a Zip file but invalid, otherwise ERR::RES_UNKNOWN_FORMAT or IO error.
//
// fairly slow - must read Central Directory from disk
// (size ~= 60 bytes*num_files); observed time ~= 80ms.
LibError zip_populate_archive(File* f, Archive* a)
{
	uint cd_entries; off_t cd_ofs; size_t cd_size;
	RETURN_ERR(za_find_cd(f, cd_entries, cd_ofs, cd_size));

	// call back with number of entries in archives (an upper bound
	// for valid files; we're not interested in the directory entries).
	// we'd have to scan through the central dir to count them out; we'll
	// just skip them and waste a bit of preallocated memory.
	RETURN_ERR(archive_allocate_entries(a, cd_entries));

	FileIOBuf buf = FILE_BUF_ALLOC;
	RETURN_ERR(file_io(f, cd_ofs, cd_size, &buf));

	// iterate through Central Directory
	LibError ret = INFO::OK;
	const CDFH* cdfh = (const CDFH*)buf;
	size_t ofs_to_next_cdfh = 0;
	for(uint i = 0; i < cd_entries; i++)
	{
		// scan for next CDFH (at or beyond current cdfh position)
		cdfh = (const CDFH*)((u8*)cdfh + ofs_to_next_cdfh);
		cdfh = (CDFH*)za_find_id((const u8*)buf, cd_size, (const u8*)cdfh, cdfh_magic, CDFH_SIZE);
		if(!cdfh)	// no (further) CDFH found:
		{
			ret  = ERR::CORRUPTED;
			break;
		}

		// copy translated fields from CDFH into ArchiveEntry.
		ArchiveEntry ae;
		cdfh_decompose(cdfh, ae.method, ae.mtime, ae.csize, ae.ucsize, ae.atom_fn, ae.ofs, ofs_to_next_cdfh);
		ae.flags = ZIP_LFH_FIXUP_NEEDED;

		// if file (we don't care about directories):
		if(ae.csize && ae.ucsize)
		{
			ret = archive_add_file(a, &ae);
			if(ret != INFO::OK)
				break;
		}
	}

	file_buf_free(buf);
	return ret;
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
	return INFO::CB_CONTINUE;
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

	ent->ofs += lfh_total_size(&lfh);
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

	ZipArchive* za = za_mgr.alloc();
	if(!za)
		WARN_RETURN(ERR::NO_MEM);
	*za = za_copy;
	*pza = za;
	return INFO::OK;
}


// add a file (described by ArchiveEntry) to the archive. file_contents
// is the actual file data; its compression method is given in ae->method and
// can be CM_NONE.
// IO cost: writes out <file_contents> to disk (we don't currently attempt
// any sort of write-buffering).
LibError zip_archive_add_file(ZipArchive* za, const ArchiveEntry* ae, void* file_contents)
{
	const size_t fn_len = strlen(ae->atom_fn);

	// write (LFH, filename, file contents) to archive
	// .. put LFH and filename into one 'package'
	LFH_Package header;
	lfh_assemble(&header.lfh, ae->method, ae->mtime, ae->crc, ae->csize, ae->ucsize, fn_len);
	strcpy_s(header.fn, ARRAY_SIZE(header.fn), ae->atom_fn);
	// .. write that out in 1 IO
	const off_t lfh_ofs = za->cur_file_size;
	FileIOBuf buf;
	buf = (FileIOBuf)&header;
	file_io(&za->f, lfh_ofs, LFH_SIZE+fn_len, &buf);
	// .. write out file contents
	buf = (FileIOBuf)file_contents;
	file_io(&za->f, lfh_ofs+(off_t)(LFH_SIZE+fn_len), ae->csize, &buf);
	za->cur_file_size += (off_t)(LFH_SIZE+fn_len+ae->csize);

	// append a CDFH to the central dir (in memory)
	// .. note: pool_alloc may round size up for padding purposes.
	const size_t prev_pos = za->cdfhs.da.pos;
	CDFH_Package* p = (CDFH_Package*)pool_alloc(&za->cdfhs, CDFH_SIZE+fn_len);
	if(!p)
		WARN_RETURN(ERR::NO_MEM);
	const size_t slack = za->cdfhs.da.pos-prev_pos - (CDFH_SIZE+fn_len);
	cdfh_assemble(&p->cdfh, ae->method, ae->mtime, ae->crc, ae->csize, ae->ucsize, fn_len, slack, lfh_ofs);
	memcpy2(p->fn, ae->atom_fn, fn_len);

	za->cd_entries++;

	return INFO::OK;
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
		WARN_RETURN(ERR::NO_MEM);
	ecdr_assemble(ecdr, za->cd_entries, za->cur_file_size, cd_size);

	FileIOBuf buf = za->cdfhs.da.base;
	file_io(&za->f, za->cur_file_size, cd_size+ECDR_SIZE, &buf);

	(void)file_close(&za->f);
	(void)pool_destroy(&za->cdfhs);
	za_mgr.release(za);
	return INFO::OK;
}
