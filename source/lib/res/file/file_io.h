/**
 * =========================================================================
 * File        : file_io.h
 * Project     : 0 A.D.
 * Description : provide fast I/O via POSIX aio and splitting into blocks.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FILE_IO
#define INCLUDED_FILE_IO

struct FileProvider_VTbl;
struct File;


namespace ERR
{
	const LibError IO     = -110100;
	const LibError IO_EOF = -110101;
}


extern void file_io_init();
extern void file_io_shutdown();


//
// asynchronous IO
//

// this is a thin wrapper on top of the system AIO calls.
// IOs are carried out exactly as requested - there is no caching or
// alignment done here. rationale: see source.

// again chosen for nice alignment; each user checks if big enough.
const size_t FILE_IO_OPAQUE_SIZE = 28;

struct FileIo
{
	const FileProvider_VTbl* type;
	u8 opaque[FILE_IO_OPAQUE_SIZE];
};

// queue the IO; it begins after the previous ones (if any) complete.
//
// rationale: this interface is more convenient than implicitly advancing a
// file pointer because archive.cpp often accesses random offsets.
extern LibError file_io_issue(File* f, off_t ofs, size_t size, u8* buf, FileIo* io);

// indicates if the given IO has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
extern int file_io_has_completed(FileIo* io);

// wait for the given IO to complete. passes back its buffer and size.
extern LibError file_io_wait(FileIo* io, u8*& p, size_t& size);

// indicates the IO's buffer is no longer needed and frees that memory.
extern LibError file_io_discard(FileIo* io);

extern LibError file_io_validate(const FileIo* io);


//
// synchronous IO
//

extern size_t file_sector_size;

// called by file_io after a block IO has completed.
// *bytes_processed must be set; file_io will return the sum of these values.
// example: when reading compressed data and decompressing in the callback,
// indicate #bytes decompressed.
// return value: INFO::CB_CONTINUE to continue calling; anything else:
//   abort immediately and return that.
// note: in situations where the entire IO is not split into blocks
// (e.g. when reading from cache or not using AIO), this is still called but
// for the entire IO. we do not split into fake blocks because it is
// advantageous (e.g. for decompressors) to have all data at once, if available
// anyway.
typedef LibError (*FileIOCB)(uintptr_t ctx, const u8* block, size_t size, size_t* bytes_processed);


typedef const u8* FileIOBuf;

FileIOBuf* const FILE_BUF_TEMP = (FileIOBuf*)1;
const FileIOBuf FILE_BUF_ALLOC = (FileIOBuf)2;


enum FileBufFlags
{
	// indicates the buffer will not be freed immediately
	// (i.e. before the next buffer alloc) as it normally should.
	// this flag serves to suppress a warning and better avoid fragmentation.
	// caller sets this when FILE_LONG_LIVED is specified.
	//
	// also used by file_cache_retrieve because it may have to
	// 'reactivate' the buffer (transfer from cache to extant list),
	// which requires knowing whether the buffer is long-lived or not.
	FB_LONG_LIVED    = 1,

	// statistics (e.g. # buffer allocs) should not be updated.
	// (useful for simulation, e.g. trace_entry_causes_io)
	FB_NO_STATS      = 2,

	// file_cache_retrieve should not update item credit.
	// (useful when just looking up buffer given atom_fn)
	FB_NO_ACCOUNTING = 4,

	// memory will be allocated from the heap, not the (limited) file cache.
	// this makes sense for write buffers that are never used again,
	// because we avoid having to displace some other cached items.
	FB_FROM_HEAP     = 8
};

// allocate a new buffer of <size> bytes (possibly more due to internal
// fragmentation). never returns 0.
// <atom_fn>: owner filename (buffer is intended to be used for data from
//   this file).
extern FileIOBuf file_buf_alloc(size_t size, const char* atom_fn, uint fb_flags = 0);

// mark <buf> as no longer needed. if its reference count drops to 0,
// it will be removed from the extant list. if it had been added to the
// cache, it remains there until evicted in favor of another buffer.
extern LibError file_buf_free(FileIOBuf buf, uint fb_flags = 0);


// transfer <size> bytes, starting at <ofs>, to/from the given file.
// (read or write access was chosen at file-open time).
//
// if non-NULL, <cb> is called for each block transferred, passing <ctx>.
// it returns how much data was actually transferred, or a negative error
// code (in which case we abort the transfer and return that value).
// the callback mechanism is useful for user progress notification or
// processing data while waiting for the next I/O to complete
// (quasi-parallel, without the complexity of threads).
//
// return number of bytes transferred (see above), or a negative error code.
extern ssize_t file_io(File* f, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb = 0, uintptr_t ctx = 0);

extern ssize_t file_read_from_cache(const char* atom_fn, off_t ofs, size_t size,
	FileIOBuf* pbuf, FileIOCB cb, uintptr_t ctx);


extern LibError file_io_get_buf(FileIOBuf* pbuf, size_t size,
	const char* atom_fn, uint file_flags, FileIOCB cb);

#endif	// #ifndef INCLUDED_FILE_IO
