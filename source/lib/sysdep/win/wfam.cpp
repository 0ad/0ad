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

#include "res/res.h"

#include "sysdep/win/win_internal.h"


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


static HANDLE hIOCP = 0;
	// not INVALID_HANDLE_VALUE! (CreateIoCompletionPort requirement)


static const size_t WATCH_BUF_SIZE = 16384;
	// better be enough - if too small, we miss changes made to a directory.


// ReadDirectoryChangesW must be called again after each time it returns data,
// so we need to pass this struct to the notification "callback".
// we do this implicitly with the guarantee that &ovl = &Watch (verified via
// assert). rationale: the Key param is already in use (tells the callback to
// call RDC - see KEY_ADD_WATCH_ONLY); this way of passing the parameter is
// cleaner IMO than stuffing the address in an unused member of OVERLAPPED.
struct Watch
{
	OVERLAPPED ovl;
		// we don't use any of its fields.

	void* buf;

	HANDLE hDir;

	bool watch_subdirs;

	// history to detect series of notifications, so we can skip
	// redundant reloads (slow)
	std::string last_path;
	DWORD last_action;	// FILE_ACTION_* codes or 0
	DWORD last_ticks;	// timestamp via GetTickCount


	Watch(HANDLE hDir, bool watch_subdirs)
	{
		this->hDir = hDir;
		this->watch_subdirs = watch_subdirs;

		buf = mem_alloc(WATCH_BUF_SIZE, 32);
			// ReadDirectoryChangesW requirement: at least 4-byte alignment.

		memset(&ovl, 0, sizeof(ovl));

		last_action = 0;
	}

	~Watch()
	{
//		mem_free(buf);
// FIXME: mem has already shut down when this is called.

		CancelIo(hDir);

		CloseHandle(hDir);
		hDir = INVALID_HANDLE_VALUE;
	}
};


// don't store directly in container - not copy-equivalent.
typedef std::map<std::string, Watch*> Watches;
typedef Watches::iterator WatchIt;
static Watches watches;


static void cleanup(void)
{
	CloseHandle(hIOCP);

	// container holds dynamically allocated Watch structs
	for(WatchIt it = watches.begin(); it != watches.end(); ++it)
		delete it->second;
}


int dir_watch_abort(const char* const dir)
{
	// find watch
	const std::string dir_s(dir);
	WatchIt it = watches.find(dir_s);
	if(it == watches.end())
		return -1;

	delete it->second;
	watches.erase(it);
	return 0;
}


// it's nice to have only 1 call site of ReadDirectoryChangesW, namely
// in the notification "callback". posting an event to the IOCP with
// KEY_ADD_WATCH_ONLY tells it to call RDC; normal events pass KEY_NORMAL.
static enum
{
	KEY_NORMAL,
	KEY_ADD_WATCH_ONLY
};


int dir_add_watch(const char* const dir, const bool watch_subdirs)
{
	ONCE(atexit2(cleanup));

	// make sure dir is not already being watched
	const std::string dir_s(dir);
	WatchIt it = watches.find(dir_s);
	if(it != watches.end())
		return -1;

	// open handle to directory
	const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
	const HANDLE hDir = CreateFile(dir, FILE_LIST_DIRECTORY, share, 0, OPEN_EXISTING, flags, 0);
	if(hDir == INVALID_HANDLE_VALUE)
		return -1;

	// create IOCP (if not already done) and bind dir to it
	hIOCP = CreateIoCompletionPort(hDir, hIOCP, KEY_NORMAL, 0);
	if(hIOCP == 0 || hIOCP == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDir);
		return -1;
	}

	// insert
	Watch* const w = new Watch(hDir, watch_subdirs);
	watches[dir_s] = w;

	// tell the notification "callback" to actually add the watch
	const DWORD bytes_transferred = 0;
	PostQueuedCompletionStatus(hIOCP, bytes_transferred, KEY_ADD_WATCH_ONLY, &w->ovl);

	return 0;
}


static int dir_changed(const FILE_NOTIFY_INFORMATION*, Watch*);

// purpose of this routine (intended to be called once a frame):
// file notification may come at any time. by forcing the reloads
// to take place here, we don't require everything to be thread-safe.
//
// added bonus: can actually "poll" for changes here - obviates a worker
// thread, mutex, and 2 queues.
int allow_reload()
{
	// deque and process all pending IOCP packets
	for(;;)
	{
		// check for change_notification or add_watch packets
		DWORD bytes_transferred;	// unused
		ULONG_PTR key;
		OVERLAPPED* povl;
		BOOL got_packet = GetQueuedCompletionStatus(hIOCP, &bytes_transferred, &key, &povl, 0);
		if(!got_packet)
			break;

		// retrieve the corresponding Watch
		// (see Watch definition for rationale)
		Watch* const w = (Watch*)povl;
		assert(offsetof(struct Watch, ovl) == 0);

		// (re-)request change notification from now on
		const DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
		                     FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE |
							 FILE_NOTIFY_CHANGE_CREATION;
		ReadDirectoryChangesW(w->hDir, w->buf, WATCH_BUF_SIZE, TRUE, filter, 0, &w->ovl, 0);

		// dir_add_watch requests the watch be added;
		// we didn't actually receive any change notification
		if(key == KEY_ADD_WATCH_ONLY)
			continue;

		FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)w->buf;
		for(;;)
		{
			dir_changed(fni, w);

			DWORD ofs = fni->NextEntryOffset;
			// this was the last entry
			if(!ofs)
				break;

			// advance to next FILE_NOTIFY_INFORMATION (variable length)
			(char*&)fni += ofs;
		}
	}

	return 0;
}


int dir_changed(const FILE_NOTIFY_INFORMATION* const fni, Watch* const w)
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

	res_reload(fn);

	return 0;
}
