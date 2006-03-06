#include "precompiled.h"

#include <set>
#include <map>

#include "lib/allocators.h"
#include "lib/timer.h"
#include "file_internal.h"

#include "ps/VFSUtil.h"

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


struct FileNode
{
	const char* atom_fn;

	FileId prev_id;
	FileId next_id;
	u32 visited : 1;
	u32 output : 1;

	FileNode(const char* atom_fn_)
	{
		atom_fn = atom_fn_;

		prev_id = next_id = NULL_ID;
		visited = output = 0;
	}
};

typedef std::vector<FileNode> FileNodes;



class IdMgr
{
	FileId cur;
	typedef std::map<const char*, FileId> Map;
	Map map;
	FileNodes* nodes;

	void associate_node_with_fn(const FileNode* node, const char* atom_fn)
	{
		FileId id = id_from_node(node);
		const Map::value_type item = std::make_pair(atom_fn, id);
		std::pair<Map::iterator, bool> ret = map.insert(item);
		if(!ret.second)
			debug_warn("atom_fn already associated with node");
	}

public:
	FileId id_from_node(const FileNode* node) const
	{
		// +1 to skip NULL_ID value
		FileId id = node - &((*nodes)[0]) +1;
		debug_assert(id <= nodes->size());
		return id;
	}

	FileNode* node_from_id(FileId id) const
	{
		debug_assert(id != NULL_ID);
		return &(*nodes)[id-1];
	}

	FileId id_from_fn(const char* atom_fn) const
	{
		Map::const_iterator cit = map.find(atom_fn);
		if(cit == map.end())
		{
			debug_warn("id_from_fn: not found");
			return NULL_ID;
		}
		return cit->second;
	}

	void init(FileNodes* nodes_)
	{
		cur = NULL_ID+1;
		map.clear();
		nodes = nodes_;

		for(FileNodes::const_iterator cit = nodes->begin(); cit != nodes->end(); ++cit)
		{
			const FileNode& node = *cit;
			associate_node_with_fn(&node, node.atom_fn);
		}
	}
};
static IdMgr id_mgr;


/*
file gatherer - build vector of the complex file objects; enum via VFS
connection builder - encapsulate trace details; stitch those into series of connections
tour builder: take this list, sort it, use it to generate a nice ordering of all files
  anything not covered will be added to output list by default
*/

// build list of FileNode - exactly one per file in VFS
class FileNodeGatherer
{
	struct EntCbParams
	{
		FileNodes* file_nodes;
	};

	static void EntCb(const char* path, const DirEnt* ent, void* context)
	{
		EntCbParams* params = (EntCbParams*)context;
		FileNodes* file_nodes = params->file_nodes;

		if(!DIRENT_IS_DIR(ent))
		{
			const char* atom_fn = file_make_unique_fn_copy(path);
			file_nodes->push_back(FileNode(atom_fn));
		}
	}

public:
	FileNodeGatherer(FileNodes& file_nodes)
	{
		// jump-start allocation (avoids frequent initial reallocs)
		file_nodes.reserve(500);

		// TODO: for each mount point (with VFS_MOUNT_ARCHIVE flag set):
		EntCbParams params = { &file_nodes };
		VFSUtil::EnumDirEnts("", VFSUtil::RECURSIVE, 0, EntCb, &params);
	}
};


typedef u32 ConnectionId;
cassert(sizeof(FileId)*2 <= sizeof(ConnectionId));
ConnectionId cid_make(FileId first, FileId second)
{
	return u32_from_u16(first, second);
}
FileId cid_first(ConnectionId id)
{
	return u32_hi(id);
}
FileId cid_second(ConnectionId id)
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

typedef std::vector<Connection> Connections;

class ConnectionBuilder
{
	// put all entries in one trace file: easier to handle; obviates FS enum code
	// rationale: don't go through trace in order; instead, process most recent
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

	// note: passing i and comparing timestamp with previous timestamp
	// avoids having to keep an extra local cur_time variable.
	bool is_start_of_run(uint i, const TraceEntry* ent) const
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

