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
 * cache of file contents (supports zero-copy IO)
 */

#ifndef INCLUDED_FILE_CACHE
#define INCLUDED_FILE_CACHE

#include "lib/file/vfs/vfs_path.h"

/**
 * cache of file contents with support for zero-copy IO.
 * this works by reserving a region of the cache, using it as the IO buffer,
 * and returning the memory directly to users. optional write-protection
 * via MMU ensures that the shared contents aren't inadvertently changed.
 *
 * (unique copies of) VFS pathnames are used as lookup key and owner tag.
 *
 * to ensure efficient operation and prevent fragmentation, only one
 * reference should be active at a time. in other words, read a file,
 * process it, and only then start reading the next file.
 *
 * rationale: this is rather similar to BlockCache; however, the differences
 * (Reserve's size parameter, eviction policies) are enough to warrant
 * separate implementations.
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
	 * Reserve a chunk of the cache's memory region.
	 *
	 * @param size required number of bytes (more may be allocated due to
	 * alignment and/or internal fragmentation)
	 * @return memory suitably aligned for IO; never fails.
	 *
	 * it is expected that this data will be Add()-ed once its IO completes.
	 **/
	shared_ptr<u8> Reserve(size_t size);

	/**
	 * Add a file's contents to the cache.
	 *
	 * The cache will be able to satisfy subsequent Retrieve() calls by
	 * returning this data; if CONFIG2_CACHE_READ_ONLY, the buffer is made
	 * read-only. If need be and no references are currently attached to it,
	 * the memory can also be commandeered by Reserve().
	 *
	 * @param data
	 * @param size
	 * @param pathname key that will be used to Retrieve file contents.
	 * @param cost is the expected cost of retrieving the file again and
	 *		  influences how/when it is evicted from the cache.
	 **/
	void Add(const VfsPath& pathname, const shared_ptr<u8>& data, size_t size, size_t cost = 1);

	/**
	 * Remove a file's contents from the cache (if it exists).
	 *
	 * this ensures subsequent reads of the files see the current, presumably
	 * recently changed, contents of the file.
	 *
	 * this would typically be called in response to a notification that a
	 * file has changed.
	 **/
	void Remove(const VfsPath& pathname);

	/**
	 * Attempt to retrieve a file's contents from the file cache.
	 *
	 * @return whether the contents were successfully retrieved; if so,
	 * data references the read-only file contents.
	 **/
	bool Retrieve(const VfsPath& pathname, shared_ptr<u8>& data, size_t& size);

private:
	class Impl;
	shared_ptr<Impl> impl;
};

#endif	// #ifndef INCLUDED_FILE_CACHE
