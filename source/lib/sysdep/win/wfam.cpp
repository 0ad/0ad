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
	const std::string dir_name;
	HANDLE hDir;

	// history to detect series of notifications, so we can skip
	// redundant reloads (slow)
	std::string last_path;
	DWORD last_action;	// FILE_ACTION_* codes or 0
	DWORD last_ticks;	// timestamp via GetTickCount

	OVERLAPPED ovl;
		// fields aren't used.
		// overlapped I/O completation notification is via IOCP.

	char change_buf[15000];
		// better be big enough - if too small,
		// we miss changes made to a directory.
		// issue code uses sizeof(change_buf) to determine size.

	// these are returned in FAMEvent. could get them via FAMNextEvent's
	// fc parameter and associating packets with FAMRequest,
	// but storing them here is more convenient.
	FAMConnection* fc;
	FAMRequest* fr;


	Watch(const std::string& _dir_name, HANDLE _hDir)
		: dir_name(_dir_name), last_path("")
	{
		hDir = _hDir;

		last_action = 0;
		last_ticks = 0;

		memset(&ovl, 0, sizeof(ovl));

		// change_buf[] doesn't need init
	}

	~Watch()
	{
		CloseHandle(hDir);
		hDir = INVALID_HANDLE_VALUE;
	}
};


// list of all active watches to detect duplicates and
// for easier cleanup. only store pointer in container -
// they're not copy-equivalent.
typedef std::map<std::string, Watch*> Watches;
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

	AppState(const char* _app_name)
		: app_name(_app_name)
	{
		hIOCP = 0;
		// not INVALID_HANDLE_VALUE! (CreateIoCompletionPort requirement)
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
#define GET_APP_STATE(fc, ptr_name)\
	AppState* const ptr_name = (AppState*)fc->internal;\
	if(!ptr_name)\
	{\
		debug_warn("no FAM connection");\
		return -1;\
	}

#define GET_WATCH(fr, ptr_name)\
	Watch* const ptr_name = (Watch*)fr->internal;\
	if(!ptr_name)\
	{\
		debug_warn("FAMRequest.internal invalid!");\
		return -1;\
	}


int FAMOpen2(FAMConnection* const fc, const char* const app_name)
{
	try
	{
		fc->internal = new AppState(app_name);
	}
	catch(std::bad_alloc)
	{
		fc->internal = 0;
	}

	// either (VC6) new returned 0, or we caught bad_alloc => fail.
	if(!fc->internal)
		return -1;

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
static void get_packet(AppState*);


int FAMMonitorDirectory(FAMConnection* const fc, char* const _dir, FAMRequest* const fr, void* const user)
{
	GET_APP_STATE(fc, state);
	Watches& watches = state->watches;
	HANDLE& hIOCP = state->hIOCP;

	const std::string dir(_dir);

	// make sure dir is not already being watched
	WatchIt it = watches.find(dir);
	if(it != watches.end())
		return -1;

	// open handle to directory
	const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
	HANDLE hDir = CreateFile(_dir, FILE_LIST_DIRECTORY, share, 0, OPEN_EXISTING, flags, 0);
	if(hDir == INVALID_HANDLE_VALUE)
		return -1;

	// create Watch and associate with FAM structs
	Watch* const w = new Watch(dir, hDir);
	watches[dir] = w;
	w->fc = fc;
	w->fr = fr;

	// associate Watch* with the directory handle. when we receive a packet
	// from the IOCP, we will need to re-issue the watch and find the
	// corresponding FAMRequest.
	const ULONG_PTR key = (ULONG_PTR)w;

	// create IOCP (if not already done) and attach hDir to it
	hIOCP = CreateIoCompletionPort(hDir, hIOCP, key, 0);
	if(hIOCP == 0 || hIOCP == INVALID_HANDLE_VALUE)
	{
		delete w;
		CloseHandle(hDir);
		return -1;
	}

	// post a dummy kickoff packet; the IOCP polling code will "re"issue
	// the corresponding watch. this keeps the ReadDirectoryChangesW call
	// and directory <--> Watch association code in one place.
	//
	// we call get_packet so that it's issued immediately,
	// instead of only at the next call to FAMPending.
	PostQueuedCompletionStatus(hIOCP, 0, key, 0);
	get_packet(state);

	fr->internal = w;

	return 0;
}


int FAMCancelMonitor(FAMConnection* const fc, FAMRequest* const fr)
{
	GET_APP_STATE(fc, state);
	GET_WATCH(fr, w)

	// TODO

	return -1;
}


static int extract_events(Watch* const w)
{
	FAMConnection* const fc = w->fc;
	FAMRequest* const fr = w->fr;

	GET_APP_STATE(fc, state);
	Events& events = state->pending_events;

	// will be modified for each event and added to events
	FAMEvent event_template;
	event_template.fc = fc;
	event_template.fr = *fr;

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


		//
		// convert filename from Windows BSTR to portable C string
		//

		char* fn = event.filename;
		const int num_chars = fni->FileNameLength/2;
		for(int i = 0; i < num_chars; i++)
		{
			char c = (char)fni->FileName[i];
			if(c == '\\')
				c = '/';
			*fn++ = c;
		}
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
void get_packet(AppState* const state)
{
	// poll for change notifications from all pending FAMRequests
	DWORD bytes_transferred;
		// used to determine if packet is valid or a kickoff
	ULONG_PTR key;
	OVERLAPPED* povl;
	BOOL got_packet = GetQueuedCompletionStatus(state->hIOCP, &bytes_transferred, &key, &povl, 0);
	if(!got_packet)	// no new packet - done
		return;

	Watch* w = (Watch*)key;

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
	BOOL ret = ReadDirectoryChangesW(w->hDir, w->change_buf, buf_size, FALSE, filter, 0, &w->ovl, 0);
	if(!ret)
		debug_warn("ReadDirectoryChangesW failed");
}


int FAMPending(FAMConnection* const fc)
{
	GET_APP_STATE(fc, state);
	Events& pending_events = state->pending_events;

	// still have events in the queue?
	// (slight optimization; no need to call get_packet if so)
	if(!pending_events.empty())
		return 1;

	get_packet(state);

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
