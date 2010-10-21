/* Copyright (c) 2010 Wildfire Games
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

#include "lib/bits.h"
#include "lib/byte_order.h"
#include "lib/utf8.h"	// wstring_from_utf8
#include "lib/fat_time.h"
#include "lib/path_util.h"
#include "lib/allocators/pool.h"
#include "lib/sysdep/cpu.h"		// cpu_memcpy
#include "lib/file/archive/archive.h"
#include "lib/file/archive/codec_zlib.h"
#include "lib/file/archive/stream.h"
#include "lib/file/file.h"
#include "lib/file/io/io.h"
#include "lib/file/io/io_align.h"	// BLOCK_SIZE
#include "lib/file/io/write_buffer.h"


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
	void Init(const FileInfo& fileInfo, off_t csize, ZipMethod method, u32 checksum, const fs::wpath& pathname)
	{
		const fs::path pathname_c = path_from_wpath(pathname);
		const size_t pathnameLength = pathname_c.string().length();

		m_magic     = lfh_magic;
		m_x1        = to_le16(0);
		m_flags     = to_le16(0);
		m_method    = to_le16(u16_from_larger(method));
		m_fat_mtime = to_le32(FAT_from_time_t(fileInfo.MTime()));
		m_crc       = to_le32(checksum);
		m_csize     = to_le32(u32_from_larger(csize));
		m_usize     = to_le32(u32_from_larger(fileInfo.Size()));
		m_fn_len    = to_le16(u16_from_larger(pathnameLength));
		m_e_len     = to_le16(0);

		cpu_memcpy((char*)this + sizeof(LFH), pathname_c.string().c_str(), pathnameLength);
	}

	size_t Size() const
	{
		debug_assert(m_magic == lfh_magic);
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
	void Init(const FileInfo& fileInfo, off_t ofs, off_t csize, ZipMethod method, u32 checksum, const fs::wpath& pathname, size_t slack)
	{
		const fs::path pathname_c = path_from_wpath(pathname);
		const size_t pathnameLength = pathname_c.string().length();

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

		cpu_memcpy((char*)this + sizeof(CDFH), pathname_c.string().c_str(), pathnameLength);
	}

	fs::wpath Pathname() const
	{
		const size_t length = (size_t)read_le16(&m_fn_len);
		const char* pathname = (const char*)this + sizeof(CDFH); // not 0-terminated!
		return wstring_from_utf8(std::string(pathname, length));
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

	virtual fs::wpath Path() const
	{
		return m_file->Pathname();
	}

	virtual LibError Load(const std::wstring& UNUSED(name), const shared_ptr<u8>& buf, size_t size) const
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
		RETURN_ERR(io_Scan(m_file, m_ofs, m_csize, FeedStream, (uintptr_t)&stream));
		RETURN_ERR(stream.Finish());
#if CODEC_COMPUTE_CHECKSUM
		debug_assert(m_checksum == stream.Checksum());
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
		u8* lfh_dst;
		size_t lfh_bytes_remaining;
	};

	// this code grabs an LFH struct from file block(s) that are
	// passed to the callback. usually, one call copies the whole thing,
	// but the LFH may straddle a block boundary.
	//
	// rationale: this allows using temp buffers for zip_fixup_lfh,
	// which avoids involving the file buffer manager and thus
	// avoids cluttering the trace and cache contents.
	static LibError lfh_copier_cb(uintptr_t cbData, const u8* block, size_t size)
	{
		LFH_Copier* p = (LFH_Copier*)cbData;

		debug_assert(size <= p->lfh_bytes_remaining);
		cpu_memcpy(p->lfh_dst, block, size);
		p->lfh_dst += size;
		p->lfh_bytes_remaining -= size;

		return INFO::CB_CONTINUE;
	}

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
		LFH_Copier params = { (u8*)&lfh, sizeof(LFH) };
		if(io_Scan(m_file, m_ofs, sizeof(LFH), lfh_copier_cb, (uintptr_t)&params) == INFO::OK)
			m_ofs += (off_t)lfh.Size();
	}

	PFile m_file;

	// all relevant LFH/CDFH fields not covered by FileInfo
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
	ArchiveReader_Zip(const fs::wpath& pathname)
		: m_file(new File(pathname, 'r'))
	{
		FileInfo fileInfo;
		GetFileInfo(pathname, &fileInfo);
		m_fileSize = fileInfo.Size();
		const size_t minFileSize = sizeof(LFH)+sizeof(CDFH)+sizeof(ECDR);
		debug_assert(m_fileSize >= off_t(minFileSize));
	}

	virtual LibError ReadEntries(ArchiveEntryCallback cb, uintptr_t cbData)
	{
		// locate and read Central Directory
		off_t cd_ofs = 0;
		size_t cd_numEntries = 0;
		size_t cd_size = 0;
		RETURN_ERR(LocateCentralDirectory(m_file, m_fileSize, cd_ofs, cd_numEntries, cd_size));
		shared_ptr<u8> buf = io_Allocate(cd_size, cd_ofs);
		u8* cd;
		RETURN_ERR(io_Read(m_file, cd_ofs, buf.get(), cd_size, cd));

		// iterate over Central Directory
		const u8* pos = cd;
		for(size_t i = 0; i < cd_numEntries; i++)
		{
			// scan for next CDFH
			CDFH* cdfh = (CDFH*)FindRecord(cd, cd_size, pos, cdfh_magic, sizeof(CDFH));
			if(!cdfh)
				WARN_RETURN(ERR::CORRUPTED);

			const VfsPath relativePathname(cdfh->Pathname().string());	// convert from fs::wpath
			const std::wstring name = relativePathname.leaf();
			if(name != L".")	// ignore directories (i.e. paths ending in slash)
			{
				FileInfo fileInfo(name, cdfh->USize(), cdfh->MTime());
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
				debug_assert(p == start);	// otherwise, the archive is a bit broken
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
	static LibError ScanForEcdr(const PFile& file, off_t fileSize, u8* buf, size_t maxScanSize, size_t& cd_numEntries, off_t& cd_ofs, size_t& cd_size)
	{
		// don't scan more than the entire file
		const size_t scanSize = std::min(maxScanSize, size_t(fileSize));

		// read desired chunk of file into memory
		const off_t ofs = fileSize - off_t(scanSize);
		u8* data;
		RETURN_ERR(io_Read(file, ofs, buf, scanSize, data));

		// look for ECDR in buffer
		const ECDR* ecdr = (const ECDR*)FindRecord(data, scanSize, data, ecdr_magic, sizeof(ECDR));
		if(!ecdr)
			return INFO::CANNOT_HANDLE;

		ecdr->Decompose(cd_numEntries, cd_ofs, cd_size);
		return INFO::OK;
	}

	static LibError LocateCentralDirectory(const PFile& file, off_t fileSize, off_t& cd_ofs, size_t& cd_numEntries, size_t& cd_size)
	{
		const size_t maxScanSize = 66000u;	// see below
		shared_ptr<u8> buf = io_Allocate(maxScanSize, BLOCK_SIZE-1);	// assume worst-case for alignment

		// expected case: ECDR at EOF; no file comment
		LibError ret = ScanForEcdr(file, fileSize, const_cast<u8*>(buf.get()), sizeof(ECDR), cd_numEntries, cd_ofs, cd_size);
		if(ret == INFO::OK)
			return INFO::OK;
		// worst case: ECDR precedes 64 KiB of file comment
		ret = ScanForEcdr(file, fileSize, const_cast<u8*>(buf.get()), maxScanSize, cd_numEntries, cd_ofs, cd_size);
		if(ret == INFO::OK)
			return INFO::OK;

		// both ECDR scans failed - this is not a valid Zip file.
		RETURN_ERR(io_ReadAligned(file, 0, const_cast<u8*>(buf.get()), sizeof(LFH)));
		// the Zip file has an LFH but lacks an ECDR. this can happen if
		// the user hard-exits while an archive is being written.
		// notes:
		// - return ERR::CORRUPTED so VFS will not include this file.
		// - we could work around this by scanning all LFHs, but won't bother
		//   because it'd be slow.
		// - do not warn - the corrupt archive will be deleted on next
		//   successful archive builder run anyway.
		if(FindRecord(buf.get(), sizeof(LFH), buf.get(), lfh_magic, sizeof(LFH)))
			return ERR::CORRUPTED;	// NOWARN
		// totally bogus
		else
			WARN_RETURN(ERR::ARCHIVE_UNKNOWN_FORMAT);
	}

	PFile m_file;
	off_t m_fileSize;
};

PIArchiveReader CreateArchiveReader_Zip(const fs::wpath& archivePathname)
{
	return PIArchiveReader(new ArchiveReader_Zip(archivePathname));
}


//-----------------------------------------------------------------------------
// ArchiveWriter_Zip
//-----------------------------------------------------------------------------

class ArchiveWriter_Zip : public IArchiveWriter
{
public:
	ArchiveWriter_Zip(const fs::wpath& archivePathname)
		: m_file(new File(archivePathname, 'w')), m_fileSize(0)
		, m_unalignedWriter(new UnalignedWriter(m_file, 0))
		, m_numEntries(0)
	{
		THROW_ERR(pool_create(&m_cdfhPool, 10*MiB, 0));
	}

	~ArchiveWriter_Zip()
	{
		// append an ECDR to the CDFH list (this allows us to
		// write out both to the archive file in one burst)
		const size_t cd_size = m_cdfhPool.da.pos;
		ECDR* ecdr = (ECDR*)pool_alloc(&m_cdfhPool, sizeof(ECDR));
		if(!ecdr)
			throw std::bad_alloc();
		const off_t cd_ofs = m_fileSize;
		ecdr->Init(m_numEntries, cd_ofs, cd_size);

		m_unalignedWriter->Append(m_cdfhPool.da.base, cd_size+sizeof(ECDR));
		m_unalignedWriter->Flush();
		m_unalignedWriter.reset();

		(void)pool_destroy(&m_cdfhPool);

		const fs::wpath pathname = m_file->Pathname();	// for truncate()
		m_file.reset();

		m_fileSize += off_t(cd_size+sizeof(ECDR));

		// remove padding added by UnalignedWriter
		wtruncate(pathname.string().c_str(), m_fileSize);
	}

	LibError AddFile(const fs::wpath& sourcepathname, const fs::wpath& pathname)
	{
		FileInfo fileInfo;
		RETURN_ERR(GetFileInfo(sourcepathname, &fileInfo));
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

		PFile file(new File);
		RETURN_ERR(file->Open(sourcepathname, 'r'));

		const size_t pathnameLength = pathname.string().length();

		// choose method and the corresponding codec
		ZipMethod method;
		PICodec codec;
		if(IsFileTypeIncompressible(pathname))
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
		shared_ptr<u8> buf = io_Allocate(sizeof(LFH) + pathnameLength + csizeMax);

		// read and compress file contents
		size_t csize; u32 checksum;
		{
			u8* cdata = (u8*)buf.get() + sizeof(LFH) + pathnameLength;
			Stream stream(codec);
			stream.SetOutputBuffer(cdata, csizeMax);
			RETURN_ERR(io_Scan(file, 0, usize, FeedStream, (uintptr_t)&stream));
			RETURN_ERR(stream.Finish());
			csize = stream.OutSize();
			checksum = stream.Checksum();
		}

		// build LFH
		{
			LFH* lfh = (LFH*)buf.get();
			lfh->Init(fileInfo, (off_t)csize, method, checksum, pathname);
		}

		// append a CDFH to the central directory (in memory)
		const off_t ofs = m_fileSize;
		const size_t prev_pos = m_cdfhPool.da.pos;	// (required to determine padding size)
		const size_t cdfhSize = sizeof(CDFH) + pathnameLength;
		CDFH* cdfh = (CDFH*)pool_alloc(&m_cdfhPool, cdfhSize);
		if(!cdfh)
			WARN_RETURN(ERR::NO_MEM);
		const size_t slack = m_cdfhPool.da.pos - prev_pos - cdfhSize;
		cdfh->Init(fileInfo, ofs, (off_t)csize, method, checksum, pathname, slack);
		m_numEntries++;

		// write LFH, pathname and cdata to file
		const size_t packageSize = sizeof(LFH) + pathnameLength + csize;
		RETURN_ERR(m_unalignedWriter->Append(buf.get(), packageSize));
		m_fileSize += (off_t)packageSize;

		return INFO::OK;
	}

private:
	static bool IsFileTypeIncompressible(const fs::wpath& pathname)
	{
		const std::wstring extension = fs::extension(pathname);

		// file extensions that we don't want to compress
		static const wchar_t* incompressibleExtensions[] =
		{
			L".zip", L".rar",
			L".jpg", L".jpeg", L".png", 
			L".ogg", L".mp3"
		};

		for(size_t i = 0; i < ARRAY_SIZE(incompressibleExtensions); i++)
		{
			if(!wcscasecmp(extension.c_str(), incompressibleExtensions[i]))
				return true;
		}

		return false;
	}

	PFile m_file;
	off_t m_fileSize;
	PUnalignedWriter m_unalignedWriter;

	Pool m_cdfhPool;
	size_t m_numEntries;
};

PIArchiveWriter CreateArchiveWriter_Zip(const fs::wpath& archivePathname)
{
	return PIArchiveWriter(new ArchiveWriter_Zip(archivePathname));
}
