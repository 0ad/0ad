/**
 * =========================================================================
 * File        : wsock.cpp
 * Project     : 0 A.D.
 * Description : emulate Berkeley sockets on Windows.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"

#include "win_internal.h"
#include "lib.h"
#include "wsock.h"
#include "delay_load.h"



#if MSC_VERSION
#pragma comment(lib, "ws2_32.lib")
#endif


#pragma data_seg(WIN_CALLBACK_POST_ATEXIT(b))
WIN_REGISTER_FUNC(wsock_shutdown);
#pragma data_seg()


// IPv6 globals
// These are included in the linux C libraries and in newer platform SDKs,
// so should only be needed in VC++6 or earlier.
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;           // ::
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT; // ::1

static HMODULE hWs2_32Dll;
static int dll_refs;


// called from delay loader the first time a wsock function is called
// (shortly before the actual wsock function is called).
static LibError wsock_init()
{
	hWs2_32Dll = LoadLibrary("ws2_32.dll");

	// first time: call WSAStartup
	if(!dll_refs++)
	{
		char d[1024];
		if(WSAStartup(0x0002, d) != 0)	// want 2.0
			debug_warn("WSAStartup failed");
	}

	return ERR_OK;
}

WDLL_LOAD_NOTIFY("ws2_32", wsock_init);



static LibError wsock_shutdown()
{
	// call WSACleanup if DLL was used
	// (this way is easier to understand than ONCE in loop below)
	if(dll_refs > 0)
		if(WSACleanup() < 0)
			debug_warn("WSACleanup failed");

	// remove all references
	while(dll_refs-- > 0)
		FreeLibrary(hWs2_32Dll);

	return ERR_OK;
}




// manual import instead of delay-load because we don't want to require
// these functions to be present. the user must be able to check if they
// are available (currently, on Win2k with IPv6 update or WinXP).
// can't use compile-time HAVE_* to make that decision because
// we don't want to distribute a separate binary for this.
//
// note: can't import at startup because we don't want to load wsock unless necessary
// don't use delay load because we don't want to confuse error handling for other users
//
// don't bother caching - these functions themselves take a while and aren't time-critical

static void* import(const char* name)
{
	return GetProcAddress(hWs2_32Dll, name);
}

fp_getnameinfo_t  import_getnameinfo()  { return (fp_getnameinfo_t )import("getnameinfo" ); }
fp_getaddrinfo_t  import_getaddrinfo()  { return (fp_getaddrinfo_t )import("getaddrinfo" ); }
fp_freeaddrinfo_t import_freeaddrinfo() { return (fp_freeaddrinfo_t)import("freeaddrinfo"); }





uint16_t htons(uint16_t s)
{
	return (s >> 8) | ((s & 0xff) << 8);
}
