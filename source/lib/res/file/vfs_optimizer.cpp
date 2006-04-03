#include "precompiled.h"

#include <set>
#include <map>

#include "file_internal.h"

#include "ps/VFSUtil.h"

// enough for 64K unique files - ought to suffice.
typedef u16 FileId;
static const FileId NULL_ID = 0;
static const size_t MAX_IDS = 0x10000 -1;	// -1 due to NULL_ID


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

//-----------------------------------------------------------------------------

// check if the file is supposed to be added to archive.
// this avoids adding e.g. screenshots (wasteful because they're never used)
// or config (bad because they are written to and that's not supported for
// archived files).
static bool is_archivable(const TFile* tf)
{
	const Mount* m = tfile_get_mount(tf);
	return mount_is_archivable(m);
}

class IdMgr
{
	FileId cur;
	typedef std::map<const char*, FileId> Map;
	Map map;
	FileNodes* nodes;

	// dummy return value so this can be called via for_each/mem_fun_ref
	void associate_node_with_fn(const FileNode& node)
	{
		FileId id = id_from_node(&node);
		const Map::value_type item = std::make_pair(node.atom_fn, id);
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

		// can't use for_each (mem_fun requires const function and
		// non-reference-type argument)
		for(FileNodes::const_iterator cit = nodes->begin(); cit != nodes->end(); ++cit)
		{
			const FileNode& node = *cit;
			associate_node_with_fn(node);
		}
	}
};
static IdMgr id_mgr;

//-----------------------------------------------------------------------------

// build list of FileNode - exactly one per file in VFS.
//
// time cost: 13ms for 5500 files; we therefore do not bother with
// optimizations like reading from vfs_tree container directly.
class FileGatherer
{
	static void EntCb(const char* path, const DirEnt* ent, void* context)
	{
		FileNodes* file_nodes = (FileNodes*)context;

		// we only want files
		if(DIRENT_IS_DIR(ent))
			return;

		if(is_archivable(ent->tf))
		{
			const char* atom_fn = file_make_unique_fn_copy(path);
			file_nodes->push_back(FileNode(atom_fn));
		}
	}

public:
	FileGatherer(FileNodes& file_nodes)
	{
		// jump-start allocation (avoids frequent initial reallocs)
		file_nodes.reserve(500);

		// TODO: only add entries from mount points that have
		// VFS_MOUNT_ARCHIVE flag set (avoids adding screenshots etc.)
		VFSUtil::EnumDirEnts("", VFSUtil::RECURSIVE, 0, EntCb, &file_nodes);

		// MAX_IDS is a rather large limit on number of files, but must not
		// be exceeded (otherwise FileId overflows).
		// check for this here and not in EntCb because it's not
		// expected to happen.
		if(file_nodes.size() > MAX_IDS)
		{
			// note: use this instead of resize because FileNode doesn't have
			// a default ctor. NB: this is how resize is implemented anyway.
			file_nodes.erase(file_nodes.begin() + MAX_IDS, file_nodes.end());
			WARN_ERR(ERR_LIMIT);
		}
	}
};

//-----------------------------------------------------------------------------

typedef u32 ConnectionId;
cassert(sizeof(FileId)*2 <= sizeof(ConnectionId));
static ConnectionId cid_make(FileId first, FileId second)
{
	return u32_from_u16(first, second);
}
static FileId cid_first(ConnectionId id)
{
	return u32_hi(id);
}
static FileId cid_second(ConnectionId id)
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