	void split_trace_into_runs(const Trace* t, Runs& runs)
	{
		uint cur_run_length = 0;
		const TraceEntry* cur_entry = t->ents;
		for(uint i = 0; i < t->num_ents; i++)
		{
			cur_run_length++;
			if(is_start_of_run(i, cur_entry))
			{
				// not first time: mark previous run as complete
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

	// simulate carrying out the entry's TraceOp to determine
	// whether this IO would be satisfied by the file_buf cache.
	bool requires_actual_io(const TraceEntry* ent) const
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


	class ConnectionAdder
	{
		// we need to check before inserting a new connection if it has
		// come up before (to increment occurrences). this map speeds
		// things up from n*n to n*log(n) (n = # files).
		typedef std::map<ConnectionId, Connection*> Map;
		Map map;

		FileId prev_id;

	public:
		ConnectionAdder() : prev_id(NULL_ID) {}

		void operator()(Connections& connections, const char* new_fn)
		{
			// we connect previous node with the one identified by new_fn;
			// on first call, there's nothing to do.
			const bool was_first_call = (prev_id == NULL_ID);
			FileId id = id_mgr.id_from_fn(new_fn);
			const ConnectionId c_id = cid_make(prev_id, id);
			prev_id = id;
			if(was_first_call)
				return;

			// if this connection already exists, increment its occurence
			// count; otherwise, add it anew.
			//
			// note: checking return value of map.insert is more efficient
			// than find + insert, but requires we know next_c beforehand
			// (bit of a hack, but safe due to coonnections.reserve() above)
			Connection* next_c = &connections[0] + connections.size();
			const std::pair<ConnectionId, Connection*> item = std::make_pair(c_id, next_c);
			std::pair<Map::iterator, bool> ret = map.insert(item);
			if(!ret.second)	// already existed
			{
				Map::iterator inserted_at = ret.first;
				Connection* c = inserted_at->second;	// std::map "payload"
				c->occurrences++;
			}
			else	// first time we've seen this connection
				connections.push_back(Connection(c_id));
		}
	};

	void add_connections_from_runs(const Runs& runs, Connections& connections)
	{
		file_cache_reset();

		ConnectionAdder add_connection;

		// extract accesses from each run (starting with most recent
		// first. this isn't critical, but may help a bit since
		// files that are equally strongly 'connected' are ordered
		// according to position in file_nodes. that means files from
		// more recent traces tend to go first, which is good.)
		for(Runs::const_iterator cit = runs.begin(); cit != runs.end(); ++cit)
		{
			const Run& run = *cit;
			const TraceEntry* ent = run.first;
			for(uint i = 0; i < run.count; i++, ent++)
			{
				// improvement: postprocess the trace and remove all IOs that would be
				// satisfied by our cache. often repeated IOs would otherwise potentially
				// be arranged badly.
				if(requires_actual_io(ent))
					add_connection(connections, ent->atom_fn);
			}

			file_cache_reset();
		}
	}

public:
	ConnectionBuilder(const char* trace_filename, Connections& connections)
	{
		Trace t;
		THROW_ERR(trace_read_from_file(trace_filename, &t));

		if (t.num_ents)
		{
			// reserve memory for worst-case amount of connections (happens if
			// all accesses are unique). this is necessary because we store
			// pointers to Connection in the map, which would be invalidated if
			// connections[] ever expands.
			// may waste up to ~3x the memory (about 1mb) for a short time,
			// which is ok.
			connections.reserve(t.num_ents-1);

			Runs runs;
			split_trace_into_runs(&t, runs);

			add_connections_from_runs(runs, connections);
		}
	}
};


class TourBuilder
{
	// sort by decreasing occurrence
	struct Occurrence_greater: public std::binary_function<const Connection&, const Connection&, bool>
	{
		bool operator()(const Connection& c1, const Connection& c2) const
		{
			return (c1.occurrences > c2.occurrences);
		}
	};

	bool has_cycle;
	void detect_cycleR(FileId id)
	{
		FileNode* pnode = id_mgr.node_from_id(id);
		pnode->visited = 1;
		FileId next_id = pnode->next_id;
		if(next_id != NULL_ID)
		{
			FileNode* pnext = id_mgr.node_from_id(next_id);
			if(pnext->visited)
				has_cycle = true;
			else
				detect_cycleR(next_id);
		}
	}
	bool is_cycle_at(FileNodes& file_nodes, FileId node)
	{
		has_cycle = false;
		for(FileNodes::iterator it = file_nodes.begin(); it != file_nodes.end(); ++it)
			it->visited = 0;
		detect_cycleR(node);
		return has_cycle;
	}

	void try_add_edge(FileNodes& file_nodes, const Connection& c)
	{
		FileId first_id  = cid_first(c.id);
		FileId second_id = cid_second(c.id);

		FileNode* first  = id_mgr.node_from_id(first_id);
		FileNode* second = id_mgr.node_from_id(second_id);
		// one of them has already been hooked up - bail
		if(first->next_id != NULL_ID || second->prev_id != NULL_ID)
			return;

		first->next_id  = second_id;
		second->prev_id = first_id;

		const bool introduced_cycle = is_cycle_at(file_nodes, second_id);
		debug_assert(introduced_cycle == is_cycle_at(file_nodes, first_id));
		if(introduced_cycle)
		{
			// undo
			first->next_id = second->prev_id = NULL_ID;
			return;
		}
	}


	void output_chain(FileNode& node, std::vector<const char*>& fn_vector)
	{
		// early out: if this access was already visited, so must the entire
		// chain of which it is a part. bail to save lots of time.
		if(node.output)
			return;

		// follow prev links starting with c until no more are left;
		// start ends up the beginning of the chain including <c>.
		FileNode* start = &node;
		while(start->prev_id != NULL_ID)
			start = id_mgr.node_from_id(start->prev_id);

		// iterate over the chain - add to Filenames list and mark as visited
		FileNode* cur = start;
		for(;;)
		{
			if(!cur->output)
			{
				fn_vector.push_back(cur->atom_fn);
				cur->output = 1;
			}
			if(cur->next_id == NULL_ID)
				break;
			cur = id_mgr.node_from_id(cur->next_id);
		}
	}

public:
	TourBuilder(FileNodes& file_nodes, Connections& connections, std::vector<const char*>& fn_vector)
	{
		std::stable_sort(connections.begin(), connections.end(), Occurrence_greater());

		for(Connections::iterator it = connections.begin(); it != connections.end(); ++it)
			try_add_edge(file_nodes, *it);

		for(FileNodes::iterator it = file_nodes.begin(); it != file_nodes.end(); ++it)
			output_chain(*it, fn_vector);
	}

	// should never be copied; this also squelches warning
private:
	TourBuilder(const TourBuilder& rhs);
	TourBuilder& operator=(const TourBuilder& rhs);
};


//-----------------------------------------------------------------------------
// autobuild logic: decides when to (re)build an archive.
//-----------------------------------------------------------------------------

static const ssize_t REBUILD_MAIN_ARCHIVE_THRESHOLD = 50;
static const ssize_t BUILD_MINI_ARCHIVE_THRESHOLD = 20;

typedef std::vector<const char*> FnVector;
static FnVector loose_files;
static ssize_t loose_file_total, non_loose_file_total;

static std::set<const char*> loose;
static std::set<const char*> archive;

void vfs_opt_notify_loose_file(const char* atom_fn)
{
	loose_file_total++;

loose.insert(atom_fn);

	// only add if it's not yet clear the main archive will be
	// rebuilt anyway (otherwise we'd just waste time and memory)
	if(loose_files.size() > REBUILD_MAIN_ARCHIVE_THRESHOLD)
		loose_files.push_back(atom_fn);
}

void vfs_opt_notify_non_loose_file(const char* atom_fn)
{
archive.insert(atom_fn);

	non_loose_file_total++;
}


static bool should_rebuild_main_archive(const char* P_archive_path,
	const char* trace_filename)
{
	UNUSED2(P_archive_path);

	// if there's no trace file, no point in building a main archive.
	struct stat s;
	if(file_stat(trace_filename, &s) != ERR_OK)
		return false;
	// otherwise, if trace is up-to-date, stop recording a new one.
	const time_t vfs_mtime = tree_most_recent_mtime();
	if(s.st_mtime >= vfs_mtime)
		trace_enable(false);

	const ssize_t loose_files_only = loose_file_total - non_loose_file_total;
	if(loose_files_only >= REBUILD_MAIN_ARCHIVE_THRESHOLD)
		return true;

	// more than 3 mini archives

	// development build only: archive is more than 2 weeks old
#ifndef FINAL
#endif

	return false;
}

static bool should_build_mini_archive()
{
	const ssize_t loose_files_only = loose_file_total - non_loose_file_total;
	if(loose_files_only >= BUILD_MINI_ARCHIVE_THRESHOLD)
		return true;
	return false;
}


static ArchiveBuildState ab;
static std::vector<const char*> fn_vector;


static void vfs_opt_init(const char* P_archive_fn_fmt, const char* trace_filename)
{
	FileNodes file_nodes;
	FileNodeGatherer gatherer(file_nodes);

	id_mgr.init(&file_nodes);

	Connections connections;
	ConnectionBuilder cbuilder(trace_filename, connections);

	TourBuilder builder(file_nodes, connections, fn_vector);
	fn_vector.push_back(0);
	Filenames V_fns = &fn_vector[0];


	char archive_fn[PATH_MAX];
	static NextNumberedFilenameInfo archive_nfi;
	next_numbered_filename(P_archive_fn_fmt, &archive_nfi, archive_fn, false);
	archive_build_init(archive_fn, V_fns, &ab);
}


static int vfs_opt_continue()
{
	int ret = archive_build_continue(&ab);
	if(ret == ERR_OK)
	{
		// do NOT delete source files or archives! some apps might want to
		// keep them (e.g. for source control), or name them differently.

		// rebuild is required to make sure the new archive is used. this is
		// already taken care of by VFS dir watch, unless it's disabled..
#ifdef NO_DIR_WATCH
		(void)mount_rebuild();
#endif
	}
	return ret;
}


static enum
{
	DECIDE_IF_BUILD,
	IN_PROGRESS,
	NOP
}
state = DECIDE_IF_BUILD;

void vfs_opt_cancel()
{
	archive_build_cancel(&ab);
	state = NOP;
}


LibError vfs_opt_rebuild_main_archive(const char* P_archive_fn_fmt, const char* trace_filename)
{
	vfs_opt_init(P_archive_fn_fmt, trace_filename);
	for(;;)
	{
		int ret = vfs_opt_continue();
		RETURN_ERR(ret);
		if(ret == ERR_OK)
			return ERR_OK;
	}
}


int vfs_opt_auto_build(const char* P_dst_path,
	const char* main_archive_name_fmt, const char* trace_filename)
{
	if(state == NOP)
		return INFO_ALL_COMPLETE;

	if(state == DECIDE_IF_BUILD)
	{
		char P_archive_fn_fmt[PATH_MAX];
		(void)vfs_path_append(P_archive_fn_fmt, P_dst_path, main_archive_name_fmt);
		if(should_rebuild_main_archive(P_archive_fn_fmt, trace_filename))
		{
			vfs_opt_init(P_archive_fn_fmt, trace_filename);
			state = IN_PROGRESS;
		}
		else
		{
			// note: only think about building mini archive if not rebuilding
			// the main archive.
			if(should_build_mini_archive())
			{
				Filenames V_fns = (Filenames)malloc((loose_files.size()+1) * sizeof(const char*));
				if(!V_fns)
					return ERR_NO_MEM;
				std::copy(loose_files.begin(), loose_files.end(), &V_fns[0]);
				V_fns[loose_files.size()] = 0;	// terminator

				// get new unused mini archive name at P_dst_path
				char mini_archive_fn[VFS_MAX_PATH];
				char fn_fmt[VFS_MAX_PATH];
				(void)vfs_path_append(fn_fmt, P_dst_path, "mini%d.zip");
				static NextNumberedFilenameInfo nfi;
				next_numbered_filename(fn_fmt, &nfi, mini_archive_fn, false);

				RETURN_ERR(archive_build(mini_archive_fn, V_fns));
			}

			state = NOP;
			return ERR_OK;	// "finished"
		}
	}

	if(state == IN_PROGRESS)
	{
		int ret = vfs_opt_continue();
		if(ret == ERR_OK)
			state = NOP;
		return ret;
	}

	UNREACHABLE;
	return ERR_OK; // To quelch gcc "control reaches end of non-void function"
}
