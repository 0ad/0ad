// Windows-specific directory change notification
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

// TODO Jan: Where is this file?
//#include "wfam.h"

#include "win_internal.h"

#if 0

static const size_t CHANGE_BUF_SIZE = 15000;
	// better be enough - if too small, we miss changes made to a directory.


// don't worry about size: the user only passes around a pointer
// to this struct, due to the pImpl idiom. this is heap-allocated.
struct FAMRequest_
{
	std::string dir_name;
	HANDLE hDir;

	// history to detect series of notifications, so we can skip
	// redundant reloads (slow)
	std::string last_path;
	DWORD last_action;	// FILE_ACTION_* codes or 0
	DWORD last_ticks;	// timestamp via GetTickCount

	OVERLAPPED ovl;
		// we don't use any of its fields.
		// overlapped I/O completation notification is via IOCP.
		// rationale: see below.
	char changes[CHANGE_BUF_SIZE];


	FAMRequest_(const char* _dir_name)
		: dir_name(_dir_name), last_path("")
	{
		last_action = 0;
		last_ticks = 0;

		memset(&ovl, 0, sizeof(ovl));

		// changes[] doesn't need init
	}
};


// don't worry about size: the user only passes around a pointer
// to this struct, due to the pImpl idiom. this is heap-allocated.
struct FAMConnection_
{
	std::string app_name;


	HANDLE hIOCP;

// queue necessary - race condition if pass to app and re-issue
// needs to be FIFO, and don't want to constantly shuffle items (can be rather large)
// around => list
	typedef std::list<FAMEvent> Events;
	Events pending_events;

	// list of all pending requests to detect duplicates and
	// for easier cleanup. only store pointer in container -
	// they're not copy-equivalent.
	typedef std::map<std::string, FAMRequest*> Requests;
	typedef Requests::iterator RequestIt;
	Requests requests;

	FAMConnection_(const char* _app_name)
		: app_name(_app_name)
	{
		hIOCP = 0;
		// not INVALID_HANDLE_VALUE! (CreateIoCompletionPort requirement)
	}

	~FAMConnection_()
	{
		CloseHandle(hIOCP);
		hIOCP = INVALID_HANDLE_VALUE;

		// container holds dynamically allocated Watch structs
		//	for(WatchIt it = watches.begin(); it != watches.end(); ++it)
		//		delete it->second;
	}
};





// rationale for using I/O completion ports for notification:
// alternatives:
// - multiple threads with blocking I/O. a good many mount points
//   and therefore directory watches are possible, so this is out.
// - normal overlapped I/O: build a contiguous array of the hEvents
//   in all OVERLAPPED structures, and WaitForMultipleObjects.
//   having to (re)build the array after every watch add/remove sucks.
// - callback notification: notification function is called when the thread
//   that initiated the I/O (ReadDirectoryChangesW) enters an alertable
//   wait state (e.g. with SleepEx). it would be nice to be able to
//   check for notifications from the mainline - would obviate
//   the separate worker thread for RDC and 2 queues to drive it.
//   unfortunately, cannot come up with a robust yet quick way of
//   working off all pending APCs - SleepEx(1) is a hack. even worse,
//   it was noted in a previous project that APCs are sometimes delivered
//   from within Windows APIs, without having used SleepEx
//   (it seems threads enter an "AWS" sometimes when calling the kernel).
//
// IOCPs work well and are elegant; have not yet noticed any drawbacks.



	


// ReadDirectoryChangesW must be called again after each time it returns data,
// so we need to pass along the associated Request.
// since we issue RDC immediately, instead of sending a bogus packet
// to the IOCP that triggers the issue, we don't need the key parameter
// for anything - use it to pass along the Request.
// cleaner than assuming &ovl = &Request, or stuffing it in an unused
// member of OVERLAPPED.





int dir_watch_abort(const char* const dir)
{
	// find watch
/*	const std::string dir_s(dir);
	WatchIt it = watches.find(dir_s);
	if(it == watches.end())
		return -1;

	delete it->second;
	watches.erase(it);
*/
	return 0;
}


// it'd be nice to have only 1 call site of ReadDirectoryChangesW, namely
// in the notification "callback". however, posting a dummy event to the IOCP
// and having the callback issue RDC is a bit ugly, and loses changes made
// before the poll routine is first called.

static int dir_watch_issue(FAMRequest_* fr)
{
	// (re-)request change notification from now on
	const DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
	                     FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE |
						 FILE_NOTIFY_CHANGE_CREATION;
	BOOL ret = ReadDirectoryChangesW(fr->hDir, fr->changes, CHANGE_BUF_SIZE, FALSE, filter, 0, &fr->ovl, 0);
	return ret? 0 : -1;
}


int dir_add_watch(const char* const dir, const bool watch_subdirs)
{
return 0;
}

void FAMCancelMonitor(FAMConnection*, FAMRequest* req)
{
}







































