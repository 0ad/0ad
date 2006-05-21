/**
 * =========================================================================
 * File        : trace.cpp
 * Project     : 0 A.D.
 * Description : allows recording and 'playing back' a sequence of
 *             : I/Os - useful for benchmarking and archive builder.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2006 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

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


static void trace_add(TraceOp op, const char* P_fn, size_t size,
	uint flags = 0, double timestamp = 0.0)
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

static void trace_get_raw_ents(const TraceEntry*& ents, size_t& num_ents)
{
	ents = (const TraceEntry*)trace_pool.da.base;
	num_ents = (uint)(trace_pool.da.pos / sizeof(TraceEntry));
}


void trace_notify_io(const char* P_fn, size_t size, uint flags)
{
	trace_add(TO_IO, P_fn, size, flags);
}

void trace_notify_free(const char* P_fn, size_t size)
{
	trace_add(TO_FREE, P_fn, size);
}


//-----------------------------------------------------------------------------

// put all entries in one trace file: easier to handle; obviates FS enum code
// rationale: don't go through trace in order; instead, process most recent
// run first, to give more weight to it (TSP code should go with first entry
// when #occurrences are equal)


static const TraceEntry delimiter_entry =
{
	0.0f,	// timestamp
	"------------------------------------------------------------",
	0,		// size
	TO_IO,	// TraceOp (never seen by user; value doesn't matter)
	0		// flags
};

// storage for Trace.runs.
static const uint MAX_RUNS = 100;
static TraceRun runs[MAX_RUNS];

// note: the last entry may be one past number of actual entries.
static std::vector<size_t> run_start_indices;

class DelimiterAdder
{
public:
	enum Consequence
	{
		SKIP_ADD,
		CONTINUE
	};
	Consequence operator()(size_t i, double timestamp, const char* P_path)
	{
		// this entry is a delimiter
		if(!strcmp(P_path, delimiter_entry.atom_fn))
		{
			run_start_indices.push_back(i+1);	// skip this entry
			// note: its timestamp is invalid, so don't set cur_timestamp!
			return SKIP_ADD;
		}

		const double last_timestamp = cur_timestamp;
		cur_timestamp = timestamp;

		// first item is always start of a run
		if((i == 0) ||
		// timestamp started over from 0 (e.g. 29, 30, 1) -> start of new run.
		   (timestamp < last_timestamp))
			run_start_indices.push_back(i);

		return CONTINUE;
	}
private:
	double cur_timestamp;
};


//-----------------------------------------------------------------------------


void trace_get(Trace* t)
{
	const TraceEntry* ents; size_t num_ents;
	trace_get_raw_ents(ents, num_ents);

	// nobody had split ents up into runs; just create one big 'run'.
	if(run_start_indices.empty())
		run_start_indices.push_back(0);

	t->runs = runs;
	t->num_runs = 0;	// counted up
	t->total_ents = num_ents;

	size_t last_start_idx = num_ents;

	std::vector<size_t>::reverse_iterator it;
	for(it = run_start_indices.rbegin(); it != run_start_indices.rend(); ++it)
	{
		const size_t start_idx = *it;
		// run_start_indices.back() may be = num_ents (could happen if
		// a zero-length run gets written out); skip that to avoid
		// zero-length run here.
		if(last_start_idx == start_idx)
			continue;

		TraceRun& run = runs[t->num_runs++];
		run.num_ents = last_start_idx - start_idx;
		run.ents = &ents[start_idx];
		last_start_idx = start_idx;

		if(t->num_runs == MAX_RUNS)
			break;
	}

	debug_assert(t->num_runs != 0);
}

void trace_clear()
{
	pool_free_all(&trace_pool);
	run_start_indices.clear();
	memset(runs, 0, sizeof(runs));	// for safety
}

//-----------------------------------------------------------------------------



static void write_entry(FILE* f, const TraceEntry* ent)
{
	char opcode = '?';
	switch(ent->op)
	{
	case TO_IO: opcode = 'L'; break;
	case TO_FREE: opcode = 'F'; break;
	default: debug_warn("invalid TraceOp");
	}

	debug_assert(ent->op == TO_IO || ent->op == TO_FREE);
	fprintf(f, "%#010f: %c \"%s\" %d %04x\n", ent->timestamp, opcode,
		ent->atom_fn, ent->size, ent->flags);
}


// *appends* entire current trace contents to file (with delimiter first)
LibError trace_write_to_file(const char* trace_filename)
{
	if(!trace_enabled)
		return INFO_SKIPPED;

	char N_fn[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(trace_filename, N_fn));
	// append at end of file, otherwise we'd only have the most
	// recently stored trace. vfs_optimizer correctly deals with
	// several trace runs per file.
	FILE* f = fopen(N_fn, "at");
	if(!f)
		WARN_RETURN(ERR_FILE_ACCESS);

	write_entry(f, &delimiter_entry);

	// somewhat of a hack: write all entries in original order, not the
	// reverse order returned by trace_get.
	const TraceEntry* ent; size_t num_ents;
	trace_get_raw_ents(ent, num_ents);
	for(size_t i = 0; i < num_ents; i++, ent++)
		write_entry(f, ent);

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
		WARN_RETURN(ERR_TNODE_NOT_FOUND);

	// we use trace_add, which is the same mechanism called by trace_notify*;
	// therefore, tracing needs to be enabled.
	trace_enabled = true;

	DelimiterAdder delim_adder;

	// parse lines and stuff them in trace_pool
	// (as if they had been trace_add-ed; replaces any existing data)
	// .. bake PATH_MAX limit into string.
	char fmt[30];
	snprintf(fmt, ARRAY_SIZE(fmt), "%%lf: %%c \"%%%d[^\"]\" %%d %%04x\n", PATH_MAX);
	for(size_t i = 0; ; i++)
	{
		double timestamp; char opcode; char P_path[PATH_MAX]; size_t size; uint flags;
		int ret = fscanf(f, fmt, &timestamp, &opcode, P_path, &size, &flags);
		if(ret == EOF)
			break;
		debug_assert(ret == 5);

		TraceOp op = TO_IO;	// default in case file is garbled
		switch(opcode)
		{
		case 'L': op = TO_IO; break;
		case 'F': op = TO_FREE; break;
		default: debug_warn("invalid TraceOp");
		}

		if(delim_adder(i, timestamp, P_path) != DelimiterAdder::SKIP_ADD)
			trace_add(op, P_path, size, flags, timestamp);
	}

	fclose(f);

	trace_get(t);

	// all previous trace entries were hereby lost (overwritten),
	// so there's no sense in continuing.
	trace_enabled = false;

	if(t->total_ents == 0)
		WARN_RETURN(ERR_TRACE_EMPTY);

	return ERR_OK;
}


void trace_gen_random(size_t num_entries)
{
	trace_clear();

	for(size_t i = 0; i < num_entries; i++)
	{
		// generate random names until we get a valid file;
		// remember its name and size.
		const char* atom_fn;
		off_t size;
		for(;;)
		{
			atom_fn = file_get_random_name();
			// use instead of vfs_stat to avoid warnings, since some of
			// atom_fn will actually be directory names.
			if(vfs_exists(atom_fn))
			{
				struct stat s;
				LibError ret = vfs_stat(atom_fn, &s);
				// ought to apply due to vfs_exists above.
				debug_assert(ret == ERR_OK && S_ISREG(s.st_mode));

				size = s.st_size;
				break;
			}
		}

		trace_add(TO_IO, atom_fn, size);
		trace_add(TO_FREE, atom_fn, size);
	}
}


//-----------------------------------------------------------------------------

// simulate carrying out the entry's TraceOp to determine
// whether this IO would be satisfied by the file_buf cache.
//
// note: TO_IO's handling of uncached buffers means the simulated and
// real cache contents will diverge if the real caller doesn't free their
// buffer immediately.
// this is a bit of a bother, but only slightly influences results
// because it works by affecting the cache allocator's eviction pattern.
// alternatives:
// - only allocate if file_cache_would_add. this would actually
//   cause divergence whenever skipping any allocation, which is worse.
// - maintain a list of "buffers we allocated" and use that instead of
//   file_cache_retrieve in TO_FREE. this would keep both caches in sync but
//   add considerable complexity (function would no longer be "stateless").
bool trace_entry_causes_io(const TraceEntry* ent)
{
	uint fb_flags = FB_NO_STATS;
	if(ent->flags & FILE_LONG_LIVED)
		fb_flags |= FB_LONG_LIVED;

	FileIOBuf buf;
	size_t size         = ent->size;
	const char* atom_fn = ent->atom_fn;
	uint file_flags     = ent->flags;
	switch(ent->op)
	{
	case TO_IO:
	{
		// we're not interested in writes
		if(file_flags & FILE_WRITE)
			return false;
		buf = file_cache_retrieve(atom_fn, &size, fb_flags);
		// would not be in cache
		if(!buf)
		{
			buf = file_buf_alloc(size, atom_fn, fb_flags);
			LibError ret = file_cache_add(buf, size, atom_fn, file_flags);
			// the cache decided not to add buf (see file_cache_would_add).
			// since TO_FREE below uses the cache to find out which
			// buffer was allocated for atom_fn, we have to free it manually.
			// see note above.
			if(ret == INFO_SKIPPED)
				(void)file_buf_free(buf, fb_flags);
			return true;
		}
		break;
	}
	case TO_FREE:
		buf = file_cache_retrieve(atom_fn, &size, fb_flags|FB_NO_ACCOUNTING);
		// note: if buf == 0, file_buf_free is a no-op. this happens in the
		// abovementioned cached-at-higher-level case.
		(void)file_buf_free(buf, fb_flags);
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
	const double first_timestamp = t.runs[t.num_runs-1].ents[0].timestamp;

	for(uint r = 0; r < t.num_runs; r++)
	{
		const TraceRun& run = t.runs[r];
		const TraceEntry* ent = run.ents;
		for(uint i = 0; i < run.num_ents; i++, ent++)
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
			case TO_IO:
				// do not 'run' writes - we'd destroy the existing data.
				if(ent->flags & FILE_WRITE)
					continue;
				(void)vfs_load(ent->atom_fn, buf, size, ent->flags);
				break;
			case TO_FREE:
				buf = file_cache_retrieve(ent->atom_fn, &size, FB_NO_STATS|FB_NO_ACCOUNTING);
				(void)file_buf_free(buf);
				break;
			default:
				debug_warn("unknown TraceOp");
			}
		}
	}

	trace_clear();

	return ERR_OK;
}

