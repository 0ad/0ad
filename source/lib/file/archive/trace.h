/**
 * =========================================================================
 * File        : trace.h
 * Project     : 0 A.D.
 * Description : allows recording and 'playing back' a sequence of
 *             : I/Os - useful for benchmarking and archive builder.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_TRACE
#define INCLUDED_TRACE

// TraceEntry operation type.
// note: rather than only a list of accessed files, we also need to
// know the application's behavior WRT caching (e.g. when it releases
// cached buffers). this is necessary so that our simulation can
// yield the same behavior.
enum TraceOp
{
	TO_LOAD,
	TO_STORE,
	TO_FREE,
};

// stores one event that is relevant for file IO / caching.
//
// size-optimized a bit since these are all kept in memory
// (to prevent trace file writes from affecting other IOs)
struct TraceEntry
{
	// note: float instead of double for nice 16 byte struct size
	float timestamp;		// returned by get_time before operation starts
	const char* vfsPathname;
	// rationale: store size in the trace because other applications
	// that use this trace format but not our IO code wouldn't know
	// size (since they cannot retrieve the file info given vfsPathname).
	size_t size;			// of IO (usually the entire file)
	TraceOp op;
};

struct ITrace
{
	virtual ~ITrace();

	virtual void NotifyLoad(const char* vfsPathname, size_t size);
	virtual void NotifyStore(const char* vfsPathname, size_t size);
	virtual void NotifyFree(const char* vfsPathname);

	// note: file format is text-based to allow human inspection and because
	// storing filenames would be a bit awkward
	virtual LibError Load(const char* osPathname);
	virtual LibError Store(const char* osPathname) const;

	virtual const TraceEntry* Entries() const;
	virtual size_t NumEntries() const;
};


// enabled by default. by the time we can decide whether a trace needs to
// be generated (see should_rebuild_main_archive), file accesses will
// already have occurred; hence default enabled and disable if not needed.


#endif	// #ifndef INCLUDED_TRACE
