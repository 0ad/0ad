#ifndef VFS_OPTIMIZER_H__
#define VFS_OPTIMIZER_H__

extern void trace_enable(bool want_enabled);
extern void trace_add(const char* P_fn);

extern LibError trace_write_to_file(const char* trace_filename);
extern LibError trace_read_from_file(const char* trace_filename);

struct TraceEntry
{
	double timestamp;
	const char* atom_fn;
};

struct Trace
{
	const TraceEntry* ents;
	uint num_ents;
};

extern void trace_get(Trace* t);
extern void trace_shutdown();

#endif	// #ifndef VFS_OPTIMIZER_H__
