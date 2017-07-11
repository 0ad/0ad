/* Copyright (C) 2010 Wildfire Games.
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

/*
 * I/O completion port
 */

#ifndef INCLUDED_WIOCP
#define INCLUDED_WIOCP

#include "lib/sysdep/os/win/win.h"

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
//   known point - see dir_watch_Poll. however, other APIs might also
//   trigger APC delivery.

// @param hIOCP 0 to create a new port
extern void AttachToCompletionPort(HANDLE hFile, HANDLE& hIOCP, ULONG_PTR key, DWORD numConcurrentThreads = 0);

extern Status PollCompletionPort(HANDLE hIOCP, DWORD timeout, DWORD& bytesTransferred, ULONG_PTR& key, OVERLAPPED*& ovl);

#endif	// #ifndef INCLUDED_WIOCP
