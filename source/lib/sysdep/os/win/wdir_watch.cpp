/**
 * =========================================================================
 * File        : wdir_watch.cpp
 * Project     : 0 A.D.
 * Description : Win32 directory change notification
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "lib/sysdep/dir_watch.h"

#include <string>
#include <map>
#include <list>

#include "lib/path_util.h"
#include "lib/allocators/allocators.h"	// HACK, get rid of STATIC_STORAGE
#include "lib/path_util.h"	// path_is_subpath
#include "win.h"
#include "winit.h"
#include "wutil.h"

WINIT_REGISTER_MAIN_INIT(wdir_watch_Init);
WINIT_REGISTER_MAIN_SHUTDOWN(wdir_watch_Shutdown);

// rationale for polling:
// much simpler than pure asynchronous notification: no need for a
// worker thread, mutex, and in/out queues. polling isn't inefficient:
// we do not examine each file; we only need to check if Windows
// has sent a change notification via ReadDirectoryChangesW.
//
// the main reason, however, is that user code will want to poll anyway,
// instead of select() from a worker thread: handling asynchronous file
// changes is much more work, requiring everything to be thread-safe.
// we currently poll once a frame, so that file changes will happen
// at a defined time.


// rationale for using I/O completion ports for notification:
// alternatives:
// - multiple threads with blocking I/O. a good many mount points
//   and therefore directory watches are possible, so this is out.
// - normal overlapped I/O: build a contiguous array of the hEvents
//   in all OVERLAPPED structures, and WaitForMultipleObjects.
//   having to (re)build the array after every watch add/remove sucks.
// - callback notification: notification function is called when the thread
//   that initiated the I/O (ReadDirectoryChangesW) enters an alertable
//   wait state (e.g. with SleepEx). we need to poll for notifications
//   from the mainline (see above). unfortunately, cannot come up with
//   a robust yet quick way of working off all pending APCs -
//   SleepEx(1) is a hack. even worse, it was noted in a previous project
//   that APCs are sometimes delivered from within Windows APIs, without
//   having used SleepEx (it seems threads enter an "AWS" sometimes when
//   calling the kernel).
//
// IOCPs work well and are elegant; have not yet noticed any drawbacks.
// the completion key is used to associate Watch with the directory handle.


// don't worry about size; heap-allocated.
struct Watch
{
	intptr_t reqnum;

	// (refcounted, since dir_add_watch reuses existing Watches)
	int refs;

	std::string dir_name;
	HANDLE hDir;

	// storage for RDC lpBytesReturned to avoid BoundsChecker warning
	// (dox are unclear on whether the pointer must be valid).
	DWORD dummy_nbytes;

	// fields aren't used.
	// overlapped I/O completation notification is via IOCP.
	OVERLAPPED ovl;

	// if too small, the current FILE_NOTIFY_INFORMATION is lost!
	// this is enough for ~7 packets (worst case) - should be enough,
	// since the app polls once a frame. we don't want to waste too much
	// memory. size chosen such that sizeof(Watch) = 4KiB.
	// issue code uses sizeof(change_buf) to determine size.
	//
	// note: we can't share one central buffer: the individual watches
	// are independent, and may be triggered 'simultaneously' before
	// the next app poll, so they'd overwrite one another.
	char change_buf[4096-58];


	Watch(intptr_t _reqnum, const std::string& _dir_name, HANDLE _hDir)
		: reqnum(_reqnum), refs(1), dir_name(_dir_name), hDir(_hDir)
	{
		memset(&ovl, 0, sizeof(ovl));
		// change_buf[] doesn't need init
	}

	~Watch()
	{
		CloseHandle(hDir);
		hDir = INVALID_HANDLE_VALUE;
	}
};


//-----------------------------------------------------------------------------
// list of active watches

// we need to be able to cancel watches, which requires a 'list' of them.
// this also makes detecting duplicates possible and simplifies cleanup.
//
// key is intptr_t "reqnum"; they aren't reused to avoid problems with
// stale reqnums after canceling; hence, use map instead of array.
//
// only store pointer in container - they're not copy-equivalent
// (dtor would close hDir).
typedef std::map<intptr_t, Watch*> Watches;
typedef Watches::iterator WatchIt;
static Watches* watches;

static void FreeAllWatches()
{
	for(WatchIt it = watches->begin(); it != watches->end(); ++it)
	{
		Watch* w = it->second;
		delete w;
	}
	watches->clear();
}

static Watch* WatchFromReqnum(intptr_t reqnum)
{
	return (*watches)[reqnum];
}


//-----------------------------------------------------------------------------
// event queue

// rationale:
// we need a queue, instead of just taking events from the change_buf,
// because we need to re-issue the watch immediately after it returns
// data. of course we can't have the app read from the buffer while
// waiting for RDC to write to the buffer - race condition.
// an alternative to a queue would be to allocate another buffer,
// but that's more complicated, and this way is cleaner anyway.
typedef std::list<std::string> Events;
static Events* pending_events;


//-----------------------------------------------------------------------------
// allocate static objects

// manual allocation and construction/destruction of static objects is
// required because winit calls us before static ctors and after dtors.

static void AllocStaticObjects()
{
	STATIC_STORAGE(ss, 200);
	void* addr1 = static_calloc(&ss, sizeof(Watches));
	watches = new(addr1) Watches;

	void* addr2 = static_calloc(&ss, sizeof(Events));
	pending_events = new(addr2) Events;
}

static void FreeStaticObjects()
{
	watches->~Watches();
	pending_events->~Events();
}


//-----------------------------------------------------------------------------
// global state

static HANDLE hIOCP = 0;	// CreateIoCompletionPort requires 0, not INVALID_HANDLE_VALUE

// note: the start value provides a little protection against bogus reqnums
// (we don't bother using a tag for safety though - it isn't important)
static intptr_t last_reqnum = 1000;


// HACK - see call site
static void get_packet();



// path: portable and relative, must add current directory and convert to native
// better to use a cached string from rel_chdir - secure
LibError dir_add_watch(const char* dir, intptr_t* preqnum)
{
	WinScopedPreserveLastError s;	// Create*

	intptr_t reqnum;
	*preqnum = 0;

	{
	const std::string dir_s(dir);

	// check if this is a subdirectory of an already watched dir tree
	// (much faster than issuing a new watch for every subdir).
	// this also prevents watching the same directory twice.
	for(WatchIt it = watches->begin(); it != watches->end(); ++it)
	{
		Watch* const w = it->second;
		if(!w)
			continue;
		const char* old_dir = w->dir_name.c_str();
		if(path_is_subpath(dir, old_dir))
		{
			*preqnum = w->reqnum;
			w->refs++;
			return INFO::OK;
		}
	}

	// open handle to directory
	const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
	const HANDLE hDir = CreateFile(dir, FILE_LIST_DIRECTORY, share, 0, OPEN_EXISTING, flags, 0);
	if(hDir == INVALID_HANDLE_VALUE)
		WARN_RETURN(ERR::FAIL);

	// assign a new (unique) request number. don't do this earlier - prevents
	// DOS via wasting reqnums due to invalid directory parameters.
	// need it before binding dir to IOCP because it is our "key".
	if(last_reqnum == INT_MAX)
	{
		debug_assert(0);	// request numbers are no longer unique
		CloseHandle(hDir);
		WARN_RETURN(ERR::LIMIT);
	}
	reqnum = ++last_reqnum;

	// associate Watch* with the directory handle. when we receive a packet
	// from the IOCP, we will need to re-issue the watch.
	const ULONG_PTR key = (ULONG_PTR)reqnum;

	// create IOCP (if not already done) and attach hDir to it
	hIOCP = CreateIoCompletionPort(hDir, hIOCP, key, 0);
	if(hIOCP == 0 || hIOCP == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDir);
		WARN_RETURN(ERR::FAIL);
	}

	// allocate watch, add to list, associate with reqnum
	// note: can't use SAFE_NEW due to ctor params.
	Watch* w = new Watch(reqnum, dir_s, hDir);
	(*watches)[reqnum] = w;

	// add trailing \ if not already there
	if(dir_s[dir_s.length()-1] != '\\')
		w->dir_name += '\\';


	// post a dummy kickoff packet; the IOCP polling code will "re"issue
	// the corresponding watch. this keeps the ReadDirectoryChangesW call
	// and directory <--> Watch association code in one place.
	//
	// we call get_packet so that it's issued immediately,
	// instead of only at the next call to dir_get_changed_file.
	PostQueuedCompletionStatus(hIOCP, 0, key, 0);
	get_packet();
	}

	*preqnum = reqnum;
	return INFO::OK;
}


LibError dir_cancel_watch(const intptr_t reqnum)
{
	if(reqnum <= 0)
		WARN_RETURN(ERR::INVALID_PARAM);

	Watch* w = WatchFromReqnum(reqnum);
	// watches[reqnum] is invalid - big trouble
	if(!w)
		WARN_RETURN(ERR::FAIL);

	// we're freeing a reference - done.
	debug_assert(w->refs >= 1);
	if(--w->refs != 0)
		return INFO::OK;

	// contrary to dox, the RDC IOs do not issue a completion notification.
	// no packet was received on the IOCP while or after cancelling in a test.
	//
	// if cancel somehow fails though, no matter - the Watch is freed, and
	// its reqnum isn't reused; if we receive a packet, it's ignored.
	BOOL ok = CancelIo(w->hDir);

	(*watches)[reqnum] = 0;
	delete w;
	return LibError_from_win32(ok);
}


static void extract_events(Watch* w)
{
	debug_assert(w);

	// points to current FILE_NOTIFY_INFORMATION;
	// char* simplifies advancing to the next (variable length) FNI.
	char* pos = w->change_buf;

	// for every packet in buffer: (there's at least one)
	for(;;)
	{
		const FILE_NOTIFY_INFORMATION* fni = (const FILE_NOTIFY_INFORMATION*)pos;

		// convert filename from Windows BSTR
		// (can't use wcstombs - FileName isn't 0-terminated)
		std::string fn = w->dir_name;
		for(int i = 0; i < (int)fni->FileNameLength/2; i++)
			fn += (char)fni->FileName[i];

		pending_events->push_back(fn);

		// advance to next entry in buffer (variable length)
		const DWORD ofs = fni->NextEntryOffset;
		// .. this one was the last - done.
		if(!ofs)
			break;
		pos += ofs;
	}
}


// if a packet is pending, extract its events, post them in the queue and
// re-issue its watch.
static void get_packet()
{
	// poll for change notifications from all pending watches
	DWORD bytes_transferred;	// used to determine if packet is valid or a kickoff
	ULONG_PTR key;
	OVERLAPPED* povl;
	BOOL got_packet = GetQueuedCompletionStatus(hIOCP, &bytes_transferred, &key, &povl, 0);
	if(!got_packet)	// no new packet - done
		return;

	const intptr_t reqnum = (intptr_t)key;
	Watch* const w = WatchFromReqnum(reqnum);
	// watch was subsequently removed - ignore the error.
	if(!w)
		return;

	// this is an actual packet, not just a kickoff for issuing the watch.
	// extract the events and push them onto AppState's queue.
	if(bytes_transferred != 0)
		extract_events(w);

	// (re-)issue change notification request.
	// it's safe to reuse Watch.change_buf, because we copied out all events.
	const DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
	                     FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE |
						 FILE_NOTIFY_CHANGE_CREATION;
	const DWORD buf_size = sizeof(w->change_buf);
	memset(&w->ovl, 0, sizeof(w->ovl));
	BOOL watch_subtree = TRUE;
		// much faster than watching every dir separately. see dir_add_watch.
	BOOL ok = ReadDirectoryChangesW(w->hDir, w->change_buf, buf_size, watch_subtree, filter, &w->dummy_nbytes, &w->ovl, 0);
	WARN_IF_FALSE(ok);
}


// if a file change notification is pending, store its filename in <fn> and
// return INFO::OK; otherwise, return ERR::AGAIN ('none currently pending') or
// a negative error code.
// <fn> must hold at least PATH_MAX chars.
LibError dir_get_changed_file(char* fn)
{
	// may or may not queue event(s).
	get_packet();

	// nothing to return; call again later.
	if(pending_events->empty())
		return ERR::AGAIN;	// NOWARN

	const std::string& fn_s = pending_events->front();
	strcpy_s(fn, PATH_MAX, fn_s.c_str());
	pending_events->pop_front();

	return INFO::OK;
}


//-----------------------------------------------------------------------------

static LibError wdir_watch_Init()
{
	AllocStaticObjects();
	return INFO::OK;
}

static LibError wdir_watch_Shutdown()
{
	CloseHandle(hIOCP);
	hIOCP = INVALID_HANDLE_VALUE;

	FreeAllWatches();
	FreeStaticObjects();

	return INFO::OK;
}
