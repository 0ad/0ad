/**
 * =========================================================================
 * File        : wsock.cpp
 * Project     : 0 A.D.
 * Description : emulate Berkeley sockets on Windows.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "wsock.h"

#include "../delay_load.h"
#include "wposix_internal.h"
#include "wsock_internal.h"

#if MSC_VERSION
#pragma comment(lib, "ws2_32.lib")
#endif


#pragma SECTION_PRE_MAIN(K)
WIN_REGISTER_FUNC(wsock_init);
#pragma FORCE_INCLUDE(wsock_init)
#pragma SECTION_POST_ATEXIT(D)
WIN_REGISTER_FUNC(wsock_shutdown);
#pragma FORCE_INCLUDE(wsock_shutdown)
#pragma SECTION_RESTORE


// IPv6 globals
// These are included in the linux C libraries and in newer platform SDKs,
// so should only be needed in VC++6 or earlier.
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;           // ::
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT; // ::_1

static HMODULE hWs2_32Dll;
static int dll_refs;


// called from delay loader the first time a wsock function is called
// (shortly before the actual wsock function is called).
static LibError wsock_actual_init()
{
	hWs2_32Dll = LoadLibrary("ws2_32.dll");

	// first time: call WSAStartup
	if(!dll_refs++)
	{
		char d[1024];
		if(WSAStartup(0x0002, d) != 0)	// want 2.0
			debug_warn("WSAStartup failed");
	}

	return INFO::OK;
}


// called via module init mechanism. triggers wsock_actual_init when
// someone first calls a wsock function.
static LibError wsock_init()
{
	WDLL_LOAD_NOTIFY("ws2_32", wsock_actual_init);
	return INFO::OK;
}

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

	return INFO::OK;
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
