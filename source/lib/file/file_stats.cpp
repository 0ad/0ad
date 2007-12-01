/**
 * =========================================================================
 * File        : file_stats.cpp
 * Project     : 0 A.D.
 * Description : gathers statistics from all file modules.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "file_stats.h"

#include <set>

#include "lib/timer.h"


typedef std::set<const char*> AtomFnSet;
typedef std::pair<AtomFnSet::iterator, bool> PairIB;

// vfs
static uint vfs_files;
static size_t vfs_size_total;
static double vfs_init_elapsed_time;

// file
static uint unique_names;
static size_t unique_name_len_total;
static uint open_files_cur, open_files_max;	// total = opened_files.size()
static double opened_file_size_total;
static AtomFnSet opened_files;

// file_buf
static uint extant_bufs_cur, extant_bufs_max, extant_bufs_total;
static double buf_size_total, buf_aligned_size_total;

// file_io
static uint user_ios;
static double user_io_size_total;
static double io_actual_size_total[FI_MAX_IDX][2];
static double io_elapsed_time[FI_MAX_IDX][2];
static double io_process_time_total;
static uint io_seeks;

// file_cache
static uint cache_count[2];
static double cache_size_total[2];
static AtomFnSet ever_cached_files;
static uint conflict_misses;
static double conflict_miss_size_total;
static uint block_cache_count[2];

// archive builder
static uint ab_connection_attempts;	// total number of trace entries
static uint ab_repeated_connections;	// how many of these were not unique


// convenience functions for measuring elapsed time in an interval.
// by exposing start/finish calls, we avoid callers from querying
// timestamps when stats are disabled.
static double start_time;
static void timer_start(double* start_time_storage = &start_time)
{
	// make sure no measurement is currently active
	// (since start_time is shared static storage)
	debug_assert(*start_time_storage == 0.0);
	*start_time_storage = get_time();
}
static double timer_reset(double* start_time_storage = &start_time)
{
	double elapsed = get_time() - *start_time_storage;
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


void stats_open(const char* atom_fn, size_t file_size)
{
	open_files_cur++;
	open_files_max = std::max(open_files_max, open_files_cur);

	PairIB ret = opened_files.insert(atom_fn);
	// hadn't been opened yet
	if(ret.second)
		opened_file_size_total += file_size;
}

void stats_close()
{
	debug_assert(open_files_cur > 0);
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
	debug_assert(extant_bufs_cur > 0);
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
	timer_start(&m_startTime);
}

ScopedIoMonitor::~ScopedIoMonitor()
{
	// note: we can only bill IOs that have succeeded :S
}

void ScopedIoMonitor::NotifyOfSuccess(FileIOImplentation fi, char mode, size_t size)
{
	debug_assert(fi < FI_MAX_IDX);
	debug_assert(mode == 'r' || mode == 'w');
	const FileOp op = (mode == 'r')? FO_READ : FO_WRITE;

	io_actual_size_total[fi][op] += size;
	io_elapsed_time[fi][op] += timer_reset(&m_startTime);
}

void stats_io_check_seek(BlockId& blockId)
{
	static BlockId lastBlockId;

	if(blockId != lastBlockId)
		io_seeks++;
	lastBlockId = blockId;
}


void stats_cb_start()
{
	timer_start();
}

void stats_cb_finish()
{
	io_process_time_total += timer_reset();
}


//
// file_cache
//

void stats_cache(CacheRet cr, size_t size, const char* atom_fn)
{
	debug_assert(cr == CR_HIT || cr == CR_MISS);

	if(cr == CR_MISS)
	{
		PairIB ret = ever_cached_files.insert(atom_fn);
		if(!ret.second)	// was already cached once
		{
			conflict_miss_size_total += size;
			conflict_misses++;
		}
	}

	cache_count[cr]++;
	cache_size_total[cr] += size;
}

void stats_block_cache(CacheRet cr)
{
	debug_assert(cr == CR_HIT || cr == CR_MISS);
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
		"\nvfs:\n"
		"Total files: %u (%g MB)\n"
		"Init/mount time: %g ms\n",
		vfs_files, vfs_size_total/MB,
		vfs_init_elapsed_time/ms
	);

	debug_printf(
		"\nfile:\n"
		"Total names: %u (%u KB)\n"
		"Accessed files: %u (%g MB) -- %u%% of data set\n"
		"Max. concurrent: %u; leaked: %u.\n",
		unique_names, unique_name_len_total/1000, 
		opened_files.size(), opened_file_size_total/MB, percent(opened_files.size(), (size_t)vfs_files),
		open_files_max, open_files_cur
	);

	debug_printf(
		"\nfile_buf:\n"
		"Total buffers used: %u (%g MB)\n"
		"Max concurrent: %u; leaked: %u\n"
		"Internal fragmentation: %d%%\n",
		extant_bufs_total, buf_size_total/MB,
		extant_bufs_max, extant_bufs_cur,
		percent(buf_aligned_size_total-buf_size_total, buf_size_total)
	);

	debug_printf(
		"\nfile_io:\n"
		"Total user load requests: %u (%g MB)\n"
		"IO thoughput [MB/s; 0=never happened]:\n"
		"  lowio: R=%.3g, W=%.3g\n"
		"    aio: R=%.3g, W=%.3g\n"
		"Average size = %g KB; seeks: %u; total callback time: %g ms\n"
		"Total data actually read from disk = %g MB\n",
		user_ios, user_io_size_total/MB,
#define THROUGHPUT(impl, op) (io_elapsed_time[impl][op] == 0.0)? 0.0 : (io_actual_size_total[impl][op] / io_elapsed_time[impl][op] / MB)
		THROUGHPUT(FI_LOWIO, FO_READ), THROUGHPUT(FI_LOWIO, FO_WRITE),
		THROUGHPUT(FI_AIO  , FO_READ), THROUGHPUT(FI_AIO  , FO_WRITE),
		user_io_size_total/user_ios/KB, io_seeks, io_process_time_total/ms,
		(io_actual_size_total[FI_LOWIO][FO_READ]+io_actual_size_total[FI_AIO][FO_READ])/MB
	);

	debug_printf(
		"\nfile_cache:\n"
		"Hits: %u (%g MB); misses %u (%g MB); ratio: %u%%\n"
		"Percent of requested bytes satisfied by cache: %u%%; non-compulsory misses: %u (%u%% of misses)\n"
		"Block hits: %u; misses: %u; ratio: %u%%\n",
		cache_count[CR_HIT], cache_size_total[CR_HIT]/MB, cache_count[CR_MISS], cache_size_total[CR_MISS]/MB, percent(cache_count[CR_HIT], cache_count[CR_HIT]+cache_count[CR_MISS]),
		percent(cache_size_total[CR_HIT], cache_size_total[CR_HIT]+cache_size_total[CR_MISS]), conflict_misses, percent(conflict_misses, cache_count[CR_MISS]),
		block_cache_count[CR_HIT], block_cache_count[CR_MISS], percent(block_cache_count[CR_HIT], block_cache_count[CR_HIT]+block_cache_count[CR_MISS])
	);

	debug_printf(
		"\nvfs_optimizer:\n"
		"Total trace entries: %u; repeated connections: %u; unique files: %u\n",
		ab_connection_attempts, ab_repeated_connections, ab_connection_attempts-ab_repeated_connections
	);
}
