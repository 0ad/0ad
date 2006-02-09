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



// enough for 64K unique files - ought to suffice.
typedef u16 FileId;
static const FileId NULL_ID = 0;

class IdMgr
{
	FileId cur;
	typedef std::map<const char*, FileId> Map;
	Map map;
public:
	FileId get(const char* atom_fn)
	{
		Map::iterator it = map.find(atom_fn);
		if(it != map.end())
			return it->second;
		FileId id = cur++;
		map[atom_fn] = id;
		return id;
	}
	void reset() { cur = NULL_ID+1; }
	IdMgr() { reset(); }
};
static IdMgr id_mgr;


struct FileAccess
{
	const char* atom_fn;
	FileId id;

	FileId prev;
	FileId next;
	bool visited;

	FileAccess(const char* atom_fn_)
	{
		atom_fn = atom_fn_;
		prev = next = NULL_ID;
		id = id_mgr.get(atom_fn);
	}
};

typedef std::vector<FileAccess> FileAccesses;

class FileGatherer
{
	// put all entries in one trace file: easier to handle; obviates FS enum code
	// rationale: don't go through file in order; instead, process most recent
	// run first, to give more weight to it (TSP code should go with first entry
	// when #occurrences are equal)
	struct Run
	{
		const TraceEntry* first;
		uint count;

		// note: up to caller to initialize count (that's done when
		// starting the next run
		Run(const TraceEntry* first_) : first(first_) {}
	};

	FileAccesses& file_accesses;

	// improvement: postprocess the trace and remove all IOs that would be
	// satisfied by our cache. often repeated IOs would otherwise potentially
	// be arranged badly.
	void extract_accesses_from_run(const Run& run)
	{
		file_cache_flush();

		const TraceEntry* ent = run.first;
		for(uint i = 0; i < run.count; i++, ent++)
		{
			// simulate carrying out the entry's TraceOp to determine
			// whether this IO would be satisfied by the file_buf cache.
			FileIOBuf buf;
			size_t size         = ent->size;
			const char* atom_fn = ent->atom_fn;
			switch(ent->op)
			{
			case TO_LOAD:
				buf = file_cache_retrieve(atom_fn, &size);
				// would not be in cache: add to list of real IOs
				if(!buf)
				{
					bool long_lived = (ent->flags & FILE_LONG_LIVED) != 0;
					buf = file_buf_alloc(size, atom_fn, long_lived);
					(void)file_cache_add(buf, size, atom_fn);

					file_accesses.push_back(atom_fn);
				}
				break;
			case TO_FREE:
				buf = file_cache_find(atom_fn, &size);
				(void)file_buf_free(buf);
				break;
			default:
				debug_warn("unknown TraceOp");
			}
		}	// foreach entry

		file_cache_flush();
	}


	// note: passing i and comparing timestamp with previous timestamp
	// avoids having to keep an extra local cur_time variable.
	bool is_start_of_run(uint i, const TraceEntry* ent)
	{
		// first item is always start of a run (protects [-1] below)
		if(i == 0)
			return true;

		// timestamp started over from 0 (e.g. 29, 30, 1) -> start of new run.
		if(ent->timestamp < ent[-1].timestamp)
			return true;

		return false;
	}

	typedef std::vector<Run> Runs;
	Runs runs;
	void split_trace_into_runs(const Trace* t)
	{
		uint cur_run_length = 0;
		const TraceEntry* cur_entry = t->ents;
		for(uint i = 0; i < t->num_ents; i++)
		{
			cur_run_length++;
			if(is_start_of_run(i, cur_entry))
			{
				if(!runs.empty())
					runs.back().count = cur_run_length;
				cur_run_length = 0;
				runs.push_back(Run(cur_entry));
			}
			cur_entry++;
		}
		// set the last run's length
		if(!runs.empty())
			runs.back().count = cur_run_length;
	}

public:
	FileGatherer(const char* trace_filename, Filenames required_fns,
		FileAccesses& file_accesses_)
		: file_accesses(file_accesses_)
	{
		Trace t;
		if(trace_read_from_file(trace_filename, &t) == 0)
		{
			split_trace_into_runs(&t);
			// extract accesses from each run (starting with most recent
			// first. this isn't critical, but may help a bit since
			// files that are equally strongly 'connected' are ordered
			// according to position in file_accesses. that means files from
			// more recent traces tend to go first, which is good.)
			for(Runs::iterator it = runs.begin(); it != runs.end(); ++it)
				extract_accesses_from_run(*it);
		}

		// add all remaining files that are to be put in archive
		for(uint i = 0; required_fns[i] != 0; i++)
			file_accesses.push_back(required_fns[i]);
	}

