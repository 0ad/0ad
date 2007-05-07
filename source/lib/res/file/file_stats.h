/**
 * =========================================================================
 * File        : file_stats.h
 * Project     : 0 A.D.
 * Description : gathers statistics from all file modules.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FILE_STATS
#define INCLUDED_FILE_STATS

#define FILE_STATS_ENABLED 1


enum FileIOImplentation { FI_LOWIO, FI_AIO, FI_BCACHE, FI_MAX_IDX };
enum FileOp { FO_READ, FO_WRITE };
enum CacheRet { CR_HIT, CR_MISS };


#if FILE_STATS_ENABLED

// vfs
extern void stats_vfs_file_add(size_t file_size);
extern void stats_vfs_file_remove(size_t file_size);
extern void stats_vfs_init_start();
extern void stats_vfs_init_finish();

// file
extern void stats_unique_name(size_t name_len);
extern void stats_open(const char* atom_fn, size_t file_size);
extern void stats_close();

// file_buf
extern void stats_buf_alloc(size_t user_size, size_t padded_size);
extern void stats_buf_free();
extern void stats_buf_ref();

// file_io
extern void stats_io_user_request(size_t user_size);
extern void stats_io_sync_start(double* start_time_storage);
extern void stats_io_sync_finish(FileIOImplentation fi, FileOp fo, ssize_t user_size, double* start_time_storage);
extern void stats_io_check_seek(const char* atom_fn, u32 block_num);
extern void stats_cb_start();
extern void stats_cb_finish();

// file_cache
extern void stats_cache(CacheRet cr, size_t size, const char* atom_fn);
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
#define stats_open(atom_fn, file_size)
#define stats_close()
#define stats_buf_alloc(user_size, padded_size)
#define stats_buf_free()
#define stats_buf_ref()
#define stats_io_user_request(user_size)
#define stats_io_sync_start(disk_pos, start_time_storage)
#define stats_io_sync_finish(fi, fo, user_size, start_time_storage)
#define stats_io_check_seek(atom_fn, block_num)
#define stats_cb_start()
#define stats_cb_finish()
#define stats_cache(cr, size, atom_fn)
#define stats_block_cache(cr)
#define stats_ab_connection(already_exists)
#define file_stats_dump()

#endif

#endif	// #ifndef INCLUDED_FILE_STATS
