// SGI File Alteration Monitor for Win32
// Copyright (c) 2004 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "wfam.h"
#include "lib.h"
#include "win_internal.h"

#include <assert.h>

#include <string>
#include <map>
#include <list>


// no module init/shutdown necessary: all global data is allocated
// as part of a FAMConnection, which must be FAMClose-d by caller.
//
// that means every routine has a FAMConnection* parameter, but it's safer.


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


// don't worry about size: the user only passes around a pointer
// to this struct, due to the pImpl idiom. this is heap-allocated.
struct Watch
{
	std::string dir_name;
	HANDLE hDir;

	// user pointer from from FAMMonitorDirectory; passed to FAMEvent
	void* user_data;

	// history to detect series of notifications, so we can skip
	// redundant reloads (slow)
	std::string last_path;
	DWORD last_action;	// FILE_ACTION_* codes or 0
	DWORD last_ticks;	// timestamp via GetTickCount

	DWORD dummy_nbytes;
		// storage for RDC lpBytesReturned, to avoid BoundsChecker warning
		// (dox are unclear on whether the pointer must be valid).

	OVERLAPPED ovl;
		// fields aren't used.
		// overlapped I/O completation notification is via IOCP.

	char change_buf[15000];
		// better be big enough - if too small,
		// we miss changes made to a directory.
		// issue code uses sizeof(change_buf) to determine size.


	Watch()
		: last_path("")
	{
		hDir = INVALID_HANDLE_VALUE;

		last_action = 0;
		last_ticks = 0;

		// change_buf[] doesn't need init
	}

	~Watch()
	{
		CloseHandle(hDir);
		hDir = INVALID_HANDLE_VALUE;
	}
};


// list of all active watches. required, since we have to be able to
// cancel watches; also makes detecting duplicates possible.
//
// only store pointer in container - they're not copy-equivalent
// (dtor would close hDir).
//
// key is uint reqnum - that's what FAMCancelMonitor is passed.
// reqnums aren't reused to avoid problems with stale reqnums after
// cancelling; hence, map instead of vector and freelist.
typedef std::map<uint, Watch*> Watches;
typedef Watches::iterator WatchIt;

typedef std::list<FAMEvent> Events;

// don't worry about size: the user only passes around a pointer
// to this struct, due to the pImpl idiom. this is heap-allocated.
struct AppState
{
	std::string app_name;

	HANDLE hIOCP;

	Events pending_events;
		// rationale:
		// we need a queue, instead of just taking events from the change_buf,
		// because we need to re-issue the watch immediately after it returns
		// data. of course we can't have the app read from the buffer while
		// waiting for RDC to write to the buffer - race condition.
		// an alternative to a queue would be to allocate another buffer,
		// but that's more complicated, and this way is cleaner anyway.
		//
		// FAMEvents are somewhat large (~300 bytes), and FIFO,
		// so make it a list.

	// list of all active watches to detect duplicates and
	// for easier cleanup. only store pointer in container -
	// they're not copy-equivalent.
	Watches watches;

	uint last_reqnum;

	AppState()
	{
		hIOCP = 0;
		// not INVALID_HANDLE_VALUE! (CreateIoCompletionPort requirement)

		// provide a little protection against random reqnums passed in
		last_reqnum = 1000;
	}

	~AppState()
	{
		CloseHandle(hIOCP);
		hIOCP = INVALID_HANDLE_VALUE;

		// free all (dynamically allocated) Watch-es
		for(WatchIt it = watches.begin(); it != watches.end(); ++it)
			delete it->second;
	}
};


// macros to return pointers to the above from the FAM* structs (pImpl)
// (macro instead of function so we can bail out of the "calling" function)
//
// WARNING: expands to multiple statements. can't use STMT, because
// the macros defined variables (*_ptr_name) that must be visible.

#define GET_APP_STATE(fc, state_ptr_var)\
	AppState* state_ptr_var = (AppState*)fc->internal;\
	if(!state_ptr_var)\
	{\
		debug_warn("no FAM connection");\
		return -1;\
	}


///////////////////////////////////////////////////////////////////////////////


static int alloc_watch(FAMConnection* const fc, const FAMRequest* const fr, Watch*& _w)
{
	GET_APP_STATE(fc, state);
	Watches& watches = state->watches;

	SAFE_NEW(Watch, w);
	watches[fr->reqnum] = w;
	_w = w;
	return 0;
}