	// should never be copied; this also squelches warning
private:
	FileGatherer(const FileGatherer& rhs);
	FileGatherer& operator=(const FileGatherer& rhs);
};


class TourBuilder
{
	typedef u32 ConnectionId;
	cassert(sizeof(FileId)*2 <= sizeof(ConnectionId));
	ConnectionId cid_make(FileId prev, FileId next)
	{
		return u32_from_u16(prev, next);
	}
	FileId cid_prev(ConnectionId id)
	{
		return u32_hi(id);
	}
	FileId cid_next(ConnectionId id)
	{
		return u32_lo(id);
	}

	struct Connection
	{
		ConnectionId id;
		// repeated edges ("connections") are reflected in
		// the 'occurrences' count; we optimize the ordering so that
		// files with frequent connections are nearby.
		uint occurrences;

		Connection(ConnectionId id_)
			: id(id_), occurrences(1) {}
	};

	struct decreasing_occurrences: public std::binary_function<const Connection&, const Connection&, bool>
	{
		bool operator()(const Connection& c1, const Connection& c2) const
		{
			return (c1.occurrences > c2.occurrences);
		}
	};

	typedef std::vector<Connection> Connections;
	Connections connections;

	// not const because we change the graph-related members
	FileAccesses& file_accesses;

	void build_connections()
	{
		// reserve memory for worst-case amount of connections (happens if
		// all accesses are unique). this is necessary because we store
		// pointers to Connection in the map, which would be invalidated if
		// connections[] ever expands.
		connections.reserve(file_accesses.size()-1);

		// we need to check before inserting a new connection if it has
		// come up before (to increment occurrences). this map speeds
		// things up from n*n to n*log(n) (n = # files).
		typedef std::map<ConnectionId, Connection*> Map;
		Map map;

		// for each file pair (i-1, i): set up a Connection
		for(uint i = 1; i < file_accesses.size(); i++)
		{
			const ConnectionId c_id = cid_make(file_accesses[i-1].id, file_accesses[i].id);

			Map::iterator it = map.find(c_id);
			if(it != map.end())
				it->second->occurrences++;
			else
			{
				connections.push_back(Connection(c_id));
				map[c_id] = &connections.back();
			}
		}
	}

	bool has_cycle;
	void detect_cycleR(FileId node)
	{
		FileAccess* pnode = &file_accesses[node];
		pnode->visited = true;
		FileId next = pnode->next;
		if(next != NULL_ID)
		{
			FileAccess* pnext = &file_accesses[next];
			if(pnext->visited)
				has_cycle = true;
			else
				detect_cycleR(next);
		}
	}
	bool is_cycle_at(FileId node)
	{
		has_cycle = false;
		for(FileAccesses::iterator it = file_accesses.begin(); it != file_accesses.end(); ++it)
			it->visited = 0;
		detect_cycleR(node);
		return has_cycle;
	}

	void add_edge(const Connection& c)
	{
		FileId prev_id = cid_prev(c.id);
		FileId next_id = cid_next(c.id);

		FileAccess& prev = file_accesses[prev_id];
		FileAccess& next = file_accesses[next_id];
		if(prev.next != NULL_ID || next.prev != NULL_ID)
			return;

		prev.next = next_id;
		next.prev = prev_id;

		bool introduced_cycle = is_cycle_at(next_id);
		debug_assert(introduced_cycle == is_cycle_at(prev_id));
		if(introduced_cycle)
		{
			// undo
			prev.next = next.prev = NULL_ID;
			return;
		}
	}

public:
	TourBuilder(FileAccesses& file_accesses_)
		: file_accesses(file_accesses_)
	{
		build_connections();
		std::sort(connections.begin(), connections.end(), decreasing_occurrences());
		for(Connections::iterator it = connections.begin(); it != connections.end(); ++it)
			add_edge(*it);


		// walk tour; make sure all nodes are covered
		// add each one to FileList
	}

