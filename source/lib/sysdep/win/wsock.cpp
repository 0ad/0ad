// Berkeley sockets emulation for Win32
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

#include "win_internal.h"
#include "lib.h"
#include "wsock.h"

#include <assert.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif


fp_getnameinfo_t getnameinfo;
fp_getaddrinfo_t getaddrinfo;
fp_freeaddrinfo_t freeaddrinfo;


/* IPv6 globals
These are included in the linux C libraries, and in newer platform SDK's, so
should only be needed in VC++6 or earlier.
*/
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;        /* :: */
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;   /* ::1 */


WIN_REGISTER_MODULE(wsock);

static int wsock_init()
{
	char d[1024];
	CHECK_ERR(WSAStartup(0x0002, d));	// want 2.0

	const HMODULE hWs2_32Dll = LoadLibrary("ws2_32.dll");
	*(void**)&getaddrinfo  = GetProcAddress(hWs2_32Dll, "getaddrinfo");
	*(void**)&getnameinfo  = GetProcAddress(hWs2_32Dll, "getnameinfo");
	*(void**)&freeaddrinfo = GetProcAddress(hWs2_32Dll, "freeaddrinfo");
	FreeLibrary(hWs2_32Dll);
		// make sure the reference is released so BoundsChecker
		// doesn't complain. it won't actually be unloaded anyway -
		// there is at least one other reference.

	return 0;
}


static int wsock_shutdown()
{
	return WSACleanup();
}


uint16_t htons(uint16_t s)
{
	return (s >> 8) | ((s & 0xff) << 8);
}
