/* Copyright (C) 2011 Wildfire Games.
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
 * bring in ENet header, with version check
 */

#ifndef INCLUDED_ENET
#define INCLUDED_ENET

#if OS_WIN

// enet/win32.h wants to include winsock2.h which causes conflicts.
// provide some required definitions from winsock.h, then pretend
// we already included winsock.h

typedef uintptr_t SOCKET;
#define INVALID_SOCKET (SOCKET)(~0)
struct fd_set;

#define _WINSOCK2API_	// winsock2.h include guard

#ifndef WIN32
# define WIN32
#endif

#endif	// OS_WIN

#include <enet/enet.h>

#if defined(ENET_VERSION_MAJOR) && !(ENET_VERSION_MAJOR > 1 || ENET_VERSION_MINOR > 2)
#error The game currently requires ENet 1.3.x. You are using an older version, which\
 has an incompatible API and network protocol. Please switch to a newer version.
#endif

#endif	// #ifndef INCLUDED_ENET
