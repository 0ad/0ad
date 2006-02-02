#include "precompiled.h"

#include "lib/allocators.h"
#include "lib/timer.h"
#include "file_internal.h"

static uintptr_t trace_initialized;	// set via CAS
static Pool trace_pool;

// call at before using trace_pool. no-op if called more than once.
static inline void trace_init()
{
	if(CAS(&trace_initialized, 0, 1))
		(void)pool_create(&trace_pool, 4*MiB, sizeof(TraceEntry));
}

void trace_shutdown()
{
	if(CAS(&trace_initialized, 1, 2))
		(void)pool_destroy(&trace_pool);
}


static bool trace_enabled;

void trace_enable(bool want_enabled)
{
	trace_enabled = want_enabled;
}


static void trace_add(TraceOp op, const char* P_fn, size_t size, uint flags = 0, double timestamp = 0.0)
{
	trace_init();
	if(!trace_enabled)
		return;

	if(timestamp == 0.0)
		timestamp = get_time();

	TraceEntry* t = (TraceEntry*)pool_alloc(&trace_pool, 0);
	if(!t)
		return;
	t->timestamp = timestamp;
	t->atom_fn   = file_make_unique_fn_copy(P_fn);
	t->size      = size;
	t->op        = op;
	t->flags     = flags;
}


void trace_notify_load(const char* P_fn, size_t size, uint flags)
{
	trace_add(TO_LOAD, P_fn, size, flags);
}

void trace_notify_free(const char* P_fn, size_t size)
{
	trace_add(TO_FREE, P_fn, size);
}


void trace_get(Trace* t)
{
	t->ents = (const TraceEntry*)trace_pool.da.base;
	t->num_ents = (uint)(trace_pool.da.pos / sizeof(TraceEntry));
}

void trace_clear()
{
	pool_free_all(&trace_pool);
}


LibError trace_write_to_file(const char* trace_filename)
{
	char N_fn[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(trace_filename, N_fn));
	FILE* f = fopen(N_fn, "wt");
	if(!f)
		WARN_RETURN(ERR_FILE_ACCESS);

	Trace t;
	trace_get(&t);
	const TraceEntry* ent = t.ents;
	for(size_t i = 0; i < t.num_ents; i++, ent++)
	{
		char opcode = '?';
		switch(ent->op)
		{
		case TO_LOAD: opcode = 'L'; break;
		case TO_FREE: opcode = 'F'; break;
		default: debug_warn("invalid TraceOp");
		}

		debug_assert(ent->op == TO_LOAD || ent->op == TO_FREE);
		fprintf(f, "%#010f: %c \"%s\" %d %04x\n", ent->timestamp, opcode, ent->atom_fn, ent->size, ent->flags);
	}

	(void)fclose(f);
	return ERR_OK;
}


LibError trace_read_from_file(const char* trace_filename, Trace* t)
{
	// we use trace_add, which is the same mechanism called by trace_notify*;
	// therefore, tracing needs to be enabled.
	trace_enabled = true;

	char N_fn[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(trace_filename, N_fn));
	FILE* f = fopen(N_fn, "rt");
	if(!f)
		WARN_RETURN(ERR_FILE_NOT_FOUND);

	// parse lines and stuff them in trace_pool
	// (as if they had been trace_add-ed; replaces any existing data)
	trace_clear();
	// .. bake PATH_MAX limit into string.
	char fmt[30];
	snprintf(fmt, ARRAY_SIZE(fmt), "%%lf: %%c \"%%%d[^\"]\" %%d %%04x\n", PATH_MAX);
	for(;;)
	{
		double timestamp; char opcode; char P_path[PATH_MAX]; size_t size; uint flags;
		int ret = fscanf(f, fmt, &timestamp, &opcode, P_path, &size, &flags);
		if(ret == EOF)
			break;
		debug_assert(ret == 5);

		TraceOp op = TO_LOAD;	// default in case file is garbled
		switch(opcode)
		{
		case 'L': op = TO_LOAD; break;
		case 'F': op = TO_FREE; break;
		default: debug_warn("invalid TraceOp");
		}

		trace_add(op, P_path, size, flags, timestamp);
	}

	fclose(f);

	trace_get(t);

	// all previous trace entries were hereby lost (overwritten),
	// so there's no sense in continuing.
	trace_enabled = false;

	return ERR_OK;
}


