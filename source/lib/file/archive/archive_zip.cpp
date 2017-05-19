/* Copyright (c) 2017 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * archive backend for Zip files.
 */

#include "precompiled.h"
#include "lib/file/archive/archive_zip.h"

#include <time.h>
#include <limits>

#include "lib/utf8.h"
#include "lib/bits.h"
#include "lib/byte_order.h"
#include "lib/allocators/pool.h"
#include "lib/sysdep/filesystem.h"
#include "lib/file/archive/archive.h"
#include "lib/file/archive/codec_zlib.h"
#include "lib/file/archive/stream.h"
#include "lib/file/file.h"
#include "lib/file/io/io.h"

//-----------------------------------------------------------------------------
// timestamp conversion: DOS FAT <-> Unix time_t
//-----------------------------------------------------------------------------

static time_t time_t_from_FAT(u32 fat_timedate)
{
	const u32 fat_time = bits(fat_timedate, 0, 15);
	const u32 fat_date = bits(fat_timedate, 16, 31);

	struct tm t;							// struct tm format:
	t.tm_sec   = bits(fat_time, 0,4) * 2;	// [0,59]
	t.tm_min   = bits(fat_time, 5,10);		// [0,59]
	t.tm_hour  = bits(fat_time, 11,15);		// [0,23]
	t.tm_mday  = bits(fat_date, 0,4);		// [1,31]
	t.tm_mon   = bits(fat_date, 5,8) - 1;	// [0,11]
	t.tm_year  = bits(fat_date, 9,15) + 80;	// since 1900
	t.tm_isdst = -1;	// unknown - let libc determine

	// otherwise: totally bogus, and at the limit of 32-bit time_t
	ENSURE(t.tm_year < 138);

	time_t ret = mktime(&t);
	ENSURE(ret != (time_t)-1);	// mktime shouldn't fail
	return ret;
}


static u32 FAT_from_time_t(time_t time)
{
	// (values are adjusted for DST)
	struct tm* t = localtime(&time);

	const u16 fat_time = u16(
		(t->tm_sec/2) |		    // 5
		(u16(t->tm_min) << 5) | // 6
		(u16(t->tm_hour) << 11)	// 5
		);

	const u16 fat_date = u16(
		(t->tm_mday) |            // 5
		(u16(t->tm_mon+1) << 5) | // 4
		(u16(t->tm_year-80) << 9) // 7
		);

	u32 fat_timedate = u32_from_u16(fat_date, fat_time);
	return fat_timedate;
}


//-----------------------------------------------------------------------------
// Zip archive definitions
//-----------------------------------------------------------------------------

static const u32 cdfh_magic = FOURCC_LE('P','K','\1','\2');
static const u32  lfh_magic = FOURCC_LE('P','K','\3','\4');
static const u32 ecdr_magic = FOURCC_LE('P','K','\5','\6');

enum ZipMethod
{
	ZIP_METHOD_NONE    = 0,
	ZIP_METHOD_DEFLATE = 8
};

#pragma pack(push, 1)

class LFH
{
public:
	void Init(const CFileInfo& fileInfo, off_t csize, ZipMethod method, u32 checksum, const Path& pathname)
	{
		const std::string pathnameUTF8 = utf8_from_wstring(pathname.string());
		const size_t pathnameSize = pathnameUTF8.length();

		m_magic     = lfh_magic;
		m_x1        = to_le16(0);
		m_flags     = to_le16(0);
		m_method    = to_le16(u16_from_larger(method));
		m_fat_mtime = to_le32(FAT_from_time_t(fileInfo.MTime()));
		m_crc       = to_le32(checksum);
		m_csize     = to_le32(u32_from_larger(csize));
		m_usize     = to_le32(u32_from_larger(fileInfo.Size()));
		m_fn_len    = to_le16(u16_from_larger(pathnameSize));
		m_e_len     = to_le16(0);

		memcpy((char*)this + sizeof(LFH), pathnameUTF8.c_str(), pathnameSize);
	}

	size_t Size() const
	{
		ENSURE(m_magic == lfh_magic);
		size_t size = sizeof(LFH);
		size += read_le16(&m_fn_len);
		size += read_le16(&m_e_len);
		// note: LFH doesn't have a comment field!
		return size;
	}

private:
	u32 m_magic;
	u16 m_x1;			// version needed
	u16 m_flags;
	u16 m_method;
	u32 m_fat_mtime;	// last modified time (DOS FAT format)
	u32 m_crc;
	u32 m_csize;
	u32 m_usize;
	u16 m_fn_len;
	u16 m_e_len;
};

