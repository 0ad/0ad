/**
 * =========================================================================
 * File        : trace.h
 * Project     : 0 A.D.
 * Description : IO event recording
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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

	TraceEntry(EAction action, const char* pathname, size_t size);
	TraceEntry(const char* textualRepresentation);
	~TraceEntry();

	EAction Action() const
	{
		return m_action;
	}

	const char* Pathname() const
	{
		return m_pathname;
	}

	size_t Size() const
	{
		return m_size;
	}

	void EncodeAsText(char* text, size_t maxTextChars) const;

private:
	// note: keep an eye on the class size because all instances are kept
	// in memory (see ITrace)

	// time (as returned by timer_Time) after the operation completes.
	// rationale: when loading, the VFS doesn't know file size until
	// querying the cache or retrieving file information.
	float m_timestamp;

	EAction m_action;

	const char* m_pathname;

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

	virtual void NotifyLoad(const char* pathname, size_t size) = 0;
	virtual void NotifyStore(const char* pathname, size_t size) = 0;

	/**
	 * store all entries into a file.
	 *
	 * @param osPathname native (absolute) pathname
	 *
	 * note: the file format is text-based to allow human inspection and
	 * because storing filename strings in a binary format would be a
	 * bit awkward.
	 **/
	virtual LibError Store(const char* osPathname) const = 0;

	/**
	 * load entries from file.
	 *
	 * @param osPathname native (absolute) pathname
	 *
	 * replaces any existing entries.
	 **/
	virtual LibError Load(const char* osPathname) = 0;

	virtual const TraceEntry* Entries() const = 0;
	virtual size_t NumEntries() const = 0;
};

typedef shared_ptr<ITrace> PITrace;

extern PITrace CreateDummyTrace(size_t maxSize);
extern PITrace CreateTrace(size_t maxSize);

#endif	// #ifndef INCLUDED_TRACE