static int find_watch(FAMConnection* const fc, const FAMRequest* const fr, Watch*& w)
{
	GET_APP_STATE(fc, state);
	Watches& watches = state->watches;
	WatchIt it = watches.find(fr->reqnum);
	if(it == watches.end())
		return -1;
	w = it->second;
	return 0;
}

#define GET_WATCH(fc, fr, watch_ptr_var)\
	Watch* watch_ptr_var;\
	CHECK_ERR(find_watch(fc, fr, watch_ptr_var))


static int free_watch(FAMConnection* const fc, const FAMRequest* const fr, Watch*& w)
{
	GET_APP_STATE(fc, state);
	Watches& watches = state->watches;
	WatchIt it = watches.find(fr->reqnum);
	if(it == watches.end())
		return -1;
	delete it->second;
	watches.erase(it);
	w = 0;
	return 0;
}


///////////////////////////////////////////////////////////////////////////////


int FAMOpen2(FAMConnection* const fc, const char* const app_name)
{
	SAFE_NEW(AppState, state);
	state->app_name = app_name;

	fc->internal = state;
	return 0;
}


int FAMClose(FAMConnection* const fc)
{
	GET_APP_STATE(fc, state);

	delete state;
	fc->internal = 0;
	return 0;
}


// HACK - see call site
static int get_packet(FAMConnection* fc);


int FAMMonitorDirectory(FAMConnection* const fc, const char* const _dir, FAMRequest* const fr, void* const user_data)
{
	const std::string dir(_dir);

	GET_APP_STATE(fc, state);
	Watches& watches  = state->watches;
	HANDLE& hIOCP     = state->hIOCP;
	uint& last_reqnum = state->last_reqnum;

	// make sure dir is not already being watched
	for(WatchIt it = watches.begin(); it != watches.end(); ++it)
		if(dir == it->second->dir_name)
			return -1;

	// open handle to directory
	const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
	const HANDLE hDir = CreateFile(_dir, FILE_LIST_DIRECTORY, share, 0, OPEN_EXISTING, flags, 0);
	if(hDir == INVALID_HANDLE_VALUE)
		return -1;

	// assign a new (unique) request number. don't do this earlier - prevents
	// DOS via wasting reqnums due to invalid directory parameters.
	// need it before binding dir to IOCP because it is our "key".
	if(last_reqnum == UINT_MAX)
	{
		debug_warn("FAMMonitorDirectory: request numbers are no longer unique");
		return -1;
	}
	const uint reqnum = ++last_reqnum;
	fr->reqnum = reqnum;

	// associate Watch* with the directory handle. when we receive a packet
	// from the IOCP, we will need to re-issue the watch.
	const ULONG_PTR key = (ULONG_PTR)reqnum;

	// create IOCP (if not already done) and attach hDir to it
	hIOCP = CreateIoCompletionPort(hDir, hIOCP, key, 0);
	if(hIOCP == 0 || hIOCP == INVALID_HANDLE_VALUE)
	{
fail:
		CloseHandle(hDir);
		return -1;
	}

	// create Watch and associate with FAM structs
	Watch* w;
	if(alloc_watch(fc, fr, w) < 0)
		goto fail;
	w->dir_name  = dir;
	w->hDir      = hDir;
	w->user_data = user_data;

	if(dir[dir.length()-1] != '\\')
		w->dir_name += '\\';

	// post a dummy kickoff packet; the IOCP polling code will "re"issue
	// the corresponding watch. this keeps the ReadDirectoryChangesW call
	// and directory <--> Watch association code in one place.
	//
	// we call get_packet so that it's issued immediately,
	// instead of only at the next call to FAMPending.
	PostQueuedCompletionStatus(hIOCP, 0, key, 0);
	get_packet(fc);

	return 0;
}


int FAMCancelMonitor(FAMConnection* const fc, FAMRequest* const fr)
{
	GET_WATCH(fc, fr, w);

	// contrary to dox, the RDC IOs do not issue a completion notification.
	// no packet was received on the IOCP while or after cancelling in a test.
	//
	// if cancel somehow fails though, no matter - the Watch is freed, and
	// its reqnum isn't reused; if we receive a packet, it's ignored.
	CancelIo(w->hDir);

	free_watch(fc, fr, w);	// can't fail
	return 0;
}