// builds a list of Connection-s (basically edges in the FileNode graph)
// defined by the trace.
//
// time cost: 70ms for 1000 trace entries. this is rather heavy;
// the main culprit is simulating file_cache to see if an IO would result.
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


	// functor: on every call except the first, adds a connection between
	// the previous file (remembered here) and the current file.
	// if the connection already exists, its occurrence count is incremented.
	class ConnectionAdder
	{
		// speeds up "already exists" overhead from n*n to n*log(n).
		typedef std::map<ConnectionId, Connection*> Map;
		typedef std::pair<ConnectionId, Connection*> MapItem;
		typedef Map::const_iterator MapCIt;
		Map map;

		FileId prev_id;

	public:
		ConnectionAdder() : prev_id(NULL_ID) {}

		void operator()(Connections& connections, const char* new_fn)
		{
			const bool was_first_call = (prev_id == NULL_ID);
			FileId id = id_mgr.id_from_fn(new_fn);
			const ConnectionId c_id = cid_make(prev_id, id);
			prev_id = id;

			if(was_first_call)
				return;	// bail after setting prev_id

			// note: always insert-ing and checking return value would be
			// more efficient (saves 1 iteration over map), but would not
			// be safe: VC8's STL disallows &vector[0] if empty
			// (even though memory has been reserved).
			// it doesn't matter much anyway (decently fast and offline task).
			MapCIt it = map.find(c_id);
			const bool already_exists = (it != map.end());
			if(already_exists)
			{
				Connection* c = it->second;	// Map "payload"
				c->occurrences++;
			}
			// seen this connection for the first time: add to map and list.
			else
			{
				connections.push_back(Connection(c_id));
				const MapItem item = std::make_pair(c_id, &connections.back());
				map.insert(item);
			}

			stats_ab_connection(already_exists);
		}
	};

	void add_connections_from_runs(const Runs& runs, Connections& connections)
	{
		file_cache_reset();

		// (note: lifetime = entire connection build process; if re-created
		// in between, entries in Connections will no longer be unique,
		// which may break TourBuilder)
		ConnectionAdder add_connection;

		// extract accesses from each run (starting with most recent
		// first. this isn't critical, but may help a bit since
		// files that are equally strongly 'connected' are ordered
		// according to position in file_nodes. that means files from
		// more recent traces tend to go first, which is good.)
		for(Runs::const_reverse_iterator it = runs.rbegin(); it != runs.rend(); ++it)
		{
			const Run& run = *it;
			for(uint i = 0; i < run.count; i++)
			{
				const TraceEntry* te = run.first + i;
				// improvement: postprocess the trace and remove all IOs that would be
				// satisfied by our cache. often repeated IOs would otherwise potentially
				// be arranged badly.
				if(trace_entry_causes_io(te))
				{
					// only add connection if this file exists and is in
					// file_nodes list. otherwise, ConnectionAdder's
					// id_from_fn call will fail.
					// note: this happens when trace contains by now
					// deleted or unarchivable files.
					TFile* tf;
					if(tree_lookup(te->atom_fn, &tf) == ERR_OK)
						if(is_archivable(tf))
							add_connection(connections, te->atom_fn);
				}
			}

			file_cache_reset();
		}
	}

public:
	LibError run(const char* trace_filename, Connections& connections)
	{
		Trace t;
		RETURN_ERR(trace_read_from_file(trace_filename, &t));

		if(!t.num_ents)
			WARN_RETURN(ERR_TRACE_EMPTY);

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

		return ERR_OK;
	}
};

//-----------------------------------------------------------------------------

// given graph and known edges, stitch together FileNodes so that
// Hamilton tour (TSP solution) length of the graph is minimized.
// heuristic is greedy adding edges sorted by decreasing 'occurrences'.
//
// time cost: 7ms for 1000 connections; quite fast despite DFS.
//
// could be improved (if there are lots of files) by storing in each node
// a pointer to end of list; if adding a new edge, check if end.endoflist
// is the start of edge.
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
#ifndef NDEBUG
		debug_assert(introduced_cycle == is_cycle_at(file_nodes, first_id));
#endif
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
};


//-----------------------------------------------------------------------------
// autobuild logic: decides when to (re)build an archive.
//-----------------------------------------------------------------------------

// for each loose or archived file encountered during mounting: add to a
// std::set; if there are more than *_THRESHOLD non-archived files, rebuild.
// this ends up costing 50ms for 5000 files, so disable it in final release.
#ifndef FINAL
# define AB_COUNT_LOOSE_FILES 1
#else
# define AB_COUNT_LOOSE_FILES 0
#endif
// rebuild if the archive is much older than most recent VFS timestamp.
// this makes sense during development: the archive will periodically be
// rebuilt with the newest trace. however, it would be annoying in the
// final release, where users will frequently mod things, which should not
// end up rebuilding the main archive.
#ifndef FINAL
# define AB_COMPARE_MTIME 1
#else
# define AB_COMPARE_MTIME 0
#endif

