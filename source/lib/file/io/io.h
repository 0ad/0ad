/**
 * =========================================================================
 * File        : io.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_IO
#define INCLUDED_IO

#include "lib/file/path.h"

namespace ERR
{
	const LibError FILE_ACCESS = -110200;
	const LibError IO          = -110201;
	const LibError IO_EOF      = -110202;
}


/**
 * simple POSIX file wrapper.
 **/
class LIB_API File
{
public:
	File();
	~File();

	LibError Open(const Path& pathname, char mode);
	void Close();

	const Path& Pathname() const
	{
		return m_pathname;
	}

	char Mode() const
	{
		return m_mode;
	}

	LibError Issue(aiocb& req, off_t alignedOfs, u8* alignedBuf, size_t alignedSize) const;
	static LibError WaitUntilComplete(aiocb& req, u8*& alignedBuf, size_t& alignedSize);

	LibError Read(off_t ofs, u8* buf, size_t size) const;
	LibError Write(off_t ofs, const u8* buf, size_t size) const;

private:
	LibError IO(off_t ofs, u8* buf, size_t size) const;

	Path m_pathname;
	char m_mode;
	int m_fd;
};

typedef shared_ptr<File> PFile;


// memory will be allocated from the heap, not the (limited) file cache.
// this makes sense for write buffers that are never used again,
// because we avoid having to displace some other cached items.
shared_ptr<u8> io_Allocate(size_t size, off_t ofs = 0);


/**
 * called after a block IO has completed.
 *
 * @return INFO::CB_CONTINUE to continue; any other value will cause the
 * IO splitter to abort immediately and return that.
 *
 * this is useful for interleaving e.g. decompression with IOs.
 **/
typedef LibError (*IoCallback)(uintptr_t cbData, const u8* block, size_t blockSize);

LIB_API LibError io_Scan(const File& file, off_t ofs, off_t size, IoCallback cb, uintptr_t cbData);

LIB_API LibError io_Read(const File& file, off_t ofs, u8* alignedBuf, size_t size, u8*& data);

LIB_API LibError io_WriteAligned(const File& file, off_t alignedOfs, const u8* alignedData, size_t size);
LIB_API LibError io_ReadAligned(const File& file, off_t alignedOfs, u8* alignedBuf, size_t size);

class LIB_API UnalignedWriter
{
public:
	UnalignedWriter(const File& file, off_t ofs);

	/**
	 * add data to the align buffer, writing it out to disk if full.
	 **/
	LibError Append(const u8* data, size_t size) const;

	/**
	 * zero-initialize any remaining space in the align buffer and write
	 * it to the file. this is called by the destructor.
	 **/
	void Flush() const;

private:
	class Impl;
	shared_ptr<Impl> impl;
};

#endif	// #ifndef INCLUDED_IO
