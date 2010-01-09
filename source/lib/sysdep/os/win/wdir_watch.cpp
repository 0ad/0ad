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
 * Win32 directory change notification
 */

#include "precompiled.h"
#include "lib/sysdep/dir_watch.h"

#include "lib/allocators/shared_ptr.h"
#include "lib/path_util.h"	// path_is_subpath
#include "win.h"
#include "winit.h"
#include "wutil.h"


WINIT_REGISTER_MAIN_INIT(wdir_watch_Init);
WINIT_REGISTER_MAIN_SHUTDOWN(wdir_watch_Shutdown);


//-----------------------------------------------------------------------------
// DirHandle

class DirHandle
{
public:
	DirHandle(const fs::wpath& path)
	{
		WinScopedPreserveLastError s;	// CreateFile
		const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
		m_hDir = CreateFileW(path.string().c_str(), FILE_LIST_DIRECTORY, share, 0, OPEN_EXISTING, flags, 0);
	}

	~DirHandle()
	{
		// contrary to MSDN, the canceled IOs do not issue a completion notification.
		// (receiving packets after (unsuccessful) cancellation would be dangerous)
		BOOL ok = CancelIo(m_hDir);
		WARN_IF_FALSE(ok);

		CloseHandle(m_hDir);
		m_hDir = INVALID_HANDLE_VALUE;
	}

	// == INVALID_HANDLE_VALUE if path doesn't exist
	operator HANDLE() const
	{
		return m_hDir;
	}

private:
	HANDLE m_hDir;
};


//-----------------------------------------------------------------------------
// DirWatchRequest

class DirWatchRequest
{
public:
	DirWatchRequest(const fs::wpath& path)
		: m_path(path), m_dirHandle(path), m_data(new u8[dataSize], ArrayDeleter())
	{
		memset(&m_ovl, 0, sizeof(m_ovl));
	}

	~DirWatchRequest()
	{
		WARN_IF_FALSE(CancelIo(m_dirHandle));
	}

	const fs::wpath& Path() const
	{
		return m_path;
	}

	/**
	 * (this is the handle to be associated with the completion port)
	 **/
	HANDLE GetDirHandle() const
	{
		return m_dirHandle;
	}

	LibError Issue()
	{
		if(m_dirHandle == INVALID_HANDLE_VALUE)
			WARN_RETURN(ERR::PATH_NOT_FOUND);

		const BOOL watchSubtree = TRUE;	// (see IntrusiveLink comments)
		const DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_CREATION;
		// not set: FILE_NOTIFY_CHANGE_ATTRIBUTES, FILE_NOTIFY_CHANGE_LAST_ACCESS, FILE_NOTIFY_CHANGE_SECURITY
		DWORD undefined = 0;	// (non-NULL pointer avoids BoundsChecker warning)
		memset(&m_ovl, 0, sizeof(m_ovl));
		const BOOL ok = ReadDirectoryChangesW(m_dirHandle, m_data.get(), dataSize, watchSubtree, filter, &undefined, &m_ovl, 0);
		WARN_IF_FALSE(ok);
		return INFO::OK;
	}

	/**
	 * (call when completion port indicates data is available)
	 **/
	void RetrieveNotifications(DirWatchNotifications& notifications) const
	{
		const FILE_NOTIFY_INFORMATION* fni = (const FILE_NOTIFY_INFORMATION*)m_data.get();
		for(;;)
		{
			// convert name from BSTR (non-zero-terminated) to std::wstring
			cassert(sizeof(wchar_t) == sizeof(WCHAR));
			const size_t nameChars = fni->FileNameLength / sizeof(WCHAR);
			const std::wstring name(fni->FileName, nameChars);

			const fs::wpath pathname(Path()/name);
			const DirWatchNotification::EType type = TypeFromAction(fni->Action);
			notifications.push_back(DirWatchNotification(pathname, type));

			if(!fni->NextEntryOffset)	// this was the last entry.
				break;
			fni = (const FILE_NOTIFY_INFORMATION*)(uintptr_t(fni) + fni->NextEntryOffset);
		}
	}

private:
	static DirWatchNotification::EType TypeFromAction(const DWORD action)
	{
		switch(action)
		{
		case FILE_ACTION_ADDED:
		case FILE_ACTION_RENAMED_NEW_NAME:
			return DirWatchNotification::Created;

		case FILE_ACTION_REMOVED:
		case FILE_ACTION_RENAMED_OLD_NAME:
			return DirWatchNotification::Deleted;

		case FILE_ACTION_MODIFIED:
			return DirWatchNotification::Changed;

		default:
			debug_assert(0);
			return DirWatchNotification::Changed;
		}
	}

