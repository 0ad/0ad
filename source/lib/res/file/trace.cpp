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


// enabled by default. by the time we can decide whether a trace needs to
// be generated (see should_rebuild_main_archive), file accesses will
// already have occurred; hence default enabled and disable if not needed.
static bool trace_enabled = true;
static bool trace_force_enabled = false;	// see below

// note: explicitly enabling trace means the user wants one to be
// generated even if an up-to-date version exists.
// (mechanism: ignore any attempts to disable)
void trace_enable(bool want_enabled)
{
	trace_enabled = want_enabled;

	if(want_enabled)
		trace_force_enabled = true;
	if(trace_force_enabled)
		trace_enabled = true;
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
	if(!trace_enabled)
		return INFO_SKIPPED;

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
	trace_clear();

	char N_fn[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(trace_filename, N_fn));
	FILE* f = fopen(N_fn, "rt");
	if(!f)
		WARN_RETURN(ERR_FILE_NOT_FOUND);

	// we use trace_add, which is the same mechanism called by trace_notify*;
	// therefore, tracing needs to be enabled.
	trace_enabled = true;

	// parse lines and stuff them in trace_pool
	// (as if they had been trace_add-ed; replaces any existing data)
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


//-----------------------------------------------------------------------------

// simulate carrying out the entry's TraceOp to determine
// whether this IO would be satisfied by the file_buf cache.
bool trace_entry_causes_io(const TraceEntry* ent)
{
	FileIOBuf buf;
	size_t size         = ent->size;
	const char* atom_fn = ent->atom_fn;
	switch(ent->op)
	{
	case TO_LOAD:
	{
		bool long_lived = (ent->flags & FILE_LONG_LIVED) != 0;
		buf = file_cache_retrieve(atom_fn, &size, long_lived);
		// would not be in cache: add to list of real IOs
		if(!buf)
		{
			buf = file_buf_alloc(size, atom_fn, long_lived);
			(void)file_cache_add(buf, size, atom_fn);
			return true;
		}
		break;
	}
	case TO_FREE:
		buf = file_cache_find(atom_fn, &size);
		(void)file_buf_free(buf);
		break;
	default:
		debug_warn("unknown TraceOp");
	}

	return false;
}


// carry out all operations specified in the trace.
// if flags&TRF_SYNC_TO_TIMESTAMP, waits until timestamp for each event is
// reached; otherwise, they are run as fast as possible.
LibError trace_run(const char* trace_filename, uint flags)
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
		if(flags & TRF_SYNC_TO_TIMESTAMP)
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

