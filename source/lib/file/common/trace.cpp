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

/*
 * IO event recording
 */

#include "precompiled.h"
#include "trace.h"

#include <cstdio>

#include "lib/allocators/pool.h"
#include "lib/bits.h"	// round_up
#include "lib/timer.h"	// timer_Time
#include "lib/path_util.h"


/*virtual*/ ITrace::~ITrace()
{

}


//-----------------------------------------------------------------------------

TraceEntry::TraceEntry(EAction action, const fs::wpath& pathname, size_t size)
: m_timestamp((float)timer_Time())
, m_action(action)
, m_pathname(pathname)
, m_size(size)
{
}


TraceEntry::TraceEntry(const std::wstring& text)
{
	wchar_t pathname[PATH_MAX] = L"";
	wchar_t action;
#if EMULATE_SECURE_CRT
	#define TRACE_FORMAT L"%f: %c \"%" STRINGIZE(PATH_MAX) "[^\"]\" %zd\n" /* use a macro to allow compile-time type-checking */
	const int fieldsRead = swscanf(text.c_str(), TRACE_FORMAT, &m_timestamp, &action, pathname, &m_size);
#else
	#define TRACE_FORMAT L"%f: %c \"%[^\"]\" %d\n"
	const int fieldsRead = swscanf_s(text.c_str(), TRACE_FORMAT, &m_timestamp, &action, 1, pathname, PATH_MAX, &m_size);
#endif
	debug_assert(fieldsRead == 4);
	debug_assert(action == 'L' || action == 'S');
	m_action = (EAction)action;
	m_pathname = pathname;
}


std::wstring TraceEntry::EncodeAsText() const
{
	const wchar_t action = (wchar_t)m_action;
	wchar_t buf[1000];
	swprintf_s(buf, ARRAY_SIZE(buf), L"%#010f: %c \"%ls\" %lu\n", m_timestamp, action, m_pathname.string().c_str(), (unsigned long)m_size);
	return buf;
}


//-----------------------------------------------------------------------------

class Trace_Dummy : public ITrace
{
public:
	Trace_Dummy(size_t UNUSED(maxSize))
	{

	}

	virtual void NotifyLoad(const fs::wpath& UNUSED(pathname), size_t UNUSED(size))
	{
	}

	virtual void NotifyStore(const fs::wpath& UNUSED(pathname), size_t UNUSED(size))
	{
	}

	virtual LibError Load(const fs::wpath& UNUSED(pathname))
	{
		return INFO::OK;
	}

	virtual LibError Store(const fs::wpath& UNUSED(pathname)) const
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
		for(size_t i = 0; i < NumEntries(); i++)
		{
			TraceEntry* entry = (TraceEntry*)(uintptr_t(m_pool.da.base) + i*m_pool.el_size);
			entry->~TraceEntry();
		}

		(void)pool_destroy(&m_pool);
	}

	virtual void NotifyLoad(const fs::wpath& pathname, size_t size)
	{
		new(Allocate()) TraceEntry(TraceEntry::Load, pathname, size);
	}

	virtual void NotifyStore(const fs::wpath& pathname, size_t size)
	{
		new(Allocate()) TraceEntry(TraceEntry::Store, pathname, size);
	}

	virtual LibError Load(const fs::wpath& pathname)
	{
		pool_free_all(&m_pool);

		errno = 0;
		FILE* file;
		errno_t err = _wfopen_s(&file, pathname.string().c_str(), L"rt");
		if(err != 0)
			return LibError_from_errno();

		for(;;)
		{
			wchar_t text[500];
			if(!fgetws(text, ARRAY_SIZE(text)-1, file))
				break;
			new(Allocate()) TraceEntry(text);
		}
		fclose(file);

		return INFO::OK;
	}

	virtual LibError Store(const fs::wpath& pathname) const
	{
		errno = 0;
		FILE* file;
		errno_t err = _wfopen_s(&file, pathname.string().c_str(), L"at");
		if(err != 0)
			return LibError_from_errno();
		for(size_t i = 0; i < NumEntries(); i++)
		{
			std::wstring text = Entries()[i].EncodeAsText();
			fputws(text.c_str(), file);
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
		return m_pool.da.pos / m_pool.el_size;
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
