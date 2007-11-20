/**
 * =========================================================================
 * File        : io_posix.h
 * Project     : 0 A.D.
 * Description : lightweight POSIX aio wrapper
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_IO_POSIX
#define INCLUDED_IO_POSIX

#include "../io/io_buf.h"

// rationale for using aio instead of mmap:
// - parallelism: instead of just waiting for the transfer to complete,
//   other work can be done in the meantime.
//   example: decompressing from a Zip archive is practically free because
//   we inflate one block while reading the next.
// - throughput: with aio, the drive always has something to do, as opposed
//   to read requests triggered by the OS for mapped files, which come
//   in smaller chunks. this leads to much higher transfer rates.
// - memory: when used with VFS, aio makes better use of a file cache.
//   data is generally compressed in an archive. a cache should store the
//   decompressed and decoded (e.g. BGR->RGB) data; mmap would keep the
//   original compressed data in memory, which doesn't help.
//   we bypass the OS file cache via aio; high-level code will take care
//   of caching the decoded file contents.


// however, aio is only used internally in file_io. this simplifies the
// interface by preventing the need for multiple implementations (archive,
// vfs, etc.) and avoiding the need for automatic free (since aio won't
// be used directly by client code).
// this also affects streaming of sounds, which is currently indeed
// implemented via aio from archives. however, it doesn't appear to be used
// (even music files are loaded at once) and really ought to be done via
// thread, so we could disable it.



// we don't do any caching or alignment here - this is just a thin AIO wrapper.
// rationale:
// - aligning the transfer isn't possible here since we have no control
//   over the buffer, i.e. we cannot read more data than requested.
//   instead, this is done in io_manager.
// - transfer sizes here are arbitrary (i.e. not block-aligned);
//   that means the cache would have to handle this or also split them up
//   into blocks, which would duplicate the abovementioned work.
// - if caching here, we'd also have to handle "forwarding" (i.e.
//   desired block has been issued but isn't yet complete). again, it
//   is easier to let the synchronous io_manager handle this.
// - finally, io_manager knows more about whether the block should be cached
//   (e.g. whether another block request will follow), but we don't
//   currently make use of this.
//
// disadvantages:
// - streamed data will always be read from disk. no problem, because
//   such data (e.g. music, long speech) is unlikely to be used again soon.
// - prefetching (issuing the next few blocks from archive/file during
//   idle time to satisfy potential future IOs) requires extra buffers;
//   this is a bit more complicated than just using the cache as storage.

namespace ERR
{
	const LibError FILE_ACCESS = -110200;
	const LibError IO          = -110201;
	const LibError IO_EOF      = -110202;
}

namespace INFO
{
	const LibError IO_PENDING  = +110203;
	const LibError IO_COMPLETE = +110204;
}


class File_Posix
{
	friend class Io_Posix;

public:
	File_Posix();
	~File_Posix();

	LibError Open(const char* pathname, char mode);
	void Close();

	const char* Pathname() const
	{
		return m_pathname;
	}

	char Mode() const
	{
		return m_mode;
	}

	int Handle() const
	{
		return m_fd;
	}

	LibError Validate() const;

private:
	const char* m_pathname;
	char m_mode;
	int m_fd;
};


class Io_Posix
{
public:
	Io_Posix();
	~Io_Posix();

	// no attempt is made at aligning or padding the transfer.
	LibError Issue(File_Posix& file, off_t ofs, IoBuf buf, size_t size);

	// check if the IO has completed.
	LibError Status() const;

	// passes back the buffer and its size.
	LibError WaitUntilComplete(IoBuf& buf, size_t& size);

private:
	class Impl;
	boost::shared_ptr<Impl> impl;
};


extern LibError io_posix_Synchronous(File_Posix& file, off_t ofs, IoBuf buf, size_t size);

#endif	// #ifndef INCLUDED_IO_POSIX
