/* Copyright (C) 2015 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/wiocp.h"

#include "lib/file/file.h"	// ERR::IO
#include "lib/sysdep/os/win/wutil.h"


void AttachToCompletionPort(HANDLE hFile, HANDLE& hIOCP, ULONG_PTR key, DWORD numConcurrentThreads)
{
	WinScopedPreserveLastError s;	// CreateIoCompletionPort

	// (when called for the first time, ends up creating hIOCP)
	hIOCP = CreateIoCompletionPort(hFile, hIOCP, key, numConcurrentThreads);
	ENSURE(wutil_IsValidHandle(hIOCP));
}


Status PollCompletionPort(HANDLE hIOCP, DWORD timeout, DWORD& bytesTransferred, ULONG_PTR& key, OVERLAPPED*& ovl)
{
	if(hIOCP == 0)
		return ERR::INVALID_HANDLE;	// NOWARN (happens if called before the first Attach)

	WinScopedPreserveLastError s;

	bytesTransferred = 0;
	key = 0;
	ovl = 0;
	if(GetQueuedCompletionStatus(hIOCP, &bytesTransferred, &key, &ovl, timeout))
		return INFO::OK;
	const Status ret = StatusFromWin();
	if(ret == ERR::AGAIN || ret == ERR::ABORTED)	// avoid polluting last error
		SetLastError(0);
	return ret;	// NOWARN (let caller decide what to do)
}
