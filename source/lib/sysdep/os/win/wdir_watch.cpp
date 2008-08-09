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

#include "lib/path_util.h"	// path_is_subpath
#include "win.h"
#include "winit.h"
#include "wutil.h"


WINIT_REGISTER_MAIN_INIT(wdir_watch_Init);
WINIT_REGISTER_MAIN_SHUTDOWN(wdir_watch_Shutdown);


//-----------------------------------------------------------------------------
// DirWatchRequest

class DirWatchRequest
{
public:
	DirWatchRequest()
		: m_data(new char[dataSize])
	{
		m_undefined = 0;
		memset(&m_ovl, 0, sizeof(m_ovl));
	}

	/**
	 * @return the buffer containing one or more FILE_NOTIFY_INFORMATION
	 **/
	char* Results() const
	{
		debug_assert(HasOverlappedIoCompleted(&m_ovl));
		return m_data.get();
	}

	void Issue(HANDLE hDir)
	{
		memset(&m_ovl, 0, sizeof(m_ovl));

		const DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_CREATION;

		// (this is much faster than watching every directory separately.)
		const BOOL watchSubtree = TRUE;

		const BOOL ok = ReadDirectoryChangesW(hDir, m_data.get(), dataSize, watchSubtree, filter, &m_undefined, &m_ovl, 0);
		WARN_IF_FALSE(ok);
	}

private:
	// rationale:
	// - if too small, notifications may be lost! (the CSD-poll application
	//   may be confronted with hundreds of new files in a short timeframe)
	// - requests larger than 64 KiB fail on SMB due to packet restrictions.
	static const size_t dataSize = 64*KiB;

	// note: each instance needs their own buffer. (we can't share a central
	// copy because the watches are independent and may be triggered
	// 'simultaneously' before the next poll.)
	shared_ptr<char> m_data;

	// (passing this instead of a null pointer avoids a BoundsChecker warning
	// but has no other value since its contents are undefined.)
	DWORD m_undefined;

	// (ReadDirectoryChangesW's asynchronous mode is triggered by passing
	// a valid OVERLAPPED parameter; we don't use its fields because
	// notification proceeds via completion ports.)
	OVERLAPPED m_ovl;
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
//   an "AWS" when calling the kernel).
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
		DWORD dwBytesTransferred = 0;
		ULONG_PTR ulKey = 0;
		ovl = 0;
		{
			const DWORD timeout = 0;
			const BOOL gotPacket = GetQueuedCompletionStatus(m_hIOCP, &dwBytesTransferred, &ulKey, &ovl, timeout);
			bytesTransferred = size_t(bytesTransferred);
			key = uintptr_t(ulKey);
			if(gotPacket)
				return INFO::OK;
		}
		if(GetLastError() == WAIT_TIMEOUT)
			return ERR::AGAIN;	// NOWARN
		// else: there was actually an error
		return LibError_from_GLE();
	}

private:
	HANDLE m_hIOCP;
};


//-----------------------------------------------------------------------------

// (watches are removed by resetting shared_ptr; the DirWatch dtor must
// notify DirWatchManager and remove itself from the list there)
static void RemoveFromList(DirWatch* dirWatch);

class DirWatch
{
public:
	DirWatch(const fs::wpath& path, CompletionPort& completionPort)
		: m_path(path)
	{
		m_path /= L"\\";	// must end in slash

		// open handle to directory
		{
			WinScopedPreserveLastError s;	// CreateFile
			const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
			const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
			const std::wstring dirPath = m_path.external_directory_string();
			m_hDir = CreateFileW(dirPath.c_str(), FILE_LIST_DIRECTORY, share, 0, OPEN_EXISTING, flags, 0);
			if(m_hDir == INVALID_HANDLE_VALUE)
				throw std::runtime_error("");
		}

		completionPort.Attach(m_hDir, (uintptr_t)this);
	}

	~DirWatch()
	{
		// contrary to MSDN, the RDC IOs do not issue a completion notification.
		// no packet was received on the IOCP while or after canceling in a test.
		// if cancel fails and future packets are received, we'd need weak pointers..
		BOOL ok = CancelIo(m_hDir);
		debug_assert(ok);

		CloseHandle(m_hDir);
		m_hDir = INVALID_HANDLE_VALUE;

		RemoveFromList(this);
	}

	void Issue()
	{
		m_request.Issue(m_hDir);
	}

	bool IncludesDirectory(const fs::wpath& path) const
	{
		return path_is_subpathw(path.string().c_str(), m_path.string().c_str());
	}

	const fs::wpath& Path() const
	{
		return m_path;
	}

	const char* Results() const
	{
		return m_request.Results();
	}

private:
	fs::wpath m_path;
	HANDLE m_hDir;
	DirWatchRequest m_request;
};