cassert(sizeof(LFH) == 30);


class CDFH
{
public:
	void Init(const CFileInfo& fileInfo, off_t ofs, off_t csize, ZipMethod method, u32 checksum, const Path& pathname, size_t slack)
	{
		const std::string pathnameUTF8 = utf8_from_wstring(pathname.string());
		const size_t pathnameLength = pathnameUTF8.length();

		m_magic     = cdfh_magic;
		m_x1        = to_le32(0);
		m_flags     = to_le16(0);
		m_method    = to_le16(u16_from_larger(method));
		m_fat_mtime = to_le32(FAT_from_time_t(fileInfo.MTime()));
		m_crc       = to_le32(checksum);
		m_csize     = to_le32(u32_from_larger(csize));
		m_usize     = to_le32(u32_from_larger(fileInfo.Size()));
		m_fn_len    = to_le16(u16_from_larger(pathnameLength));
		m_e_len     = to_le16(0);
		m_c_len     = to_le16(u16_from_larger((size_t)slack));
		m_x2        = to_le32(0);
		m_x3        = to_le32(0);
		m_lfh_ofs   = to_le32(u32_from_larger(ofs));

		memcpy((char*)this + sizeof(CDFH), pathnameUTF8.c_str(), pathnameLength);
	}

	Path Pathname() const
	{
		const size_t length = (size_t)read_le16(&m_fn_len);
		const char* pathname = (const char*)this + sizeof(CDFH); // not 0-terminated!
		return Path(std::string(pathname, length));
	}

	off_t HeaderOffset() const
	{
		return read_le32(&m_lfh_ofs);
	}

	off_t USize() const
	{
		return (off_t)read_le32(&m_usize);
	}

	off_t CSize() const
	{
		return (off_t)read_le32(&m_csize);
	}

	ZipMethod Method() const
	{
		return (ZipMethod)read_le16(&m_method);
	}

	u32 Checksum() const
	{
		return read_le32(&m_crc);
	}

	time_t MTime() const
	{
		const u32 fat_mtime = read_le32(&m_fat_mtime);
		return time_t_from_FAT(fat_mtime);
	}

	size_t Size() const
	{
		size_t size = sizeof(CDFH);
		size += read_le16(&m_fn_len);
		size += read_le16(&m_e_len);
		size += read_le16(&m_c_len);
		return size;
	}

private:
	u32 m_magic;
	u32 m_x1;			// versions
	u16 m_flags;
	u16 m_method;
	u32 m_fat_mtime;	// last modified time (DOS FAT format)
	u32 m_crc;
	u32 m_csize;
	u32 m_usize;
	u16 m_fn_len;
	u16 m_e_len;
	u16 m_c_len;
	u32 m_x2;			// spanning
	u32 m_x3;			// attributes
	u32 m_lfh_ofs;
};

cassert(sizeof(CDFH) == 46);


class ECDR
{
public:
	void Init(size_t cd_numEntries, off_t cd_ofs, size_t cd_size)
	{
		m_magic         = ecdr_magic;
		m_diskNum       = to_le16(0);
		m_cd_diskNum    = to_le16(0);
		m_cd_numEntriesOnDisk = to_le16(u16_from_larger(cd_numEntries));
		m_cd_numEntries = m_cd_numEntriesOnDisk;
		m_cd_size       = to_le32(u32_from_larger(cd_size));
		m_cd_ofs        = to_le32(u32_from_larger(cd_ofs));
		m_comment_len   = to_le16(0);
	}

	void Decompose(size_t& cd_numEntries, off_t& cd_ofs, size_t& cd_size) const
	{
		cd_numEntries = (size_t)read_le16(&m_cd_numEntries);
		cd_ofs       = (off_t)read_le32(&m_cd_ofs);
		cd_size      = (size_t)read_le32(&m_cd_size);
	}

private:
	u32 m_magic;
	u16 m_diskNum;
	u16 m_cd_diskNum;
	u16 m_cd_numEntriesOnDisk;
	u16 m_cd_numEntries;
	u32 m_cd_size;
	u32 m_cd_ofs;
	u16 m_comment_len;
};

