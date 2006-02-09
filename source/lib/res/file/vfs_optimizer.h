#ifndef VFS_OPTIMIZER_H__
#define VFS_OPTIMIZER_H__

extern void trace_enable(bool want_enabled);
extern void trace_shutdown();

extern void trace_notify_load(const char* P_fn, size_t size, uint flags);
extern void trace_notify_free(const char* P_fn, size_t size);

// TraceEntry operation type.
// note: rather than only a list of accessed files, we also need to
// know the application's behavior WRT caching (e.g. when it releases
// cached buffers). this is necessary so that our simulation can
// yield the same results.
enum TraceOp
{
	TO_LOAD,
	TO_FREE
};

// stores one event that is relevant for file IO / caching.
//
// size-optimized a bit since these are all kept in memory
// (to prevent trace file writes from affecting other IOs)
struct TraceEntry
{
	// note: float instead of double for nice 16 byte struct size
	float timestamp;		// returned by get_time before operation starts
	const char* atom_fn;	// path+name of affected file
	// rationale: store size in the trace because other applications
	// that use this trace format but not our IO code wouldn't know
	// size (since they cannot retrieve the file info given atom_fn).
	size_t size;			// of IO (usually the entire file)
	uint op : 8;			// operation - see TraceOp
	uint flags : 24;		// misc, e.g. file_io flags.
};

struct Trace
{
	const TraceEntry* ents;
	size_t num_ents;
};

extern void trace_get(Trace* t);
extern LibError trace_write_to_file(const char* trace_filename);
extern LibError trace_read_from_file(const char* trace_filename, Trace* t);

extern LibError vfs_opt_rebuild_main_archive(const char* P_dst_path);

extern void vfs_opt_notify_loose_file(const char* atom_fn);

#endif	// #ifndef VFS_OPTIMIZER_H__
