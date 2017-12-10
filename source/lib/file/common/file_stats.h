/* Copyright (C) 2010 Wildfire Games.
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
 * gathers statistics from all file modules.
 */

#ifndef INCLUDED_FILE_STATS
#define INCLUDED_FILE_STATS

#include "lib/posix/posix_aio.h"	// LIO_READ, LIO_WRITE

#define FILE_STATS_ENABLED 0


enum FileIOImplentation { FI_LOWIO, FI_AIO, FI_BCACHE, FI_MAX_IDX };
enum CacheRet { CR_HIT, CR_MISS };

#if FILE_STATS_ENABLED

// vfs
extern void stats_vfs_file_add(size_t file_size);
extern void stats_vfs_file_remove(size_t file_size);
extern void stats_vfs_init_start();
extern void stats_vfs_init_finish();

// file

// currently not called because string_pool is now in lib/allocators
extern void stats_unique_name(size_t name_len);
extern void stats_open();
extern void stats_close();

// file_buf
extern void stats_buf_alloc(size_t size, size_t alignedSize);
extern void stats_buf_free();
extern void stats_buf_ref();

// file_io
extern void stats_io_user_request(size_t user_size);

// this is used to measure effective throughput for the two
// synchronous IO variants.
// note: improved measurements of the actual aio throughput by instrumenting
// issue/wait doesn't work because IOManager's decompression may cause us to
// miss the exact end of IO, thus throwing off measurements.
class ScopedIoMonitor
{
public:
	ScopedIoMonitor();
	~ScopedIoMonitor();
	void NotifyOfSuccess(FileIOImplentation fi, int opcode, off_t size);

private:
	double m_startTime;
};

extern void stats_cb_start();
extern void stats_cb_finish();

// file_cache
extern void stats_block_cache(CacheRet cr);

// archive builder
extern void stats_ab_connection(bool already_exists);

extern void file_stats_dump();

#else

#define stats_vfs_file_add(file_size)
#define stats_vfs_file_remove(file_size)
#define stats_vfs_init_start()
#define stats_vfs_init_finish()
#define stats_unique_name(name_len)
#define stats_open()
#define stats_close()
#define stats_buf_alloc(size, alignedSize)
#define stats_buf_free()
#define stats_buf_ref()
#define stats_io_user_request(user_size)
class ScopedIoMonitor
{
public:
	ScopedIoMonitor() {}
	~ScopedIoMonitor() {}
	void NotifyOfSuccess(FileIOImplentation UNUSED(fi), int UNUSED(opcode), off_t UNUSED(size)) {}
};
#define stats_cb_start()
#define stats_cb_finish()
#define stats_block_cache(cr)
#define stats_ab_connection(already_exists)
#define file_stats_dump()

#endif

#endif	// #ifndef INCLUDED_FILE_STATS