cassert(sizeof(ECDR) == 22);

#pragma pack(pop)


//-----------------------------------------------------------------------------
// ArchiveFile_Zip
//-----------------------------------------------------------------------------

class ArchiveFile_Zip : public IArchiveFile
{
public:
	ArchiveFile_Zip(const PFile& file, off_t ofs, off_t csize, u32 checksum, ZipMethod method)
		: m_file(file), m_ofs(ofs)
		, m_csize(csize), m_checksum(checksum), m_method((u16)method)
		, m_flags(NeedsFixup)
	{
	}

	virtual size_t Precedence() const
	{
		return 2u;
	}

	virtual wchar_t LocationCode() const
	{
		return 'A';
	}

	virtual OsPath Path() const
	{
		return m_file->Pathname();
	}

	virtual Status Load(const OsPath& UNUSED(name), const shared_ptr<u8>& buf, size_t size) const
	{
		AdjustOffset();

		PICodec codec;
		switch(m_method)
		{
		case ZIP_METHOD_NONE:
			codec = CreateCodec_ZLibNone();
			break;
		case ZIP_METHOD_DEFLATE:
			codec = CreateDecompressor_ZLibDeflate();
			break;
		default:
			WARN_RETURN(ERR::ARCHIVE_UNKNOWN_METHOD);
		}

		Stream stream(codec);
		stream.SetOutputBuffer(buf.get(), size);
		io::Operation op(*m_file.get(), 0, m_csize, m_ofs);
		StreamFeeder streamFeeder(stream);
		RETURN_STATUS_IF_ERR(io::Run(op, io::Parameters(), streamFeeder));
		RETURN_STATUS_IF_ERR(stream.Finish());
#if CODEC_COMPUTE_CHECKSUM
		ENSURE(m_checksum == stream.Checksum());
#endif

		return INFO::OK;
	}

private:
	enum Flags
	{
		// indicates m_ofs points to a "local file header" instead of
		// the file data. a fixup routine is called when reading the file;
		// it skips past the LFH and clears this flag.
		// this is somewhat of a hack, but vital to archive open performance.
		// without it, we'd have to scan through the entire archive file,
		// which can take *seconds*.
		// (we cannot use the information in CDFH, because its 'extra' field
		// has been observed to differ from that of the LFH)
		// since we read the LFH right before the rest of the file, the block
		// cache will absorb the IO cost.
		NeedsFixup = 1
	};

	struct LFH_Copier
	{
		LFH_Copier(u8* lfh_dst, size_t lfh_bytes_remaining)
			: lfh_dst(lfh_dst), lfh_bytes_remaining(lfh_bytes_remaining)
		{
		}

		// this code grabs an LFH struct from file block(s) that are
		// passed to the callback. usually, one call copies the whole thing,
		// but the LFH may straddle a block boundary.
		//
		// rationale: this allows using temp buffers for zip_fixup_lfh,
		// which avoids involving the file buffer manager and thus
		// avoids cluttering the trace and cache contents.
		Status operator()(const u8* block, size_t size) const
		{
			ENSURE(size <= lfh_bytes_remaining);
			memcpy(lfh_dst, block, size);
			lfh_dst += size;
			lfh_bytes_remaining -= size;

			return INFO::OK;
		}

		mutable u8* lfh_dst;
		mutable size_t lfh_bytes_remaining;
	};

	/**
	 * fix up m_ofs (adjust it to point to cdata instead of the LFH).
	 *
	 * note: we cannot use CDFH filename and extra field lengths to skip
	 * past LFH since that may not mirror CDFH (has happened).
	 *
	 * this is called at file-open time instead of while mounting to
	 * reduce seeks: since reading the file will typically follow, the
	 * block cache entirely absorbs the IO cost.
	 **/
	void AdjustOffset() const
	{
		if(!(m_flags & NeedsFixup))
			return;
		m_flags &= ~NeedsFixup;

		// performance note: this ends up reading one file block, which is
		// only in the block cache if the file starts in the same block as a
		// previously read file (i.e. both are small).
		LFH lfh;
		io::Operation op(*m_file.get(), 0, sizeof(LFH), m_ofs);
		if(io::Run(op, io::Parameters(), LFH_Copier((u8*)&lfh, sizeof(LFH))) == INFO::OK)
			m_ofs += (off_t)lfh.Size();
	}

