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

#include "lib/sysdep/win/wdll_delay_load.h"
#include "wposix_internal.h"
#include "wsock_internal.h"
#include "lib/module_init.h"

#if MSC_VERSION
#pragma comment(lib, "ws2_32.lib")
#endif


#pragma SECTION_INIT(5)
WINIT_REGISTER_FUNC(wsock_init);
#pragma FORCE_INCLUDE(wsock_init)
#pragma SECTION_SHUTDOWN(5)
WINIT_REGISTER_FUNC(wsock_shutdown);
#pragma FORCE_INCLUDE(wsock_shutdown)
#pragma SECTION_RESTORE


uint16_t htons(uint16_t s)
{
	return (s >> 8) | ((s & 0xff) << 8);
}

// IPv6 globals
// These are included in the linux C libraries and in newer platform SDKs,
// so should only be needed in VC++6 or earlier.
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;           // ::
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT; // ::_1


//-----------------------------------------------------------------------------
// manual import of certain functions

// don't delay-load because we don't want to require these functions to be
// present. the user must be able to check if they are available (currently,
// on Win2k with IPv6 update or WinXP). can't use compile-time HAVE_* to
// make that decision because we don't want to distribute a separate EXE.

// function pointers, automatically initialized before any use of ws2_32.dll
static int (WINAPI *pgetnameinfo)(const struct sockaddr*, socklen_t, char*, socklen_t, char*, socklen_t, unsigned int);
static int (WINAPI *pgetaddrinfo)(const char*, const char*, const struct addrinfo*, struct addrinfo**);
static void (WINAPI *pfreeaddrinfo)(struct addrinfo*);

static HMODULE hWs2_32Dll;

static void ImportOptionalFunctions()
{
	*(void**)&pgetnameinfo = GetProcAddress(hWs2_32Dll, "getnameinfo");
	*(void**)&pgetaddrinfo = GetProcAddress(hWs2_32Dll, "getaddrinfo");
	*(void**)&pfreeaddrinfo = GetProcAddress(hWs2_32Dll, "freeaddrinfo");
}

int getnameinfo(const struct sockaddr* sa, socklen_t salen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, unsigned int flags)
{
	return pgetnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
}

int getaddrinfo(const char* nodename, const char* servname, const struct addrinfo* hints, struct addrinfo** res)
{
	return pgetaddrinfo(nodename, servname, hints, res);
}

void freeaddrinfo(struct addrinfo* ai)
{
	pfreeaddrinfo(ai);
}


//-----------------------------------------------------------------------------

static ModuleInitState initState;

// called from delay loader the first time a wsock function is called
// (shortly before the actual wsock function is called).
static LibError wsock_actual_init()
{
	if(!ModuleShouldInitialize(&initState))
		return INFO::OK;

	hWs2_32Dll = LoadLibrary("ws2_32.dll");

	char d[1024];
	int ret = WSAStartup(0x0002, d);	// want 2.0
	debug_assert(ret == 0);

	ImportOptionalFunctions();

	return INFO::OK;
}

static LibError wsock_init()
{
	// trigger wsock_actual_init when someone first calls a wsock function.
	static WdllLoadNotify loadNotify = { "ws2_32", wsock_actual_init };
	wdll_add_notify(&loadNotify);
	return INFO::OK;
}


static LibError wsock_shutdown()
{
	if(!ModuleShouldShutdown(&initState))
		return INFO::OK;

	int ret = WSACleanup();
	debug_assert(ret >= 0);

	FreeLibrary(hWs2_32Dll);

	return INFO::OK;
}
