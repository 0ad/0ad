/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * IO event recording
 */

// traces are useful for determining the optimal ordering of archived files
// and can also serve as a repeatable IO benchmark.

// note: since FileContents are smart pointers, the trace can't easily
// be notified when they are released (relevant for cache simulation).
// we have to assume that users process one file at a time -- as they
// should.

#ifndef INCLUDED_TRACE
#define INCLUDED_TRACE

#include "lib/os_path.h"

// stores information about an IO event.
class TraceEntry
{
public:
	enum EAction
	{
		Load = 'L',
		Store = 'S'
	};

	TraceEntry(EAction action, const Path& pathname, size_t size);
	TraceEntry(const std::wstring& text);

	EAction Action() const
	{
		return m_action;
	}

	const Path& Pathname() const
	{
		return m_pathname;
	}

	size_t Size() const
	{
		return m_size;
	}

	std::wstring EncodeAsText() const;

private:
	// note: keep an eye on the class size because all instances are kept
	// in memory (see ITrace)

	// time (as returned by timer_Time) after the operation completes.
	// rationale: when loading, the VFS doesn't know file size until
	// querying the cache or retrieving file information.
	float m_timestamp;

	EAction m_action;

	Path m_pathname;

	// size of file.
	// rationale: other applications using this trace format might not
	// have access to the VFS and its file information.
	size_t m_size;
};


// note: to avoid interfering with measurements, this trace container
// does not cause any IOs (except of course in Load/Store)
struct ITrace
{
	virtual ~ITrace();

	virtual void NotifyLoad(const Path& pathname, size_t size) = 0;
	virtual void NotifyStore(const Path& pathname, size_t size) = 0;

	/**
	 * store all entries into a file.
	 *
	 * @param pathname (native, absolute)
	 *
	 * note: the file format is text-based to allow human inspection and
	 * because storing filename strings in a binary format would be a
	 * bit awkward.
	 **/
	virtual Status Store(const OsPath& pathname) const = 0;

	/**
	 * load entries from file.
	 *
	 * @param pathname (native, absolute)
	 *
	 * replaces any existing entries.
	 **/
	virtual Status Load(const OsPath& pathname) = 0;

	virtual const TraceEntry* Entries() const = 0;
	virtual size_t NumEntries() const = 0;
};

typedef shared_ptr<ITrace> PITrace;

extern PITrace CreateDummyTrace(size_t maxSize);
extern PITrace CreateTrace(size_t maxSize);

#endif	// #ifndef INCLUDED_TRACE
