/**
 * =========================================================================
 * File        : trace.cpp
 * Project     : 0 A.D.
 * Description : allows recording and 'playing back' a sequence of
 *             : I/Os - useful for benchmarking and archive builder.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "trace.h"

#include "lib/allocators.h"
#include "lib/timer.h"
#include "lib/sysdep/cpu.h"

// one run per file


/*virtual*/ ITrace::~ITrace()
{
}

class Trace_Dummy : public ITrace
{
public:
	Trace_Dummy(size_t UNUSED(maxSize))
	{

	}

	virtual void NotifyLoad(const char* UNUSED(pathname), size_t UNUSED(size))
	{
	}

	virtual void NotifyStore(const char* UNUSED(pathname), size_t UNUSED(size))
	{
	}

	virtual void NotifyFree(const char* UNUSED(pathname))
	{
	}

	virtual LibError Load(const char* UNUSED(vfsPathname))
	{
		return INFO::OK;
	}

	virtual LibError Store(const char* UNUSED(vfsPathname)) const
	{
		return INFO::OK;
	}

	virtual const TraceEntry* Entries() const
	{
		return 0;
	}

	virtual size_t NumEntries() const
	{
		return 0;
	}
};


class Trace : public ITrace
{
public:
	Trace(size_t maxSize)
	{
		(void)pool_create(&m_pool, maxSize, sizeof(TraceEntry));
	}

	virtual ~Trace()
	{
		(void)pool_destroy(&m_pool);
	}

	virtual void NotifyLoad(const char* pathname, size_t size)
	{
		AddEntry(TO_LOAD, pathname, size);
	}

	virtual void NotifyStore(const char* pathname, size_t size)
	{
		AddEntry(TO_STORE, pathname, size);
	}

	virtual void NotifyFree(const char* pathname)
	{
		const size_t size = 0;	// meaningless for TO_FREE
		AddEntry(TO_FREE, pathname, size);
	}

	virtual LibError Load(const char* osPathname)
	{
		pool_free_all(&m_pool);

		FILE* f = fopen(osPathname, "rt");
		if(!f)
			WARN_RETURN(ERR::FILE_ACCESS);
		for(size_t i = 0; ; i++)
		{
			TraceEntry* t = (TraceEntry*)pool_alloc(&m_pool, 0);
			t->Read(f);
		}

		fclose(f);

		return INFO::OK;
	}

	virtual LibError Store(const char* osPathname) const
	{
		FILE* f = fopen(osPathname, "at");
		if(!f)
			WARN_RETURN(ERR::FILE_ACCESS);
		for(size_t i = 0; i < NumEntries(); i++)
			Entries()[i]->Write(f);
		(void)fclose(f);
		return INFO::OK;
	}

	virtual const TraceEntry* Entries() const
	{
		return (const TraceEntry*)m_pool.da.base;
	}

	virtual size_t NumEntries() const
	{
		return m_pool.da.pos / sizeof(TraceEntry);
	}

private:
	void AddEntry(TraceOp op, const char* pathname, size_t size, double timestamp = 0.0)
	{
		TraceEntry* t = (TraceEntry*)pool_alloc(&m_pool, 0);
		debug_assert(t);
		t->timestamp = timestamp == 0.0? get_time() : timestamp;
		t->vfsPathname = path_Pool()->UniqueCopy(pathname);
		t->size = size;
		t->op = op;
	}

	Pool m_pool;
};





// parse lines and stuff them in m_pool
// (as if they had been AddEntry-ed; replaces any existing data)
// .. bake PATH_MAX limit into string.
char fmt[30];
snprintf(fmt, ARRAY_SIZE(fmt), "%%lf: %%c \"%%%d[^\"]\" %%d %%04x\n", PATH_MAX);

double timestamp; char opcode; char P_path[PATH_MAX]; size_t size;
int fieldsRead = fscanf(f, fmt, &timestamp, &opcode, P_path, &size);
if(fieldsRead == EOF)
break;
debug_assert(fieldsRead == 4);



static char CharFromTraceOp(TraceOp op)
{
	switch(op)
	{
	case TO_LOAD:
		return 'L';
	case TO_STORE:
		return 'S';
	case TO_FREE:
		return 'F';
	default:
		DEBUG_WARN_ERR(ERR::CORRUPTED);
	}
}

static TraceOp TraceOpFromChar(char c)
{
	switch(c)
	{
	case 'L':
		return TO_LOAD;
	case 'S':
		return TO_STORE;
	case 'F':
		return TO_FREE;
	default:
		DEBUG_WARN_ERR(ERR::CORRUPTED);
	}
}

static void write_entry(FILE* f, const TraceEntry* ent)
{
	fprintf(f, "%#010f: %c \"%s\" %d\n", ent->timestamp, CharFromTraceOp(ent->op), ent->vfsPathname, ent->size);
}

{
	// carry out this entry's operation
	switch(ent->op)
	{
		// do not 'run' writes - we'd destroy the existing data.
	case TO_STORE:
		break;
	case TO_LOAD:
		{
			IoBuf buf; size_t size;
			(void)vfs_load(ent->vfsPathname, buf, size, ent->flags);
			break;
		}
	case TO_FREE:
		fileCache.Release(ent->vfsPathname);
		break;
	default:
		debug_warn("unknown TraceOp");
	}
}





LibError trace_run(const char* osPathname)
{
	Trace trace;
	RETURN_ERR(trace.Load(osPathname));
	for(uint i = 0; i < trace.NumEntries(); i++)
		trace.Entries()[i]->Run();
	return INFO::OK;
}
