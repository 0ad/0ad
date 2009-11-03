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

// traces are useful for determining the optimal ordering of archived files
// and can also serve as a repeatable IO benchmark.

// note: since FileContents are smart pointers, the trace can't easily
// be notified when they are released (relevant for cache simulation).
// we have to assume that users process one file at a time -- as they
// should.

#ifndef INCLUDED_TRACE
#define INCLUDED_TRACE

// stores information about an IO event.
class TraceEntry
{
public:
	enum EAction
	{
		Load = 'L',
		Store = 'S'
	};

	TraceEntry(EAction action, const fs::wpath& pathname, size_t size);
	TraceEntry(const std::wstring& text);

	EAction Action() const
	{
		return m_action;
	}

	const fs::wpath& Pathname() const
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

	fs::wpath m_pathname;

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

	virtual void NotifyLoad(const fs::wpath& pathname, size_t size) = 0;
	virtual void NotifyStore(const fs::wpath& pathname, size_t size) = 0;

	/**
	 * store all entries into a file.
	 *
	 * @param pathname (native, absolute)
	 *
	 * note: the file format is text-based to allow human inspection and
	 * because storing filename strings in a binary format would be a
	 * bit awkward.
	 **/
	virtual LibError Store(const fs::wpath& pathname) const = 0;

	/**
	 * load entries from file.
	 *
	 * @param pathname (native, absolute)
	 *
	 * replaces any existing entries.
	 **/
	virtual LibError Load(const fs::wpath& osPathname) = 0;

	virtual const TraceEntry* Entries() const = 0;
	virtual size_t NumEntries() const = 0;
};

typedef shared_ptr<ITrace> PITrace;

extern PITrace CreateDummyTrace(size_t maxSize);
extern PITrace CreateTrace(size_t maxSize);

#endif	// #ifndef INCLUDED_TRACE