	fs::wpath m_path;
	DirHandle m_dirHandle;

	// rationale:
	// - if too small, notifications may be lost! (the CSD-poll application
	//   may be confronted with hundreds of new files in a short time frame)
	// - requests larger than 64 KiB fail on SMB due to packet restrictions.
	static const size_t dataSize = 64*KiB;

	// note: each instance needs their own buffer. (we can't share a central
	// copy because the watches are independent and may be triggered
	// 'simultaneously' before the next poll.)
	shared_ptr<u8> m_data;

	// (ReadDirectoryChangesW's asynchronous mode is triggered by passing
	// a valid OVERLAPPED parameter; we don't use its fields because
	// notification proceeds via completion ports.)
	OVERLAPPED m_ovl;
};

typedef shared_ptr<DirWatchRequest> PDirWatchRequest;


//-----------------------------------------------------------------------------
// IntrusiveLink

// using watches of entire subtrees to satisfy single-directory requests
// requires a list of existing watches. an intrusive, doubly-linked list
// is convenient because removal must occur within the DirWatch destructor.
// since boost::intrusive doesn't automatically remove objects from their
// containers when they are destroyed, we implement a simple circular list
// via sentinel. note that DirWatchManager iterates over DirWatch, not their
// embedded links. we map from link to the parent object via offsetof
// (slightly less complex than storing back pointers to the parents, and
// avoids 'this-pointer used during initialization list' warnings).

class IntrusiveLink
{
public:
	IntrusiveLink()
	{
		m_prev = m_next = this;	// sentinel
	}

	IntrusiveLink(IntrusiveLink* sentinel)
	{
		// insert after sentinel
		m_prev = sentinel;
		m_next = sentinel->m_next;
		m_next->m_prev = this;
		sentinel->m_next = this;
	}

	~IntrusiveLink()
	{
		// remove from list
		m_prev->m_next = m_next;
		m_next->m_prev = m_prev;
	}

	IntrusiveLink* Next() const
	{
		return m_next;
	}

private:
	IntrusiveLink* m_prev;
	IntrusiveLink* m_next;
};


//-----------------------------------------------------------------------------
// DirWatch

struct DirWatch
{
	DirWatch(IntrusiveLink* sentinel, const PDirWatchRequest& request)
		: link(sentinel), request(request)
	{
	}

	IntrusiveLink link;
	PDirWatchRequest request;
};


//-----------------------------------------------------------------------------
// CompletionPort

// this appears to be the best solution for IO notification.
// there are three alternatives:
// - multiple threads with blocking I/O. this is rather inefficient when
//   many directories (e.g. mods) are being watched.
// - normal overlapped I/O: build a contiguous array of the hEvents
//   in all OVERLAPPED structures, and WaitForMultipleObjects.
//   it would be cumbersome to update this array when adding/removing watches.
// - callback notification: a notification function is called when the thread
//   that initiated the I/O (ReadDirectoryChangesW) enters an alertable
//   wait state. it is desirable for notifications to arrive at a single
//   known point - see dir_watch_Poll. unfortunately there doesn't appear to
//   be a reliable and non-blocking means of entering AWS - SleepEx(1) may
//   wait for 10..15 ms if the system timer granularity is low. even worse,
//   it was noted in a previous project that APCs are sometimes delivered from
//   within APIs without having used SleepEx (it seems threads sometimes enter
//   a semi-AWS when calling the kernel).
class CompletionPort
{
public:
	CompletionPort()
	{
		m_hIOCP = 0;	// CreateIoCompletionPort requires 0, not INVALID_HANDLE_VALUE
	}