	// should never be copied; this also squelches warning
private:
	TourBuilder(const TourBuilder& rhs);
	TourBuilder& operator=(const TourBuilder& rhs);
};


/*

static LibError determine_optimal_ordering(const char* trace_filename, Filenames* pfns)
{


	*pfl = 0;


	// count # files
	uint num_files = 0;
	for(size_t i = 0; i < t.num_ents; i++)
		if(t.ents[i].op == TO_LOAD)
			num_files++;

	if(!num_files)
		return ERR_DIR_END;

	Filenames fns = (Filenames)malloc((num_files+1)*sizeof(const char*));
	if(!fns)
		return ERR_NO_MEM;

	size_t ti = 0;
	for(size_t i = 0; i < num_files; i++)
	{
		// find next trace entry that is a load (must exist)
		while(t.ents[ti].op != TO_LOAD)
			ti++;
		fns[i] = t.ents[ti].atom_fn;
		ti++;
	}

	trace_clear();
	*pfl = fl;
	return ERR_OK;
}
*/

//-----------------------------------------------------------------------------

typedef std::vector<const char*> FnVector;
static FnVector loose_files;

void vfs_opt_notify_loose_file(const char* atom_fn)
{
	// we could stop adding to loose_files if it's already got more than
	// REBUILD_MAIN_ARCHIVE_THRESHOLD entries, but don't bother
	// (it's ok to waste a bit of mem - this is rare)

	loose_files.push_back(atom_fn);
}


LibError vfs_opt_rebuild_main_archive(const char* P_archive_dst_dir)
{
debug_warn("currently non-functional");

	// for each mount point (with VFS_MOUNT_ARCHIVE flag set):
	// get list of all files
	Filenames required_fns = 0;

	FileAccesses file_accesses;
	FileGatherer gatherer("../logs/trace.txt", required_fns, file_accesses);

	TourBuilder builder(file_accesses);
//	builder.store_list(pfns);
	Filenames fns = 0;
// (Filenames)malloc((num_files+1)*sizeof(const char*));
//	if(!fns)
//		return ERR_NO_MEM;

	char P_path[VFS_MAX_PATH];
	RETURN_ERR(vfs_path_append(P_path, P_archive_dst_dir, "main.zip"));
	LibError ret = archive_build("main.zip", fns);
	// delete all loose files in list
	free(fns);
	// delete all archives in P_dst_path
	return ret;

}


//
// autobuild logic: decides when to (re)build an archive.
//

static const size_t REBUILD_MAIN_ARCHIVE_THRESHOLD = 100;
static const size_t BUILD_MINI_ARCHIVE_THRESHOLD = 30;

static bool should_rebuild_main_archive()
{
	if(loose_files.size() >= REBUILD_MAIN_ARCHIVE_THRESHOLD)
		return true;

	// more than 3 mini archives

	// development build only: archive is more than 2 weeks old
#ifndef FINAL
#endif

	return false;
}

static bool should_build_mini_archive()
{
	if(loose_files.size() >= BUILD_MINI_ARCHIVE_THRESHOLD)
		return true;
	return false;
}


LibError vfs_opt_auto_build_archive(const char* P_dst_path)
{
	if(should_rebuild_main_archive())
		return vfs_opt_rebuild_main_archive(P_dst_path);
	else if(should_build_mini_archive())
	{
		loose_files.push_back(0);
		// get new unused mini archive name
		const char* archive_filename = 0;
		RETURN_ERR(archive_build(archive_filename, &loose_files[0]));
		// delete all newly added loose files
	}

	return ERR_OK;
}
