#include "precompiled.h"
#include "lib/sysdep/os/win/wiocp.h"

#include "lib/file/file.h"	// ERR::IO
#include "lib/sysdep/os/win/wutil.h"


void AttachToCompletionPort(HANDLE hFile, HANDLE& hIOCP, ULONG_PTR key, DWORD numConcurrentThreads)
{
	WinScopedPreserveLastError s;	// CreateIoCompletionPort

	// (when called for the first time, ends up creating hIOCP)
	hIOCP = CreateIoCompletionPort(hFile, hIOCP, key, numConcurrentThreads);
	debug_assert(wutil_IsValidHandle(hIOCP));
}


LibError PollCompletionPort(HANDLE hIOCP, DWORD timeout, DWORD& bytesTransferred, ULONG_PTR& key, OVERLAPPED*& ovl)
{
	if(hIOCP == 0)
		return ERR::INVALID_HANDLE;	// NOWARN (happens if called before the first Attach)

	bytesTransferred = 0;
	key = 0;
	ovl = 0;
	if(GetQueuedCompletionStatus(hIOCP, &bytesTransferred, &key, &ovl, timeout))
		return INFO::OK;
	if(GetLastError() == WAIT_TIMEOUT)
		return ERR::AGAIN;	// NOWARN (nothing pending)
	else
		return ERR::FAIL;	// NOWARN (let caller decide what to do)
}