	~CompletionPort()
	{
		CloseHandle(m_hIOCP);
		m_hIOCP = INVALID_HANDLE_VALUE;
	}

	void Attach(HANDLE hFile, uintptr_t key)
	{
		WinScopedPreserveLastError s;	// CreateIoCompletionPort

		// (when called for the first time, ends up creating m_hIOCP)
		m_hIOCP = CreateIoCompletionPort(hFile, m_hIOCP, (ULONG_PTR)key, 0);
		debug_assert(wutil_IsValidHandle(m_hIOCP));
	}

	LibError Poll(size_t& bytesTransferred, uintptr_t& key, OVERLAPPED*& ovl)
	{
		for(;;)	// don't return abort notifications to caller
		{
			DWORD dwBytesTransferred = 0;
			ULONG_PTR ulKey = 0;
			ovl = 0;
			const DWORD timeout = 0;
			const BOOL gotPacket = GetQueuedCompletionStatus(m_hIOCP, &dwBytesTransferred, &ulKey, &ovl, timeout);
			bytesTransferred = size_t(dwBytesTransferred);
			key = uintptr_t(ulKey);
			if(gotPacket)
				return INFO::OK;

			if(GetLastError() == WAIT_TIMEOUT)
				return ERR::AGAIN;	// NOWARN (nothing pending)
			else if(GetLastError() == ERROR_OPERATION_ABORTED)
				continue;		// watch was canceled - ignore
			else
				return LibError_from_GLE();	// actual error
		}
	}

private:
	HANDLE m_hIOCP;
};


//-----------------------------------------------------------------------------
// DirWatchManager

class DirWatchManager
{
public:
	LibError Add(const fs::wpath& path, PDirWatch& dirWatch)
	{
		debug_assert(path.leaf() == L".");	// must be a directory path (i.e. end in slash)

		// check if this is a subdirectory of a tree that's already being
		// watched (this is much faster than issuing a new watch; it also
		// prevents accidentally watching the same directory twice).
		for(IntrusiveLink* link = m_sentinel.Next(); link != &m_sentinel; link = link->Next())
		{
			DirWatch* const existingDirWatch = (DirWatch*)(uintptr_t(link) - offsetof(DirWatch, link));
			if(path_is_subpath(path.string().c_str(), existingDirWatch->request->Path().string().c_str()))
			{
				dirWatch.reset(new DirWatch(&m_sentinel, existingDirWatch->request));
				return INFO::OK;
			}
		}

		PDirWatchRequest request(new DirWatchRequest(path));
		m_completionPort.Attach(request->GetDirHandle(), (uintptr_t)request.get());
		RETURN_ERR(request->Issue());
		dirWatch.reset(new DirWatch(&m_sentinel, request));
		return INFO::OK;
	}

	LibError Poll(DirWatchNotifications& notifications)
	{
		size_t bytesTransferred; uintptr_t key; OVERLAPPED* ovl;
		RETURN_ERR(m_completionPort.Poll(bytesTransferred, key, ovl));
		DirWatchRequest* request = (DirWatchRequest*)key;
		request->RetrieveNotifications(notifications);
		RETURN_ERR(request->Issue());	// re-issue
		return INFO::OK;
	}

private:
	IntrusiveLink m_sentinel;
	CompletionPort m_completionPort;
};

static DirWatchManager* s_dirWatchManager;


//-----------------------------------------------------------------------------

LibError dir_watch_Add(const fs::wpath& path, PDirWatch& dirWatch)
{
	WinScopedLock lock(WDIR_WATCH_CS);
	return s_dirWatchManager->Add(path, dirWatch);
}

LibError dir_watch_Poll(DirWatchNotifications& notifications)
{
	WinScopedLock lock(WDIR_WATCH_CS);
	return s_dirWatchManager->Poll(notifications);
}


//-----------------------------------------------------------------------------

static LibError wdir_watch_Init()
{
	s_dirWatchManager = new DirWatchManager;
	return INFO::OK;
}

static LibError wdir_watch_Shutdown()
{
	SAFE_DELETE(s_dirWatchManager);
	return INFO::OK;
}
