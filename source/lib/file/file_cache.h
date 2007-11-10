/**
 * =========================================================================
 * File        : file_cache.h
 * Project     : 0 A.D.
 * Description : cache of file contents (supports zero-copy IO)
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FILE_CACHE
#define INCLUDED_FILE_CACHE

#include <boost/shared_ptr.hpp>
#include "io_buf.h"


/**
 * cache of file contents with support for zero-copy IO.
 * this works by reserving a region of the cache, using it as the IO buffer,
 * and returning the memory directly to users. optional write-protection
 * via MMU ensures that the shared contents aren't inadvertently changed.
 *
 * to ensure efficient operation and prevent fragmentation, only one
 * reference should be active at a time. in other words, read a file,
 * process it, and only then start reading the next file.
 *
 * rationale: this is very similar to BlockCache; however, the differences
 * (Reserve's size and MarkComplete's cost parameters and different eviction
 * policies) are enough to warrant separate implementations.
 **/
class FileCache
{
public:
	/**
	 * @param size maximum amount [bytes] of memory to use for the cache.
	 * (managed as a virtual memory region that's committed on-demand)
	 **/
	FileCache(size_t size);

	/**
	 * Allocate an IO buffer in the cache's memory region.
	 *
	 * @param atom_fn pathname of the file that is to be read; this is
	 * the key that will be used to Retrieve the file contents.
	 * @param size required number of bytes (more may be allocated due to
	 * alignment and/or internal fragmentation)
	 * @return suitably aligned memory; never fails.
	 *
	 * no further operations with the same atom_fn are allowed to succeed
	 * until MarkComplete has been called.
	 **/
	IoBuf Reserve(const char* atom_fn, size_t size);

	/**
	 * Indicate that IO into the buffer has completed.
	 *
	 * this allows the cache to satisfy subsequent Retrieve() calls by
	 * returning this buffer; if CONFIG_READ_ONLY_CACHE, the buffer is
	 * made read-only. if need be and no references are currently attached
	 * to it, the memory can also be commandeered by Reserve().
	 *
	 * @param cost is the expected cost of retrieving the file again and
	 * influences how/when it is evicted from the cache.
	 **/
	void MarkComplete(const char* atom_fn, uint cost = 1);

	/**
	 * Attempt to retrieve a file's contents from the file cache.
	 *
	 * @return 0 if not in cache or its IO is still pending, otherwise a
	 * pointer to its (read-only) contents.
	 *
	 * if successful, the size is passed back and a reference is added to
	 * the file contents.
	 *
	 * note: does not call stats_cache because it does not know the file size
	 * in case of a cache miss; doing so is left to the caller.
	 **/
	const u8* Retrieve(const char* atom_fn, size_t& size);

	/**
	 * Indicate the file contents are no longer needed.
	 *
	 * this decreases the reference count; the memory can only be reused
	 * if it reaches 0. the contents remain in cache until they are evicted
	 * by a subsequent Reserve() call.
	 *
	 * note: fails (raises a warning) if called for an file that is
	 * currently between Reserve and MarkComplete operations.
	 **/
	void Release(const char* atom_fn);

	/**
	 * Invalidate the cached contents of a file.
	 *
	 * this ensures subsequent reads of the files see the current (presumably
	 * recently changed) contents of the file. has no effect if the file is
	 * not cached at the moment.
	 *
	 * this would typically be called in response to a notification that a
	 * file has changed.
	 **/
	LibError Invalidate(const char* atom_fn);

private:
	class Impl;
	boost::shared_ptr<Impl> impl;
};

#endif	// #ifndef INCLUDED_FILE_CACHE