	PFile m_file;

	// all relevant LFH/CDFH fields not covered by CFileInfo
	mutable off_t m_ofs;
	off_t m_csize;
	u32 m_checksum;
	u16 m_method;
	mutable u16 m_flags;
};


//-----------------------------------------------------------------------------
// ArchiveReader_Zip
//-----------------------------------------------------------------------------

class ArchiveReader_Zip : public IArchiveReader
{
public:
	ArchiveReader_Zip(const OsPath& pathname)
		: m_file(new File(pathname, O_RDONLY))
	{
		CFileInfo fileInfo;
		GetFileInfo(pathname, &fileInfo);
		m_fileSize = fileInfo.Size();
		const size_t minFileSize = sizeof(LFH)+sizeof(CDFH)+sizeof(ECDR);
		ENSURE(m_fileSize >= off_t(minFileSize));
	}

	virtual Status ReadEntries(ArchiveEntryCallback cb, uintptr_t cbData)
	{
		// locate and read Central Directory
		off_t cd_ofs = 0;
		size_t cd_numEntries = 0;
		size_t cd_size = 0;
		RETURN_STATUS_IF_ERR(LocateCentralDirectory(m_file, m_fileSize, cd_ofs, cd_numEntries, cd_size));
		UniqueRange buf(io::Allocate(cd_size));

		io::Operation op(*m_file.get(), buf.get(), cd_size, cd_ofs);
		RETURN_STATUS_IF_ERR(io::Run(op));

		// iterate over Central Directory
		const u8* pos = (const u8*)buf.get();
		for(size_t i = 0; i < cd_numEntries; i++)
		{
			// scan for next CDFH
			CDFH* cdfh = (CDFH*)FindRecord((const u8*)buf.get(), cd_size, pos, cdfh_magic, sizeof(CDFH));
			if(!cdfh)
				WARN_RETURN(ERR::CORRUPTED);

			const Path relativePathname(cdfh->Pathname());
			if(!relativePathname.IsDirectory())
			{
				const OsPath name = relativePathname.Filename();
				CFileInfo fileInfo(name, cdfh->USize(), cdfh->MTime());
				shared_ptr<ArchiveFile_Zip> archiveFile(new ArchiveFile_Zip(m_file, cdfh->HeaderOffset(), cdfh->CSize(), cdfh->Checksum(), cdfh->Method()));
				cb(relativePathname, fileInfo, archiveFile, cbData);
			}

			pos += cdfh->Size();
		}

		return INFO::OK;
	}

private:
	/**
	 * Scan buffer for a Zip file record.
	 *
	 * @param buf
	 * @param size
	 * @param start position within buffer
	 * @param magic signature of record
	 * @param recordSize size of record (including signature)
	 * @return pointer to record within buffer or 0 if not found.
	 **/
	static const u8* FindRecord(const u8* buf, size_t size, const u8* start, u32 magic, size_t recordSize)
	{
		// (don't use <start> as the counter - otherwise we can't tell if
		// scanning within the buffer was necessary.)
		for(const u8* p = start; p <= buf+size-recordSize; p++)
		{
			// found it
			if(*(u32*)p == magic)
			{
				ENSURE(p == start);	// otherwise, the archive is a bit broken
				return p;
			}
		}

		// passed EOF, didn't find it.
		// note: do not warn - this happens in the initial ECDR search at
		// EOF if the archive contains a comment field.
		return 0;
	}

	// search for ECDR in the last <maxScanSize> bytes of the file.
	// if found, fill <dst_ecdr> with a copy of the (little-endian) ECDR and
	// return INFO::OK, otherwise IO error or ERR::CORRUPTED.
	static Status ScanForEcdr(const PFile& file, off_t fileSize, u8* buf, size_t maxScanSize, size_t& cd_numEntries, off_t& cd_ofs, size_t& cd_size)
	{
		// don't scan more than the entire file
		const size_t scanSize = std::min(maxScanSize, size_t(fileSize));

		// read desired chunk of file into memory
		const off_t ofs = fileSize - off_t(scanSize);
		io::Operation op(*file.get(), buf, scanSize, ofs);
		RETURN_STATUS_IF_ERR(io::Run(op));

		// look for ECDR in buffer
		const ECDR* ecdr = (const ECDR*)FindRecord(buf, scanSize, buf, ecdr_magic, sizeof(ECDR));
		if(!ecdr)
			return INFO::CANNOT_HANDLE;

		ecdr->Decompose(cd_numEntries, cd_ofs, cd_size);
		return INFO::OK;
	}

