#include "lib/timer.h"
#include "vfs_optimizer.h"

const size_t AIO_SECTOR_SIZE = 512;

#define CHECK_FILE(f) CHECK_ERR(file_validate(f))

extern void file_io_shutdown();

extern void file_cache_init();
extern void file_cache_shutdown();

//-----------------------------------------------------------------------------

enum FileIOImplentation { FI_LOWIO, FI_AIO, FI_MAX_IDX };
enum FileOp { FO_READ, FO_WRITE };
enum CacheRet { CR_HIT, CR_MISS };

#define FILE_STATS_ENABLED 1

#if FILE_STATS_ENABLED

class FileStats
{
public:
	void notify_unique_file()
	{
		unique_files++;
	}

	void notify_open(const char* atom_fn, size_t file_size)
	{
		open_files_cur++;
		open_files_max = MAX(open_files_max, open_files_cur);

		typedef std::pair<std::set<const char*>::iterator, bool> PairIB;
		PairIB ret = accessed_files.insert(atom_fn);
		// wasn't inserted yet
		if(ret.second)
			unique_file_size_total += file_size;
	}
	void notify_close()
	{
		debug_assert(open_files_cur > 0);
		open_files_cur--;
	}

	void notify_buf_alloc()
	{
		extant_bufs_cur++;
		extant_bufs_max = MAX(extant_bufs_max, extant_bufs_cur);
		extant_bufs_total++;
	}
	void notify_buf_free()
	{
		debug_assert(extant_bufs_cur > 0);
		extant_bufs_cur--;
	}

	void notify_io(FileIOImplentation fi, FileOp fo, size_t user_size, size_t actual_size, double start_time)
	{
		io_count++;
		debug_assert(io_actual_size_total >= io_user_size_total);
		io_user_size_total += user_size;
		io_actual_size_total += actual_size;

		const double end_time = get_time();
		const double throughput = actual_size / (end_time - start_time);

		debug_assert(fi < FI_MAX_IDX);
		double& avg = (fo == FO_READ)? read_throughput_avg[fi] : write_throughput_avg[fi];

		const float gain = (avg == 0.0)? 1.0f : 0.5f;
		avg = gain*throughput + (1.0f-gain)*avg;
	}

	void notify_cache(CacheRet cr, size_t size)
	{
		debug_assert(cr == CR_HIT || cr == CR_MISS);
		cache_count[cr]++;
		cache_size_total[cr] += size;
	}

	void dump()
	{
		// note: writes count toward io_actual_size_total but not cache.
		debug_assert(io_actual_size_total >= cache_size_total[CR_MISS]);

// not necessarily true, since not all IO clients use the cache.		
//		debug_assert(io_count >= cache_count[CR_MISS]);

		const size_t unique_files_accessed = accessed_files.size();

		// guesstimate miss rate due to cache capacity
		// (indicates effectiveness of caching algorithm)
		const u64 working_set_est = unique_file_size_total * ((double)unique_files_accessed/
		                                                              unique_files);
		const uint cache_capacity_miss_rate_est = (cache_size_total[CR_MISS]-working_set_est)/
		                               (double)(cache_size_total[CR_HIT]+cache_size_total[CR_MISS]);

		const double KB = 1000.0; const double MB = 1000000.0;
		debug_printf(
			"File statistics\n"
			"--------------------------------------------------------------------------------\n"
			"Total files seen: %u; total files accessed: %u.\n"
			"  unused files: %d%%.\n"
			"Max. open files: %u; leaked files: %u.\n"
			"Total buffers (re)used: %u; max. extant buffers: %u; leaked buffers: %u.\n"
			"Total # user IOs: %u; cumulative size: %.3g MB; average size: %.2g KB.\n"
			"  unused data: %d%%.\n"
			"IO thoughput [MB/s; 0=never happened]:\n"
			"  lowio: R=%.3g, W=%.3g\n"
			"    aio: R=%.3g, W=%.3g\n"
			"File cache totals: hits: %.3g MB; misses: %.3g MB.\n"
			"  ratio: %d%%; capacity miss rate: ~%d%%.\n"
			"--------------------------------------------------------------------------------\n"
			,
			unique_files, unique_files_accessed,
			100-(int)(((float)unique_files_accessed)/unique_files),
			open_files_max, open_files_cur,
			extant_bufs_total, extant_bufs_max, extant_bufs_cur,
			io_count, io_user_size_total/MB, ((double)io_user_size_total)/io_count/KB,
			100-(int)(((float)io_user_size_total)/unique_file_size_total),
			read_throughput_avg[FI_LOWIO]/MB, write_throughput_avg[FI_LOWIO]/MB,
			read_throughput_avg[FI_AIO  ]/MB, write_throughput_avg[FI_AIO  ]/MB,
			cache_size_total[CR_HIT]/MB, cache_size_total[CR_MISS]/MB,
			(int)(((float)cache_size_total[CR_HIT])/(cache_size_total[CR_HIT]+cache_size_total[CR_MISS])), cache_capacity_miss_rate_est
		);
	}

	FileStats()
		: accessed_files(), read_throughput_avg(), write_throughput_avg(),
		  cache_count(), cache_size_total()
	{
		unique_files = 0;
		unique_file_size_total = 0;
		open_files_cur = open_files_max = 0;
		extant_bufs_cur = extant_bufs_max = 0;
		io_count = 0;
		io_user_size_total = io_actual_size_total = 0;
	}

private:
	uint unique_files;
	std::set<const char*> accessed_files;
	u64 unique_file_size_total;

	uint open_files_cur, open_files_max;	// total = accessed_files.size()

	uint extant_bufs_cur, extant_bufs_max, extant_bufs_total;

	uint io_count;
	u64 io_user_size_total;
	u64 io_actual_size_total;

	double  read_throughput_avg[FI_MAX_IDX];
	double write_throughput_avg[FI_MAX_IDX];

	// file cache only (hit and miss; indexed via CacheRet)
	uint cache_count[2];
	u64 cache_size_total[2];
};

extern FileStats stats;

#define FILE_STATS_NOTIFY_UNIQUE_FILE() stats.notify_unique_file()
#define FILE_STATS_NOTIFY_OPEN(atom_fn, file_size) stats.notify_open(atom_fn, file_size)
#define FILE_STATS_NOTIFY_CLOSE() stats.notify_close()
#define FILE_STATS_NOTIFY_BUF_ALLOC() stats.notify_buf_alloc()
#define FILE_STATS_NOTIFY_BUF_FREE() stats.notify_buf_free()
#define FILE_STATS_NOTIFY_IO(fi, fo, user_size, actual_size, start_time) stats.notify_io(fi, fo, user_size, actual_size, start_time)
#define FILE_STATS_NOTIFY_CACHE(cr, size) stats.notify_cache(cr, size)
#define FILE_STATS_DUMP() stats.dump()

#else	// !FILE_STATS_ENABLED

#define FILE_STATS_NOTIFY_UNIQUE_FILE() 0
#define FILE_STATS_NOTIFY_OPEN(atom_fn, file_size) 0
#define FILE_STATS_NOTIFY_CLOSE() 0
#define FILE_STATS_NOTIFY_BUF_ALLOC() 0
#define FILE_STATS_NOTIFY_BUF_FREE() 0
#define FILE_STATS_NOTIFY_IO(fi, fo, user_size, actual_size, start_time) 0
#define FILE_STATS_NOTIFY_CACHE(cr, size) 0
#define FILE_STATS_DUMP() 0

#endif
