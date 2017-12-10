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

#include "precompiled.h"
#include "lib/file/common/file_stats.h"

#include <set>

#include "lib/timer.h"

#if FILE_STATS_ENABLED

// vfs
static size_t vfs_files;
static double vfs_size_total;
static double vfs_init_elapsed_time;

// file
static size_t unique_names;
static size_t unique_name_len_total;
static size_t open_files_cur, open_files_max;	// total = opened_files.size()

// file_buf
static size_t extant_bufs_cur, extant_bufs_max, extant_bufs_total;
static double buf_size_total, buf_aligned_size_total;

// file_io
static size_t user_ios;
static double user_io_size_total;
static double io_actual_size_total[FI_MAX_IDX][2];
static double io_elapsed_time[FI_MAX_IDX][2];
static double io_process_time_total;
static size_t io_seeks;

// file_cache
static size_t cache_count[2];
static double cache_size_total[2];
static size_t conflict_misses;
//static double conflict_miss_size_total;	// JW: currently not used nor computed
static size_t block_cache_count[2];

// archive builder
static size_t ab_connection_attempts;	// total number of trace entries
static size_t ab_repeated_connections;	// how many of these were not unique


// convenience functions for measuring elapsed time in an interval.
// by exposing start/finish calls, we avoid callers from querying
// timestamps when stats are disabled.
static double start_time;
static void timer_start(double* start_time_storage = &start_time)
{
	// make sure no measurement is currently active
	// (since start_time is shared static storage)
	ENSURE(*start_time_storage == 0.0);
	*start_time_storage = timer_Time();
}
static double timer_reset(double* start_time_storage = &start_time)
{
	double elapsed = timer_Time() - *start_time_storage;
	*start_time_storage = 0.0;
	return elapsed;
}

//-----------------------------------------------------------------------------

//
// vfs
//

void stats_vfs_file_add(size_t file_size)
{
	vfs_files++;
	vfs_size_total += file_size;
}

void stats_vfs_file_remove(size_t file_size)
{
	vfs_files--;
	vfs_size_total -= file_size;
}

// stats_vfs_init_* are currently unused
void stats_vfs_init_start()
{
	timer_start();
}

void stats_vfs_init_finish()
{
	vfs_init_elapsed_time += timer_reset();
}


//
// file
//

void stats_unique_name(size_t name_len)
{
	unique_names++;
	unique_name_len_total += name_len;
}


void stats_open()
{
	open_files_cur++;
	open_files_max = std::max(open_files_max, open_files_cur);

	// could also use a set to determine unique files that have been opened
}

void stats_close()
{
	ENSURE(open_files_cur > 0);
	open_files_cur--;
}


//
// file_buf
//

void stats_buf_alloc(size_t size, size_t alignedSize)
{
	extant_bufs_cur++;
	extant_bufs_max = std::max(extant_bufs_max, extant_bufs_cur);
	extant_bufs_total++;

	buf_size_total += size;
	buf_aligned_size_total += alignedSize;
}

void stats_buf_free()
{
	ENSURE(extant_bufs_cur > 0);
	extant_bufs_cur--;
}

void stats_buf_ref()
{
	extant_bufs_cur++;
}


//
// file_io
//

void stats_io_user_request(size_t user_size)
{
	user_ios++;
	user_io_size_total += user_size;
}

ScopedIoMonitor::ScopedIoMonitor()
{
	m_startTime = 0.0;
	timer_start(&m_startTime);
}

ScopedIoMonitor::~ScopedIoMonitor()
{
	// note: we can only bill IOs that have succeeded :S
	timer_reset(&m_startTime);
}

void ScopedIoMonitor::NotifyOfSuccess(FileIOImplentation fi, int opcode, off_t size)
{
	ENSURE(fi < FI_MAX_IDX);
	ENSURE(opcode == LIO_READ || opcode == LIO_WRITE);

	io_actual_size_total[fi][opcode == LIO_WRITE] += size;
	io_elapsed_time[fi][opcode == LIO_WRITE] += timer_reset(&m_startTime);
}


void stats_cb_start()
{
	timer_start();
}

void stats_cb_finish()
{
	io_process_time_total += timer_reset();
}


void stats_block_cache(CacheRet cr)
{
	ENSURE(cr == CR_HIT || cr == CR_MISS);
	block_cache_count[cr]++;
}


//
// archive builder
//

void stats_ab_connection(bool already_exists)
{
	ab_connection_attempts++;
	if(already_exists)
		ab_repeated_connections++;
}