	static Status LocateCentralDirectory(const PFile& file, off_t fileSize, off_t& cd_ofs, size_t& cd_numEntries, size_t& cd_size)
	{
		const size_t maxScanSize = 66000u;	// see below
		UniqueRange buf(io::Allocate(maxScanSize));

		// expected case: ECDR at EOF; no file comment
		Status ret = ScanForEcdr(file, fileSize, (u8*)buf.get(), sizeof(ECDR), cd_numEntries, cd_ofs, cd_size);
		if(ret == INFO::OK)
			return INFO::OK;
		// worst case: ECDR precedes 64 KiB of file comment
		ret = ScanForEcdr(file, fileSize, (u8*)buf.get(), maxScanSize, cd_numEntries, cd_ofs, cd_size);
		if(ret == INFO::OK)
			return INFO::OK;

		// both ECDR scans failed - this is not a valid Zip file.
		io::Operation op(*file.get(), buf.get(), sizeof(LFH));
		RETURN_STATUS_IF_ERR(io::Run(op));
		// the Zip file has an LFH but lacks an ECDR. this can happen if
		// the user hard-exits while an archive is being written.
		// notes:
		// - return ERR::CORRUPTED so VFS will not include this file.
		// - we could work around this by scanning all LFHs, but won't bother
		//   because it'd be slow.
		// - do not warn - the corrupt archive will be deleted on next
		//   successful archive builder run anyway.
		if(FindRecord((const u8*)buf.get(), sizeof(LFH), (const u8*)buf.get(), lfh_magic, sizeof(LFH)))
			return ERR::CORRUPTED;	// NOWARN
		// totally bogus
		else
			WARN_RETURN(ERR::ARCHIVE_UNKNOWN_FORMAT);
	}

	PFile m_file;
	off_t m_fileSize;
};

PIArchiveReader CreateArchiveReader_Zip(const OsPath& archivePathname)
{
	try
	{
		return PIArchiveReader(new ArchiveReader_Zip(archivePathname));
	}
	catch(Status)
	{
		return PIArchiveReader();
	}
}


//-----------------------------------------------------------------------------
// ArchiveWriter_Zip
//-----------------------------------------------------------------------------

class ArchiveWriter_Zip : public IArchiveWriter
{
public:
	ArchiveWriter_Zip(const OsPath& archivePathname, bool noDeflate)
		: m_file(new File(archivePathname, O_WRONLY)), m_fileSize(0)
		, m_numEntries(0), m_noDeflate(noDeflate)
	{
		THROW_STATUS_IF_ERR(pool_create(&m_cdfhPool, 10*MiB, 0));
	}

	~ArchiveWriter_Zip()
	{
		// append an ECDR to the CDFH list (this allows us to
		// write out both to the archive file in one burst)
		const size_t cd_size = m_cdfhPool.da.pos;
		ECDR* ecdr = (ECDR*)pool_alloc(&m_cdfhPool, sizeof(ECDR));
		if(!ecdr)
			std::terminate();
		const off_t cd_ofs = m_fileSize;
		ecdr->Init(m_numEntries, cd_ofs, cd_size);

		if(write(m_file->Descriptor(), m_cdfhPool.da.base, cd_size+sizeof(ECDR)) < 0)
			DEBUG_WARN_ERR(ERR::IO);	// no way to return error code

		(void)pool_destroy(&m_cdfhPool);
	}

	Status AddFile(const OsPath& pathname, const OsPath& pathnameInArchive)
	{
		CFileInfo fileInfo;
		RETURN_STATUS_IF_ERR(GetFileInfo(pathname, &fileInfo));

		PFile file(new File);
		RETURN_STATUS_IF_ERR(file->Open(pathname, O_RDONLY));

		return AddFileOrMemory(fileInfo, pathnameInArchive, file, NULL);
	}

	Status AddMemory(const u8* data, size_t size, time_t mtime, const OsPath& pathnameInArchive)
	{
		CFileInfo fileInfo(pathnameInArchive, size, mtime);

		return AddFileOrMemory(fileInfo, pathnameInArchive, PFile(), data);
	}