//-----------------------------------------------------------------------------
// DirWatchNotificationQueue

// DirWatchRequest needs to reissue its IO immediately, else subsequent
// changes may be lost. using a second buffer is more complicated and
// also not perfectly safe. instead it is preferable to copy the data to
// a queue, thus completely decoupling the application and IO logic.
class DirWatchNotificationQueue
{
public:
	/**
	 * extract all notifications from a buffer and add them to the queue.
	 *
	 * @param path of the watched directory; necessary since
	 *   ReadDirectoryChangesW only returns filenames.
	 * @param packets points to at least one (variable-length)
	 *   FILE_NOTIFY_INFORMATION.
	 **/
	void Enqueue(const fs::wpath& path, const char* const packets)
	{
		const char* pos = packets;
		for(;;)
		{
			const FILE_NOTIFY_INFORMATION* fni = (const FILE_NOTIFY_INFORMATION*)pos;
			const DirWatchNotification::Event type = TypeFromAction(fni->Action);

			// convert from BSTR (non-zero-terminated)
			debug_assert(sizeof(wchar_t) == sizeof(WCHAR));
			const size_t nameChars = fni->FileNameLength / sizeof(WCHAR);
			const std::wstring name(fni->FileName, nameChars);

			const fs::wpath pathname(path/name);
			m_queue.push(DirWatchNotification(pathname, type));

			const DWORD ofs = fni->NextEntryOffset;
			if(!ofs)	// this was the last entry.
				break;
			pos += ofs;
		}
	}

	/**
	 * retrieve a pending notification from the queue.
	 *
	 * @param notification is only valid if INFO::OK was returned.
	 *
	 * @return ERR::AGAIN if no notifications are pending, otherwise INFO::OK.
	 **/
	LibError Dequeue(DirWatchNotification& notification)
	{
		if(m_queue.empty())
			return ERR::AGAIN;	// NOWARN
		notification = m_queue.front();
		m_queue.pop();
		return INFO::OK;
	}

private:
	static DirWatchNotification::Event TypeFromAction(const DWORD action)
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
			throw std::logic_error("invalid action");
		}
	}

	std::queue<DirWatchNotification> m_queue;
};


//-----------------------------------------------------------------------------
// list of active watches (required for detecting duplicates)

class DirWatchManager
{
public:
	LibError Add(const fs::wpath& path, PDirWatch& dirWatch)
	{
		// check if this is a subdirectory of a tree that's already being
		// watched (this is much faster than issuing a new watch; it also
		// prevents accidentally watching the same directory twice).
		for(Watches::const_iterator it = m_watches.begin(); it != m_watches.end(); ++it)
		{
			const PDirWatch& existingDirWatch = *it;
			if(existingDirWatch->IncludesDirectory(path))
			{
				dirWatch = existingDirWatch;
				return INFO::OK;
			}
		}

		try
		{
			dirWatch.reset(new DirWatch(path, m_completionPort));
			dirWatch->Issue();
		}
		catch(std::runtime_error e)
		{
			return ERR::FAIL;
		}

		m_watches.push_back(dirWatch);
		return INFO::OK;
	}

	void Remove(DirWatch* dirWatch)
	{
		for(Watches::iterator it = m_watches.begin(); it != m_watches.end(); ++it)
		{
			if(it->get() == dirWatch)
			{
				m_watches.erase(it);
				break;
			}
		}
	}

	LibError Poll(DirWatchNotification& notification)
	{
		size_t bytesTransferred; uintptr_t key; OVERLAPPED* ovl;
		if(m_completionPort.Poll(bytesTransferred, key, ovl) == INFO::OK)
		{
			DirWatch& dirWatch = *(DirWatch*)key;
			const fs::wpath& path = dirWatch.Path();
			const char* const packets = dirWatch.Results();

			m_queue.Enqueue(path, packets);

			dirWatch.Issue();	// re-issue
		}

		return m_queue.Dequeue(notification);
	}

private:
	typedef std::list<PDirWatch> Watches;
	Watches m_watches;

	CompletionPort m_completionPort;
	DirWatchNotificationQueue m_queue;
};

static DirWatchManager* s_dirWatchManager;


//-----------------------------------------------------------------------------

LibError dir_watch_Add(const fs::wpath& path, PDirWatch& dirWatch)
{
	WinScopedLock lock(WDIR_WATCH_CS);
	return s_dirWatchManager->Add(path, dirWatch);
}

static void RemoveFromList(DirWatch* dirWatch)
{
	WinScopedLock lock(WDIR_WATCH_CS);
	return s_dirWatchManager->Remove(dirWatch);
}

LibError dir_watch_Poll(DirWatchNotification& notification)
{
	WinScopedLock lock(WDIR_WATCH_CS);
	return s_dirWatchManager->Poll(notification);
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
