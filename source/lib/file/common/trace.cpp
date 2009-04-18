/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : trace.cpp
 * Project     : 0 A.D.
 * Description : IO event recording
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "trace.h"

#include "lib/allocators/pool.h"
#include "lib/timer.h"	// timer_Time

/*virtual*/ ITrace::~ITrace()
{

}


//-----------------------------------------------------------------------------

TraceEntry::TraceEntry(EAction action, const char* pathname, size_t size)
: m_timestamp((float)timer_Time())
, m_action(action)
, m_pathname(strdup(pathname))
, m_size(size)
{
}


TraceEntry::TraceEntry(const char* text)
{
	const char* fmt = "%f: %c \"" STRINGIZE(PATH_MAX) "[^\"]\" %d\n";
	char pathname[PATH_MAX];
	char action;
	const int fieldsRead = sscanf_s(text, fmt, &m_timestamp, &action, pathname, &m_size);
	debug_assert(fieldsRead == 4);
	debug_assert(action == 'L' || action == 'S');
	m_action = (EAction)action;
	m_pathname = strdup(pathname);
}


TraceEntry::~TraceEntry()
{
	SAFE_FREE(m_pathname);
}


void TraceEntry::EncodeAsText(char* text, size_t maxTextChars) const
{
	const char action = (char)m_action;
	sprintf_s(text, maxTextChars, "%#010f: %c \"%s\" %d\n", m_timestamp, action, m_pathname, m_size);
}


//-----------------------------------------------------------------------------

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

	virtual LibError Load(const char* UNUSED(pathname))
	{
		return INFO::OK;
	}

	virtual LibError Store(const char* UNUSED(pathname)) const
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


//-----------------------------------------------------------------------------

class Trace : public ITrace
{
public:
	Trace(size_t maxSize)
	{
		(void)pool_create(&m_pool, maxSize, sizeof(TraceEntry));
	}

	virtual ~Trace()
	{
		TraceEntry* entries = (TraceEntry*)m_pool.da.base;
		for(TraceEntry* entry = entries; entry < entries+NumEntries(); entry++)
			entry->~TraceEntry();
		(void)pool_destroy(&m_pool);
	}

	virtual void NotifyLoad(const char* pathname, size_t size)
	{
		new(Allocate()) TraceEntry(TraceEntry::Load, pathname, size);
	}

	virtual void NotifyStore(const char* pathname, size_t size)
	{
		new(Allocate()) TraceEntry(TraceEntry::Store, pathname, size);
	}

	virtual LibError Load(const char* osPathname)
	{
		pool_free_all(&m_pool);

		errno = 0;
		FILE* file = fopen(osPathname, "rt");
		if(!file)
			return LibError_from_errno();
		for(;;)
		{
			char text[500];
			if(!fgets(text, ARRAY_SIZE(text)-1, file))
				break;
			new(Allocate()) TraceEntry(text);
		}
		fclose(file);

		return INFO::OK;
	}

	virtual LibError Store(const char* osPathname) const
	{
		errno = 0;
		FILE* file = fopen(osPathname, "at");
		if(!file)
			return LibError_from_errno();
		for(size_t i = 0; i < NumEntries(); i++)
		{
			char text[500];
			Entries()[i].EncodeAsText(text, ARRAY_SIZE(text));
			fputs(text, file);
		}
		(void)fclose(file);
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
	void* Allocate()
	{
		void* p = pool_alloc(&m_pool, 0);
		debug_assert(p);
		return p;
	}

	Pool m_pool;
};


PITrace CreateDummyTrace(size_t maxSize)
{
	return PITrace(new Trace_Dummy(maxSize));
}

PITrace CreateTrace(size_t maxSize)
{
	return PITrace(new Trace(maxSize));
}