#if AB_COUNT_LOOSE_FILES
static const ssize_t REBUILD_MAIN_ARCHIVE_THRESHOLD = 50;
static const ssize_t BUILD_MINI_ARCHIVE_THRESHOLD = 20;

typedef std::set<const char*> FnSet;
static FnSet loose_files;
static FnSet archived_files;
#endif

void vfs_opt_notify_loose_file(const char* atom_fn)
{
#if AB_COUNT_LOOSE_FILES
	// note: files are added before archives, so we can't stop adding to
	// set after one of the above thresholds are reached.
	loose_files.insert(atom_fn);
#endif
}

void vfs_opt_notify_non_loose_file(const char* atom_fn)
{
#if AB_COUNT_LOOSE_FILES
	archived_files.insert(atom_fn);
#endif
}


// functor called for every entry in root directory. counts # archives and
// remembers most recent modification time (directly accessible).
class ArchiveScanner
{
	const char* archive_ext;

public:
	time_t most_recent_archive_mtime;
	uint num_archives;

	ArchiveScanner(const char* archive_fn_fmt)
	{
		archive_ext = strrchr(archive_fn_fmt, '.');
		debug_assert(archive_ext);

		most_recent_archive_mtime = 0;
		num_archives = 0;
	}

	void operator()(const DirEnt& ent)
	{
		// only interested in files
		if(DIRENT_IS_DIR(&ent))
			return;

		// same extension, i.e. is also archive
		const char* ext = strrchr(ent.name, '.');
		if(ext && !strcmp(archive_ext, ext))
		{
			most_recent_archive_mtime = MAX(ent.mtime, most_recent_archive_mtime);
			num_archives++;
		}
	}
};

static bool should_rebuild_main_archive(const char* trace_filename,
	const char* archive_fn_fmt, DirEnts& dirents)
{
	// if there's no trace file, no point in building a main archive.
	// (we wouldn't know how to order the files)
	if(!file_exists(trace_filename))
		return false;

#if AB_COUNT_LOOSE_FILES
	// too many (eligible for archiving!) loose files not in archive: rebuild.
	const ssize_t loose_files_only = (ssize_t)loose_files.size() - (ssize_t)archived_files.size();
	if(loose_files_only >= REBUILD_MAIN_ARCHIVE_THRESHOLD)
		return true;
#endif

	// scan dir and see what archives are already present..
	{
	ArchiveScanner archive_scanner(archive_fn_fmt);
	// note: a loop is more convenient than std::for_each, which would
	// require referencing the returned functor (since param is a copy).
	for(DirEnts::const_iterator it = dirents.begin(); it != dirents.end(); ++it)
		archive_scanner(*it);
	// .. no archive yet OR 'lots' of them: rebuild so that they'll be
	//    merged into one archive and the rest deleted.
	if(archive_scanner.num_archives == 0 || archive_scanner.num_archives >= 4)
		return true;
#if AB_COMPARE_MTIME
	// .. archive is much older than most recent data: rebuild.
	const double max_diff = 14*86400;	// 14 days
	if(difftime(tree_most_recent_mtime(), archive_scanner.most_recent_archive_mtime) > max_diff)
		return true;
#endif
	}

	return false;
}

//-----------------------------------------------------------------------------


static ArchiveBuildState ab;
static std::vector<const char*> fn_vector;
static DirEnts existing_archives;

static LibError vfs_opt_init(const char* trace_filename, const char* archive_fn_fmt, bool force_build)
{
	// get list of files in root dir.
	// note: this is needed by should_rebuild_main_archive and later in
	//   vfs_opt_continue; must be done here instead of inside the former
	//   because that is not called when force_build == true.
	char dir[PATH_MAX];
	path_dir_only(archive_fn_fmt, dir);
	DirEnts existing_archives;	// and possibly other entries
	RETURN_ERR(file_get_sorted_dirents(dir, existing_archives));

	// bail if we shouldn't rebuild the archive.
	if(!force_build && !should_rebuild_main_archive(trace_filename, archive_fn_fmt, existing_archives))
		return INFO_SKIPPED;

	// build 'graph' (nodes only) of all files that must be added.
	FileNodes file_nodes;
	FileGatherer gatherer(file_nodes);
	if(file_nodes.empty())
		WARN_RETURN(ERR_DIR_END);

	// scan nodes and add them to filename->FileId mapping.
	id_mgr.init(&file_nodes);

	// build list of edges between FileNodes (referenced via FileId) that
	// are defined by trace entries.
	Connections connections;
	ConnectionBuilder cbuilder;
	RETURN_ERR(cbuilder.run(trace_filename, connections));

	// create output filelist by first adding the above edges (most
	// frequent first) and then adding the rest sequentially.
	TourBuilder builder(file_nodes, connections, fn_vector);
	fn_vector.push_back(0);	// 0-terminate for Filenames
	Filenames V_fns = &fn_vector[0];

	// get next not-yet-existing archive filename.
	char archive_fn[PATH_MAX];
	static NextNumberedFilenameInfo archive_nfi;
	bool use_vfs = false;	// can't use VFS for archive files
	next_numbered_filename(archive_fn_fmt, &archive_nfi, archive_fn, use_vfs);

	RETURN_ERR(archive_build_init(archive_fn, V_fns, &ab));
	return ERR_OK;
}


