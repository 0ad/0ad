#include "precompiled.h"

#include "lib/res/res.h"
#include "lib/res/file/compression.h"
#include "lib/allocators.h"
#include "lib/timer.h"
#include "vfs_optimizer.h"


enum TraceState
{
	TS_UNINITIALIZED,
	TS_DISABLED,
	TS_ENABLED,
	TS_ERROR,
	TS_SHUTDOWN
};
static uintptr_t trace_state = TS_UNINITIALIZED;	// values from TraceState; type for use with CAS


static Pool trace_pool;



void trace_shutdown()
{
	if(trace_state == TS_DISABLED || trace_state == TS_ENABLED)
	{
		(void)pool_destroy(&trace_pool);
		trace_state = TS_SHUTDOWN;
	}
}

void trace_enable(bool want_enabled)
{
	if(trace_state == TS_SHUTDOWN || trace_state == TS_ERROR)
		WARN_ERR_RETURN(ERR_LOGIC);

	if(CAS(&trace_state, TS_UNINITIALIZED, TS_ERROR))
	{
		if(pool_create(&trace_pool, 4*MiB, sizeof(TraceEntry)) < 0)
			return;	// leave trace_state set to TS_ERROR
	}

	trace_state = want_enabled? TS_ENABLED : TS_DISABLED;
}


void trace_add(const char* P_fn)
{
	if(trace_state == TS_DISABLED || trace_state == TS_UNINITIALIZED)
		return;
	if(trace_state != TS_ENABLED)
		WARN_ERR_RETURN(ERR_LOGIC);

	TraceEntry* t = (TraceEntry*)pool_alloc(&trace_pool, 0);
	if(!t)
		return;
	t->timestamp = get_time();
	t->atom_fn = file_make_unique_fn_copy(P_fn, 0);
}


LibError trace_write_to_file(const char* trace_filename)
{
	if(trace_state == TS_UNINITIALIZED)
		return ERR_OK;
	if(trace_state != TS_ENABLED && trace_state != TS_DISABLED)
		WARN_RETURN(ERR_LOGIC);

	char N_fn[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(trace_filename, N_fn));
	FILE* f = fopen(N_fn, "wt");
	if(!f)
		return ERR_FILE_ACCESS;

	Trace t;
	trace_get(&t);
	for(size_t i = 0; i < t.num_ents; i++)
		fprintf(f, "%#010f: %s\n", t.ents[i].timestamp, t.ents[i].atom_fn);

	(void)fclose(f);
	return ERR_OK;
}


LibError trace_load_from_file(const char* trace_filename)
{
	char N_fn[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(trace_filename, N_fn));
	FILE* f = fopen(N_fn, "rt");
	if(!f)
		return ERR_FILE_NOT_FOUND;

	// parse lines and stuff them in trace_pool
	// (as if they had been trace_add-ed; replaces any existing data)
	pool_free_all(&trace_pool);
	char fmt[20];
	snprintf(fmt, ARRAY_SIZE(fmt), "%%f: %%%ds\n", PATH_MAX);
	for(;;)
	{
		double timestamp; char P_path[PATH_MAX];
		int ret = fscanf(f, fmt, &timestamp, P_path);
		if(ret == EOF)
			break;
		if(ret != 2)
			debug_warn("invalid line in trace file");

		TraceEntry* ent = (TraceEntry*)pool_alloc(&trace_pool, 0);
		debug_assert(ent != 0);	// was written to file from same pool => must fit
		ent->timestamp = timestamp;
		ent->atom_fn = file_make_unique_fn_copy(P_path, 0);
	}

	fclose(f);
	return ERR_OK;
}


void trace_get(Trace* t)
{
	t->ents = (const TraceEntry*)trace_pool.da.base;
	t->num_ents = (uint)(trace_pool.da.pos / sizeof(TraceEntry));
}


///////////////////////////////////////////////////////////////////////////////




#if 0

struct FileList
{
	const char* atom_fns;
	size_t num_files;
};

static LibError filelist_build(Trace* t, FileList* fl)
{
}

static LibError filelist_get(FileList* fl, uint i, const char* path)
{
	return ERR_DIR_END;
}



static LibError compress_cb(uintptr_t cb_ctx, const void* block, size_t size, size_t* bytes_processed)
{
	uintptr_t ctx = cb_ctx;

	*bytes_processed = comp_feed(ctx, block, size);
	return INFO_CB_CONTINUE;
}

static LibError read_and_compress_file(uintptr_t ctx, ZipEntry* ze)
{
	const char* fn = ze->path;

	struct stat s;
	RETURN_ERR(file_stat(fn, &s));
	const size_t ucsize = s.st_size;

	RETURN_ERR(comp_reset(ctx));
	RETURN_ERR(comp_alloc_output(ctx, ucsize));

	File f;
	RETURN_ERR(file_open(fn, 0, &f));
	FileIOBuf buf = FILE_BUF_ALLOC;
	uintptr_t cb_ctx = ctx;
	ssize_t cbytes_output = file_io(&f, 0, ucsize, &buf, compress_cb, cb_ctx);
	(void)file_close(&f);

	void* cdata; size_t csize;
	RETURN_ERR(comp_finish(ctx, &cdata, &csize));
	debug_assert(cbytes_output <= csize);

	RETURN_ERR(cbytes_output);

// decide if it was better compressed or not

	ze->ucsize = ucsize;
	ze->mtime  = s.st_mtime;
	ze->method = CM_DEFLATE;
	ze->csize  = csize;
	ze->cdata  = cdata;

	zip_archive_add(&za, &ze);

	return ERR_OK;
}

static void build_optimized_archive(const char* trace_file, const char* zip_filename)
{
	FileList fl;
	{
		Trace t;
		RETURN_ERR(trace_load_from_file(trace_filename, &t));
		filelist_build(&t, &fl);
	}

	ZipArchive za;
	zip_archive_create(zip_filename, &za);

	uintptr_t ctx = comp_alloc();
	uint trace_i = 0;
	uint queued_files = 0, committed_files = 0;

	for(;;)
	{

/*
document: zlib layer is ok to allocate. caller shouldnt do so from a pool:
		when the next file is going to be loaded and decompressed but our pool is full,
			we need to wait for the archive write to finish and mark pool as reclaimed.
			this is better done with heap; also, memory isn't bottleneck for readqueue size
*/

		ZipEntry ze; // TODO: QUEUE
		const int max_readqueue_depth = 1;
		for(uint i = 0; i < max_readqueue_depth; i++)
		{
			LibError ret = trace_get_next_file(trace, trace_i, ze.path);
			if(ret == ERR_DIR_END)
				break;

			WARN_ERR(read_and_compress_file(ctx, &ze));
			queued_files++;
		}

		if(committed_files == queued_files)
			break;
		zip_archive_add(&za, &ze);
		committed_files++;
	}


	comp_free(ctx);

	zip_archive_finish(&za);
}
#endif