	Status AddFileOrMemory(const CFileInfo& fileInfo, const OsPath& pathnameInArchive, const PFile& file, const u8* data)
	{
		ENSURE((file && !data) || (data && !file));

		const off_t usize = fileInfo.Size();
		// skip 0-length files.
		// rationale: zip.cpp needs to determine whether a CDFH entry is
		// a file or directory (the latter are written by some programs but
		// not needed - they'd only pollute the file table).
		// it looks like checking for usize=csize=0 is the safest way -
		// relying on file attributes (which are system-dependent!) is
		// even less safe.
		// we thus skip 0-length files to avoid confusing them with directories.
		if(!usize)
			return INFO::SKIPPED;

		const size_t pathnameLength = pathnameInArchive.string().length();

		// choose method and the corresponding codec
		ZipMethod method;
		PICodec codec;
		if(m_noDeflate || IsFileTypeIncompressible(pathnameInArchive))
		{
			method = ZIP_METHOD_NONE;
			codec = CreateCodec_ZLibNone();
		}
		else
		{
			method = ZIP_METHOD_DEFLATE;
			codec = CreateCompressor_ZLibDeflate();
		}

		// allocate memory
		const size_t csizeMax = codec->MaxOutputSize(size_t(usize));
		UniqueRange buf(io::Allocate(sizeof(LFH) + pathnameLength + csizeMax));

		// read and compress file contents
		size_t csize; u32 checksum;
		{
			u8* cdata = (u8*)buf.get() + sizeof(LFH) + pathnameLength;
			Stream stream(codec);
			stream.SetOutputBuffer(cdata, csizeMax);
			StreamFeeder streamFeeder(stream);
			if(file)
			{
				io::Operation op(*file.get(), 0, usize);
				RETURN_STATUS_IF_ERR(io::Run(op, io::Parameters(), streamFeeder));
			}
			else
			{
				RETURN_STATUS_IF_ERR(streamFeeder(data, usize));
			}
			RETURN_STATUS_IF_ERR(stream.Finish());
			csize = stream.OutSize();
			checksum = stream.Checksum();
		}

		// build LFH
		{
			LFH* lfh = (LFH*)buf.get();
			lfh->Init(fileInfo, (off_t)csize, method, checksum, pathnameInArchive);
		}

		// append a CDFH to the central directory (in memory)
		const off_t ofs = m_fileSize;
		const size_t prev_pos = m_cdfhPool.da.pos;	// (required to determine padding size)
		const size_t cdfhSize = sizeof(CDFH) + pathnameLength;
		CDFH* cdfh = (CDFH*)pool_alloc(&m_cdfhPool, cdfhSize);
		if(!cdfh)
			WARN_RETURN(ERR::NO_MEM);
		const size_t slack = m_cdfhPool.da.pos - prev_pos - cdfhSize;
		cdfh->Init(fileInfo, ofs, (off_t)csize, method, checksum, pathnameInArchive, slack);
		m_numEntries++;

		// write LFH, pathname and cdata to file
		const size_t packageSize = sizeof(LFH) + pathnameLength + csize;
		if(write(m_file->Descriptor(), buf.get(), packageSize) < 0)
			WARN_RETURN(ERR::IO);
		m_fileSize += (off_t)packageSize;

		return INFO::OK;
	}

private:
	static bool IsFileTypeIncompressible(const OsPath& pathname)
	{
		const OsPath extension = pathname.Extension();

		// file extensions that we don't want to compress
		static const wchar_t* incompressibleExtensions[] =
		{
			L".zip", L".rar",
			L".jpg", L".jpeg", L".png",
			L".ogg", L".mp3"
		};

		for(size_t i = 0; i < ARRAY_SIZE(incompressibleExtensions); i++)
		{
			if(extension == incompressibleExtensions[i])
				return true;
		}

		return false;
	}

	PFile m_file;
	off_t m_fileSize;

	Pool m_cdfhPool;
	size_t m_numEntries;

	bool m_noDeflate;
};

PIArchiveWriter CreateArchiveWriter_Zip(const OsPath& archivePathname, bool noDeflate)
{
	try
	{
		return PIArchiveWriter(new ArchiveWriter_Zip(archivePathname, noDeflate));
	}
	catch(Status)
	{
		return PIArchiveWriter();
	}
}