enum SimulateFlags
{
	SF_SYNC_TO_TIMESTAMP = 1
};

LibError trace_simulate(const char* trace_filename, uint flags)
{
	Trace t;
	RETURN_ERR(trace_read_from_file(trace_filename, &t));

	// prevent the actions we carry out below from generating
	// trace_add-s.
	trace_enabled = false;

	const double start_time = get_time();
	const double first_timestamp = t.ents[0].timestamp;

	const TraceEntry* ent = t.ents;
	for(uint i = 0; i < t.num_ents; i++, ent++)
	{
		// wait until time for next entry if caller requested this
		if(flags & SF_SYNC_TO_TIMESTAMP)
		{
			while(get_time()-start_time < ent->timestamp-first_timestamp)
			{
				// busy-wait (don't sleep - can skew results)
			}
		}

		// carry out this entry's operation
		FileIOBuf buf; size_t size;
		switch(ent->op)
		{
		case TO_LOAD:
			(void)vfs_load(ent->atom_fn, buf, size, ent->flags);
			break;
		case TO_FREE:
			buf = file_cache_find(ent->atom_fn, &size);
			(void)file_buf_free(buf);
			break;
		default:
			debug_warn("unknown TraceOp");
		}
	}

	trace_clear();

	return ERR_OK;
}


//-----------------------------------------------------------------------------

struct FileList
{
	const char** atom_fns;
	size_t num_files;
	size_t i;
};


static LibError filelist_build(Trace* t, FileList* fl)
{
	// count # files
	fl->num_files = 0;
	for(size_t i = 0; i < t->num_ents; i++)
		if(t->ents[i].op == TO_LOAD)
			fl->num_files++;

	if(!fl->num_files)
		return ERR_DIR_END;

	fl->atom_fns = new const char*[fl->num_files];

	size_t ti = 0;
	for(size_t i = 0; i < fl->num_files; i++)
	{
		// find next trace entry that is a load (must exist)
		while(t->ents[ti].op != TO_LOAD)
			ti++;
		fl->atom_fns[i] = t->ents[ti].atom_fn;
		ti++;
	}

	fl->i = 0;
	return ERR_OK;
}


static const char* filelist_get_next(FileList* fl)
{
	if(fl->i == fl->num_files)
		return 0;
	return fl->atom_fns[fl->i++];
}


static void filelist_free(FileList* fl)
{
	delete[] fl->atom_fns;
	fl->atom_fns = 0;
}


//-----------------------------------------------------------------------------

static inline bool file_type_is_uncompressible(const char* fn)
{
	const char* ext = strrchr(fn, '.');
	// no extension? bail; assume compressible
	if(!ext)
		return true;

	// this is a selection of file types that are certainly not
	// further compressible. we need not include every type under the sun -
	// this is only a slight optimization that avoids wasting time
	// compressing files. the real decision as to cmethod is made based
	// on attained compression ratio.
	static const char* uncompressible_exts[] =
	{
		"zip", "rar",
		"jpg", "jpeg", "png",
		"ogg", "mp3"
	};

	for(uint i = 0; i < ARRAY_SIZE(uncompressible_exts); i++)
	{
		if(!stricmp(ext+1, uncompressible_exts[i]))
			return true;
	}

	return false;
}


struct CompressParams
{
	bool attempt_compress;
	uintptr_t ctx;
	u32 crc;
};

#include <zlib.h>

