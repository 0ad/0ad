// directory watch implementation for Win32
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

#include "lib.h"
#include "win_internal.h"

#include <assert.h>

#include <string>
#include <map>
#include <list>


#pragma data_seg(".LIB$WTX")
WIN_REGISTER_FUNC(wdir_watch_shutdown);
#pragma data_seg()


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


// list of all active watches. required, since we have to be able to
// cancel watches; also makes detecting duplicates possible.
//
// only store pointer in container - they're not copy-equivalent
// (dtor would close hDir).
//
// key is intptr_t "reqnum"; they aren't reused to avoid problems
// with stale reqnums after cancelling;
// hence, map instead of vector and freelist.
struct Watch;
typedef std::map<intptr_t, Watch*> Watches;
typedef Watches::iterator WatchIt;

// list of all active watches to detect duplicates and
// for easier cleanup. only store pointer in container -
// they're not copy-equivalent.
static Watches watches;

// don't worry about size; heap-allocated.
struct Watch
{
	intptr_t reqnum;

	std::string dir_name;
	HANDLE hDir;

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


	Watch(intptr_t _reqnum, const std::string& _dir_name, HANDLE _hDir)
		: reqnum(_reqnum), dir_name(_dir_name), hDir(_hDir)
	{
		memset(&ovl, 0, sizeof(ovl));
		// change_buf[] doesn't need init

		watches[reqnum] = this;
	}

	~Watch()
	{
		CloseHandle(hDir);
		hDir = INVALID_HANDLE_VALUE;

		watches.erase(reqnum);
	}
};


// global state
static HANDLE hIOCP = 0;
	// not INVALID_HANDLE_VALUE! (CreateIoCompletionPort requirement)

static intptr_t last_reqnum = 1000;
	// start value provides a little protection against passing in bogus reqnums
	// (we don't bother using a tag for safety though - it isn't important)


typedef std::list<std::string> Events;
static Events pending_events;
	// rationale:
	// we need a queue, instead of just taking events from the change_buf,
	// because we need to re-issue the watch immediately after it returns
	// data. of course we can't have the app read from the buffer while
	// waiting for RDC to write to the buffer - race condition.
	// an alternative to a queue would be to allocate another buffer,
	// but that's more complicated, and this way is cleaner anyway.


static int wdir_watch_shutdown()
{
	CloseHandle(hIOCP);
	hIOCP = INVALID_HANDLE_VALUE;

	// free all (dynamically allocated) Watch-es
	for(WatchIt it = watches.begin(); it != watches.end(); ++it)
		delete it->second;

	return 0;
}


// HACK - see call site
static int get_packet();


// path: portable and relative, must add current directory and convert to native
// better to use a cached string from rel_chdir - secure
int dir_add_watch(const char* const dir, intptr_t* const _reqnum)
{
	int err = -1;
	WIN_SAVE_LAST_ERROR;	// Create*

	intptr_t reqnum;
	*_reqnum = 0;

	{
	const std::string dir_s(dir);

	// make sure dir is not already being watched
	for(WatchIt it = watches.begin(); it != watches.end(); ++it)
		if(dir == it->second->dir_name)
			goto fail;

	// open handle to directory
	const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
	const HANDLE hDir = CreateFile(dir, FILE_LIST_DIRECTORY, share, 0, OPEN_EXISTING, flags, 0);
	if(hDir == INVALID_HANDLE_VALUE)
		goto fail;

	// assign a new (unique) request number. don't do this earlier - prevents
	// DOS via wasting reqnums due to invalid directory parameters.
	// need it before binding dir to IOCP because it is our "key".
	if(last_reqnum == INT_MAX)
	{
		debug_warn("dir_add_watch: request numbers are no longer unique");
		CloseHandle(hDir);
		goto fail;
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
		goto fail;
	}

	// allocate watch, add to list, associate with reqnum
	try
	{
		Watch* w = new Watch(reqnum, dir_s, hDir);
		assert(w != 0);		// happened once; heap corruption?

		// add trailing \ if not already there
		if(dir_s[dir_s.length()-1] != '\\')
			w->dir_name += '\\';
	}
	catch(std::bad_alloc)
	{
		goto fail;
	}


	// post a dummy kickoff packet; the IOCP polling code will "re"issue
	// the corresponding watch. this keeps the ReadDirectoryChangesW call
	// and directory <--> Watch association code in one place.
	//
	// we call get_packet so that it's issued immediately,
	// instead of only at the next call to dir_get_changed_file.
	PostQueuedCompletionStatus(hIOCP, 0, key, 0);
	get_packet();
	}

	err = 0;
	*_reqnum = reqnum;

fail:
	WIN_RESTORE_LAST_ERROR;
	return err;
}


int dir_cancel_watch(const intptr_t reqnum)
{
	if(reqnum < 0)
		return ERR_INVALID_PARAM;

	Watch* w = watches[reqnum];
	if(!w)
	{
		debug_warn("dir_cancel_watch: watches[reqnum] invalid");
		return -1;
	}

	// contrary to dox, the RDC IOs do not issue a completion notification.
	// no packet was received on the IOCP while or after cancelling in a test.
	//
	// if cancel somehow fails though, no matter - the Watch is freed, and
	// its reqnum isn't reused; if we receive a packet, it's ignored.
	BOOL ret = CancelIo(w->hDir);

	delete w;
	return ret? 0 : -1;
}


static int extract_events(Watch* w)
{
	// points to current FILE_NOTIFY_INFORMATION;
	// char* simplifies advancing to the next (variable length) FNI.
	char* pos = w->change_buf;

	// for every packet in buffer: (there's at least one)
	for(;;)
	{
		const FILE_NOTIFY_INFORMATION* const fni = (const FILE_NOTIFY_INFORMATION*)pos;

		// convert filename from Windows BSTR
		// (can't use wcstombs - FileName isn't 0-terminated)
		std::string fn = w->dir_name;
		for(int i = 0; i < (int)fni->FileNameLength/2; i++)
			fn += (char)fni->FileName[i];

		pending_events.push_back(fn);

		// advance to next entry in buffer (variable length)
		const DWORD ofs = fni->NextEntryOffset;
		// .. this one was the last - done.
		if(!ofs)
			break;
		pos += ofs;
	}

	return 0;
}


// if a packet is pending, extract its events and re-issue its watch.
static int get_packet()
{
	// poll for change notifications from all pending watches
	DWORD bytes_transferred;
		// used to determine if packet is valid or a kickoff
	ULONG_PTR key;
	OVERLAPPED* povl;
	BOOL got_packet = GetQueuedCompletionStatus(hIOCP, &bytes_transferred, &key, &povl, 0);
	if(!got_packet)	// no new packet - done
		return 1;

	Watch* w = watches[(intptr_t)key];

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
	BOOL ret = ReadDirectoryChangesW(w->hDir, w->change_buf, buf_size, FALSE, filter, &w->dummy_nbytes, &w->ovl, 0);
	if(!ret)
		debug_warn("ReadDirectoryChangesW failed");

	return 0;
}


int dir_get_changed_file(char* fn)
{
	// queue one or more events, or return 1 if none pending.
	CHECK_ERR(get_packet());

	// nothing to return; call again later.
	if(pending_events.empty())
		return ERR_AGAIN;

	const std::string& fn_s = pending_events.front();
	strcpy(fn, fn_s.c_str());
	pending_events.pop_front();

	return 0;
}