static int extract_events(FAMConnection* fc, FAMRequest* fr, Watch* w)
{
	GET_APP_STATE(fc, state);
	Events& events = state->pending_events;

	// will be modified for each event and added to events
	FAMEvent event_template;
	event_template.fc = fc;
	event_template.fr = *fr;
	event_template.user = w->user_data;

	// points to current FILE_NOTIFY_INFORMATION;
	// char* simplifies advancing to the next (variable length) FNI.
	char* pos = w->change_buf;

	// for every packet in buffer: (there's at least one)
	for(;;)
	{
		const FILE_NOTIFY_INFORMATION* const fni = (const FILE_NOTIFY_INFORMATION*)pos;

		events.push_back(event_template);
		FAMEvent& event = events.back();
			// fields are set below; we need to add the event here
			// so that we have a place to put the converted filename.


		//
		// interpret action
		//

		const char* actions[] = { "", "FILE_ACTION_ADDED", "FILE_ACTION_REMOVED", "FILE_ACTION_MODIFIED", "FILE_ACTION_RENAMED_OLD_NAME", "FILE_ACTION_RENAMED_NEW_NAME" };
		const char* action = actions[fni->Action];

		// many apps save by creating a temp file, deleting the original,
		// and renaming the temp file. that leads to 2 reloads, which is slow.
		// try to detect this case with a simple state machine - we assume
		// the notification order is always the same.

		// TODO

		FAMCodes code;
		switch(fni->Action)
		{
		case FILE_ACTION_ADDED:
		case FILE_ACTION_RENAMED_NEW_NAME:
			code = FAMCreated;
			break;
		case FILE_ACTION_REMOVED:
		case FILE_ACTION_RENAMED_OLD_NAME:
			code = FAMDeleted;
			break;
		case FILE_ACTION_MODIFIED:
			code = FAMChanged;
			break;
		};

		event.code = code;


		// build filename
		// (prepend directory and convert from Windows BSTR)
		strcpy(event.filename, w->dir_name.c_str());
		char* fn = event.filename + w->dir_name.length();
		// .. can't use wcstombs - FileName isn't 0-terminated
		for(int i = 0; i < (int)fni->FileNameLength/2; i++)
			*fn++ = (char)fni->FileName[i];
		*fn = '\0';


		const DWORD ofs = fni->NextEntryOffset;
		// advance to next FILE_NOTIFY_INFORMATION (variable length)
		if(ofs)
			pos += ofs;
		// this was the last entry - no more elements left in buffer.
		else
			break;
	}

	return 0;
}


// if a packet is pending, extract its events and re-issue its watch.
static int get_packet(FAMConnection* fc)
{
	GET_APP_STATE(fc, state);
	const HANDLE hIOCP = state->hIOCP;

	// poll for change notifications from all pending FAMRequests
	DWORD bytes_transferred;
		// used to determine if packet is valid or a kickoff
	ULONG_PTR key;
	OVERLAPPED* povl;
	BOOL got_packet = GetQueuedCompletionStatus(hIOCP, &bytes_transferred, &key, &povl, 0);
	if(!got_packet)	// no new packet - done
		return 1;

	FAMRequest _fr = { (uint)key };
	FAMRequest* const fr = &_fr;
		// if other fields are added, their value is 0;
		// find_watch only looks at reqnum anyway.
	GET_WATCH(fc, fr, w);

	// this is an actual packet, not just a kickoff for issuing the watch.
	// extract the events and push them onto AppState's queue.
	if(bytes_transferred != 0)
		extract_events(fc, fr, w);

	// (re-)issue change notification request.
	// it's safe to reuse Watch.change_buf, because we copied out all events.
	const DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
	                     FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE |
						 FILE_NOTIFY_CHANGE_CREATION;
	const DWORD buf_size = sizeof(w->change_buf);
	memset(&w->ovl, 0, sizeof(w->ovl));
	BOOL ret = ReadDirectoryChangesW(w->hDir, w->change_buf, buf_size, FALSE, filter, &w->dummy_nbytes, &w->ovl, 0);
	if(!ret)
		debug_warn("ReadDirectoryChangesW failed");

	return 0;
}


int FAMPending(FAMConnection* const fc)
{
	GET_APP_STATE(fc, state);
	Events& pending_events = state->pending_events;

	// still have events in the queue?
	// (slight optimization; no need to call get_packet if so)
	if(!pending_events.empty())
		return 1;

	get_packet(fc);

	return !pending_events.empty();
}
 

int FAMNextEvent(FAMConnection* const fc, FAMEvent* const fe)
{
	GET_APP_STATE(fc, state);
	Events& pending_events = state->pending_events;

	if(pending_events.empty())
	{
		debug_warn("FAMNextEvent: no pending events");
		return -1;
	}

	*fe = pending_events.front();
	pending_events.pop_front();
	return 0;
}