static LibError compress_cb(uintptr_t cb_ctx, const void* block, size_t size, size_t* bytes_processed)
{
	CompressParams* p = (CompressParams*)cb_ctx;

	// comp_feed already makes note of total #bytes fed, and we need
	// vfs_io to return the uc size (to check if all data was read).
	*bytes_processed = size;

	// update checksum
	p->crc = crc32(p->crc, (const Bytef*)block, (uInt)size);

	if(p->attempt_compress)
		(void)comp_feed(p->ctx, block, size);
	return INFO_CB_CONTINUE;
}


static LibError read_and_compress_file(const char* atom_fn, uintptr_t ctx,
	ArchiveEntry& ent, void*& file_contents, FileIOBuf& buf)	// out
{
	struct stat s;
	RETURN_ERR(vfs_stat(atom_fn, &s));
	const size_t ucsize = s.st_size;

	const bool attempt_compress = !file_type_is_uncompressible(atom_fn);
	if(attempt_compress)
	{
		RETURN_ERR(comp_reset(ctx));
		RETURN_ERR(comp_alloc_output(ctx, ucsize));
	}

	// read file into newly allocated buffer. if attempt_compress, also
	// compress the file into another buffer while waiting for IOs.
	Handle hf = vfs_open(atom_fn, 0);
	RETURN_ERR(hf);
	buf = FILE_BUF_ALLOC;
	CompressParams params = { attempt_compress, ctx, 0 };
	ssize_t ucsize_read = vfs_io(hf, ucsize, &buf, compress_cb, (uintptr_t)&params);
	debug_assert(ucsize_read == (ssize_t)ucsize);
	(void)vfs_close(hf);

	// if we compressed the file trial-wise, check results and
	// decide whether to store as such or not (based on compression ratio)
	bool store_compressed = false;
	void* cdata = 0; size_t csize = 0;
	if(attempt_compress)
	{
		RETURN_ERR(comp_finish(ctx, &cdata, &csize));

		const float ratio = (float)ucsize / csize;
		const ssize_t bytes_saved = (ssize_t)ucsize - (ssize_t)csize;
		if(ratio > 1.05f && bytes_saved > 200)
			store_compressed = true;
	}

	// store file info
	ent.ucsize  = (off_t)ucsize;
	ent.mtime   = s.st_mtime;
	// .. ent.ofs is set by zip_archive_add_file
	ent.flags   = 0;
	ent.atom_fn = atom_fn;
	ent.crc32   = params.crc;
	if(store_compressed)
	{
		ent.method = CM_DEFLATE;
		ent.csize  = (off_t)csize;
		file_contents = cdata;
	}
	else
	{
		ent.method = CM_NONE;
		ent.csize  = (off_t)ucsize;
		file_contents = (void*)buf;
	}

	// note: no need to free cdata - it is owned by the
	// compression context and can be reused.

	return ERR_OK;
}

LibError build_optimized_archive(const char* trace_filename, const char* zip_filename)
{
	FileList fl;
	{
		Trace t;
		RETURN_ERR(trace_read_from_file(trace_filename, &t));
		RETURN_ERR(filelist_build(&t, &fl));
		trace_clear();
	}

	ZipArchive* za;
	RETURN_ERR(zip_archive_create(zip_filename, &za));
	uintptr_t ctx = comp_alloc(CT_COMPRESSION, CM_DEFLATE);

	const char* atom_fn;	// declare outside loop for easier debugging
	for(;;)
	{
		atom_fn = filelist_get_next(&fl);
		if(!atom_fn)
			break;

		ArchiveEntry ent; void* file_contents; FileIOBuf buf;
		if(read_and_compress_file(atom_fn, ctx, ent, file_contents, buf) == ERR_OK)
		{
			(void)zip_archive_add_file(za, &ent, file_contents);
			(void)file_buf_free(buf);
		}
	}

	filelist_free(&fl);

	// note: this is currently known to fail if there are no files in the list
	// - zlib.h says: Z_DATA_ERROR is returned if freed prematurely.
	// safe to ignore.
	comp_free(ctx);
	(void)zip_archive_finish(za);
	return ERR_OK;
}