//-----------------------------------------------------------------------------

template<typename T> int percent(T num, T divisor)
{
	if(!divisor)
		return 0;
	return (int)(100*num / divisor);
}

void file_stats_dump()
{
	if(!debug_filter_allows("FILE_STATS|"))
		return;

	const double KB = 1e3; const double MB = 1e6; const double ms = 1e-3;

	debug_printf("--------------------------------------------------------------------------------\n");
	debug_printf("File statistics:\n");

	// note: we split the reports into several debug_printfs for clarity;
	// this is necessary anyway due to fixed-size buffer.

	debug_printf(
		L"\nvfs:\n"
		L"Total files: %lu (%g MB)\n"
		L"Init/mount time: %g ms\n",
		(unsigned long)vfs_files, vfs_size_total/MB,
		vfs_init_elapsed_time/ms
	);

	debug_printf(
		L"\nfile:\n"
		L"Total names: %lu (%lu KB)\n"
		L"Max. concurrent: %lu; leaked: %lu.\n",
		(unsigned long)unique_names, (unsigned long)(unique_name_len_total/1000),
		(unsigned long)open_files_max, (unsigned long)open_files_cur
	);

	debug_printf(
		L"\nfile_buf:\n"
		L"Total buffers used: %lu (%g MB)\n"
		L"Max concurrent: %lu; leaked: %lu\n"
		L"Internal fragmentation: %d%%\n",
		(unsigned long)extant_bufs_total, buf_size_total/MB,
		(unsigned long)extant_bufs_max, (unsigned long)extant_bufs_cur,
		percent(buf_aligned_size_total-buf_size_total, buf_size_total)
	);

	debug_printf(
		L"\nfile_io:\n"
		L"Total user load requests: %lu (%g MB)\n"
		L"IO thoughput [MB/s; 0=never happened]:\n"
		L"  lowio: R=%.3g, W=%.3g\n"
		L"    aio: R=%.3g, W=%.3g\n"
		L"Average size = %g KB; seeks: %lu; total callback time: %g ms\n"
		L"Total data actually read from disk = %g MB\n",
		(unsigned long)user_ios, user_io_size_total/MB,
#define THROUGHPUT(impl, opcode) (io_elapsed_time[impl][opcode == LIO_WRITE] == 0.0)? 0.0 : (io_actual_size_total[impl][opcode == LIO_WRITE] / io_elapsed_time[impl][opcode == LIO_WRITE] / MB)
		THROUGHPUT(FI_LOWIO, LIO_READ), THROUGHPUT(FI_LOWIO, LIO_WRITE),
		THROUGHPUT(FI_AIO  , LIO_READ), THROUGHPUT(FI_AIO  , LIO_WRITE),
		user_io_size_total/user_ios/KB, (unsigned long)io_seeks, io_process_time_total/ms,
		(io_actual_size_total[FI_LOWIO][0]+io_actual_size_total[FI_AIO][0])/MB
	);

	debug_printf(
		L"\nfile_cache:\n"
		L"Hits: %lu (%g MB); misses %lu (%g MB); ratio: %u%%\n"
		L"Percent of requested bytes satisfied by cache: %u%%; non-compulsory misses: %lu (%u%% of misses)\n"
		L"Block hits: %lu; misses: %lu; ratio: %u%%\n",
		(unsigned long)cache_count[CR_HIT], cache_size_total[CR_HIT]/MB, (unsigned long)cache_count[CR_MISS], cache_size_total[CR_MISS]/MB, percent(cache_count[CR_HIT], cache_count[CR_HIT]+cache_count[CR_MISS]),
		percent(cache_size_total[CR_HIT], cache_size_total[CR_HIT]+cache_size_total[CR_MISS]), (unsigned long)conflict_misses, percent(conflict_misses, cache_count[CR_MISS]),
		(unsigned long)block_cache_count[CR_HIT], (unsigned long)block_cache_count[CR_MISS], percent(block_cache_count[CR_HIT], block_cache_count[CR_HIT]+block_cache_count[CR_MISS])
	);

	debug_printf(
		L"\nvfs_optimizer:\n"
		L"Total trace entries: %lu; repeated connections: %lu; unique files: %lu\n",
		(unsigned long)ab_connection_attempts, (unsigned long)ab_repeated_connections, (unsigned long)(ab_connection_attempts-ab_repeated_connections)
	);
}

#endif	// FILE_STATS_ENABLED