void wfam_shutdown()
{
}



int FAMOpen2(FAMConnection* const fc, const char* app_name)
{
	try
	{
		fc->internal = new FAMConnection_(app_name);
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
	FAMConnection_*& fc_ = (FAMConnection_*)fc->internal;
	if(!fc_)
	{
		debug_warn("FAMClose: already closed");
		return -1;
	}

	delete fc_;
	fc_ = 0;
	return 0;
}


int FAMMonitorDirectory(FAMConnection* fc, char* dir, FAMRequest* fr, void* user)
{
	FAMConnection_* fc_ = (FAMConnection_*)fc->internal;

/*
	// make sure dir is not already being watched
	const std::string dir_s(dir);
	WatchIt it = watches.find(dir_s);
	if(it != watches.end())
		return -1;
*/
	HANDLE hDir  = INVALID_HANDLE_VALUE;
	HANDLE& hIOCP = fc_->hIOCP;

	// open handle to directory
	const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
	hDir = CreateFile(dir, FILE_LIST_DIRECTORY, share, 0, OPEN_EXISTING, flags, 0);
	if(hDir == INVALID_HANDLE_VALUE)
		return -1;

	// create IOCP (if not already done) and bind dir to it
	hIOCP = CreateIoCompletionPort(hDir, hIOCP, KEY_NORMAL, 0);
	if(hIOCP == 0 || hIOCP == INVALID_HANDLE_VALUE)
	{
fail:
		CloseHandle(hDir);
		return -1;
	}



	// insert
	Watch* const w = new Watch(hDir, watch_subdirs);
	watches[dir_s] = w;

	dir_watch_issue(w);

	return 0;

}


// added bonus: can actually "poll" for changes here - obviates a worker
// thread, mutex, and 2 queues.



static int extract_events(FAMConnection* conn, FAMRequest* req)
{
	FAMConnection_ conn_ = (FAMConnection_*)conn->internal;
	const FILE_NOTIFY_INFORMATION* fni = (const FILE_NOTIFY_INFORMATION*)req->changes;
	Events& events = fc_->pending_events;

	FAMEvent event_template;
	event_template.conn = conn;
	event_template.req = req;

	// for every packet in buffer: (there's at least one)
	for(;;)
	{
		// many apps save by creating a temp file, deleting the original,
		// and renaming the temp file. that leads to 2 reloads, which is slow.
		// try to detect this case with a simple state machine - we assume
		// the notification order is always the same.

		// TODO:

		const char* actions[] = { "", "FILE_ACTION_ADDED", "FILE_ACTION_REMOVED", "FILE_ACTION_MODIFIED", "FILE_ACTION_RENAMED_OLD_NAME", "FILE_ACTION_RENAMED_NEW_NAME" };
		const char* action = actions[fni->Action];

		// convert Windows BSTR-style path to
		// portable C string path for the resource manager.
		// HACK: convert in place, we copy it into 
		char fn[MAX_PATH];
		char* p = fn;
		const int num_chars = fni->FileNameLength/2;
		for(int i = 0; i < num_chars; i++)
		{
			char c = (char)fni->FileName[i];
			if(c == '\\')
				c = '/';
			*p++ = c;
		}
		*p = '\0';

		// don't want to expose details

		events.push_back(event_template);


		const DWORD ofs = fni->NextEntryOffset;
		// advance to next FILE_NOTIFY_INFORMATION (variable length)
		if(ofs)
			(char*&)fni += ofs;
		// this was the last entry - no more elements left in buffer.
		else
			break;
	}

	return 0;


	res_reload(fn);

	return 0;
}


int FAMPending(FAMConnection* fc)
{
	FAMConnection_* const fc_ = (FAMConnection_*)fc->internal;
	Events& pending_events = fc_->pending_events;

	if(!pending_events.empty())
		return 1;

	// check if new buffer has been filled
	DWORD bytes_transferred;	// unused
	ULONG_PTR key;
	OVERLAPPED* povl;
	BOOL got_packet = GetQueuedCompletionStatus(fc_->hIOCP, &bytes_transferred, &key, &povl, 0);
	if(!got_packet)
		return 0;

	CHECK_ERR(extract_events(conn, req));

//	dir_watch_issue(buf);




	fc->fni = (FILE_NOTIFY_INFORMATION*)w->buf;
	return 1;
}
 

int FAMNextEvent(FAMConnection* const fc, FAMEvent* const fe)
{
	FAMConnection_* const fc_ = (FAMConnection_*)fc->internal;
	Events& pending_events = fc_->pending_events;

	if(!fe)
	{
		debug_warn("FAMNextEvent: fe = 0");
		return ERR_INVALID_PARAM;
	}

	if(pending_events.empty())
	{
		debug_warn("FAMNextEvent: no pending events");
		return -1;
	}

	*fe = pending_events.front();
	pending_events.pop_front();
	return 0;
}
 

#endif
