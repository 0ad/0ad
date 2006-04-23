/**
 * =========================================================================
 * File        : trace.h
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

#ifndef TRACE_H__
#define TRACE_H__

extern void trace_enable(bool want_enabled);
extern void trace_shutdown();

extern void trace_notify_io(const char* P_fn, size_t size, uint flags);
extern void trace_notify_free(const char* P_fn, size_t size);

// TraceEntry operation type.
// note: rather than only a list of accessed files, we also need to
// know the application's behavior WRT caching (e.g. when it releases
// cached buffers). this is necessary so that our simulation can
// yield the same results.
enum TraceOp
{
	TO_IO,
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

extern void trace_gen_random(size_t num_entries);


// simulate carrying out the entry's TraceOp to determine
// whether this IO would be satisfied by the file_buf cache.
extern bool trace_entry_causes_io(const TraceEntry* ent);

enum TraceRunFlags
{
	TRF_SYNC_TO_TIMESTAMP = 1
};

// carry out all operations specified in the trace.
// if flags&TRF_SYNC_TO_TIMESTAMP, waits until timestamp for each event is
// reached; otherwise, they are run as fast as possible.
extern LibError trace_run(const char* trace_filename, uint flags = 0);


#endif	// #ifndef TRACE_H__