static int vfs_opt_continue()
{
	int ret = archive_build_continue(&ab);
	if(ret == ERR_OK)
	{
		// do NOT delete source files! some apps might want to
		// keep them (e.g. for source control), or name them differently.

		// delete old archives

		// rebuild is required to make sure the new archive is used. this is
		// already taken care of by VFS dir watch, unless it's disabled..
		(void)mount_rebuild();
	}
	return ret;
}





static bool should_build_mini_archive(const char* UNUSED(mini_archive_fn_fmt))
{
#if AB_COUNT_LOOSE_FILES
	// too many (eligible for archiving!) loose files not in archive
	const ssize_t loose_files_only = (ssize_t)loose_files.size() - (ssize_t)archived_files.size();
	if(loose_files_only >= BUILD_MINI_ARCHIVE_THRESHOLD)
		return true;
#endif
	return false;
}

static LibError build_mini_archive(const char* mini_archive_fn_fmt)
{
	if(!should_build_mini_archive(mini_archive_fn_fmt))
		return INFO_SKIPPED;

	Filenames V_fns = (Filenames)malloc((loose_files.size()+1) * sizeof(const char*));
	if(!V_fns)
		WARN_RETURN(ERR_NO_MEM);
	std::copy(loose_files.begin(), loose_files.end(), &V_fns[0]);
	V_fns[loose_files.size()] = 0;	// terminator

	// get new unused mini archive name at P_dst_path
	char mini_archive_fn[VFS_MAX_PATH];
	static NextNumberedFilenameInfo nfi;
	bool use_vfs = false;	// can't use VFS for archive files
	next_numbered_filename(mini_archive_fn_fmt, &nfi, mini_archive_fn, use_vfs);

	RETURN_ERR(archive_build(mini_archive_fn, V_fns));
	return ERR_OK;
}



static enum
{
	DECIDE_IF_BUILD,
	IN_PROGRESS,
	NOP
}
state = DECIDE_IF_BUILD;

void vfs_opt_auto_build_cancel()
{
	archive_build_cancel(&ab);
	state = NOP;
}

int vfs_opt_auto_build(const char* trace_filename,
	const char* archive_fn_fmt, const char* mini_archive_fn_fmt, bool force_build)
{
	if(state == NOP)
		return INFO_ALL_COMPLETE;

	if(state == DECIDE_IF_BUILD)
	{
		if(vfs_opt_init(trace_filename, archive_fn_fmt, force_build) != INFO_SKIPPED)
			state = IN_PROGRESS;
		else
		{
			// create mini-archive (if needed)
			RETURN_ERR(build_mini_archive(mini_archive_fn_fmt));

			state = NOP;
			return ERR_OK;	// "finished"
		}
	}

	if(state == IN_PROGRESS)
	{
		int ret = vfs_opt_continue();
		// just finished
		if(ret == ERR_OK)
			state = NOP;
		return ret;
	}

	UNREACHABLE;
	return ERR_OK; // To quelch gcc "control reaches end of non-void function"
}

LibError vfs_opt_rebuild_main_archive(const char* trace_filename, const char* archive_fn_fmt)
{
	for(;;)
	{
		int ret = vfs_opt_auto_build(trace_filename, archive_fn_fmt, 0, true);
		RETURN_ERR(ret);
		if(ret == ERR_OK)
			return ERR_OK;
	}
}
