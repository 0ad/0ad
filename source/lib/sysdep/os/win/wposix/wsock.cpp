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

#include "lib/sysdep/os/win/wdll_delay_load.h"
#include "wposix_internal.h"
#include "wsock_internal.h"
#include "lib/module_init.h"

#if MSC_VERSION
#pragma comment(lib, "ws2_32.lib")
#endif

WINIT_REGISTER_MAIN_INIT(wsock_Init);
WINIT_REGISTER_MAIN_SHUTDOWN(wsock_Shutdown);


// IPv6 globals
// These are included in the linux C libraries and in newer platform SDKs,
// so should only be needed in VC++6 or earlier.
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;           // ::
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT; // ::_1


//-----------------------------------------------------------------------------
// 'optional' IPv6 routines

// we hide the function pointers behind stub functions - this avoids
// surprising users. speed is irrelevant here. manually writing these stubs
// is ugly, but delay-load error handling is hairy, so don't use that.
//
// the first call of these stubs must trigger wsock_ActualInit in case no
// other winsock function was called yet. we can't simply rely on
// ModuleShouldInitialize because taking references prevents shutdown.
// adding an extra haveInitialized flag would be redundant. instead,
// enter a clever but safe hack: we call a harmless winsock function that
// triggers the delay load or does nothing if init has already happened.

typedef int (WINAPI *Pgetnameinfo)(const struct sockaddr*, socklen_t, char*, socklen_t, char*, socklen_t, unsigned int);
typedef int (WINAPI *Pgetaddrinfo)(const char*, const char*, const struct addrinfo*, struct addrinfo**);
typedef void (WINAPI *Pfreeaddrinfo)(struct addrinfo*);
static Pgetnameinfo pgetnameinfo;
static Pgetaddrinfo pgetaddrinfo;
static Pfreeaddrinfo pfreeaddrinfo;

int getnameinfo(const struct sockaddr* sa, socklen_t salen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, unsigned int flags)
{
	(void)htonl(0);	// trigger init if not done already
	if(!pgetnameinfo)
	{
		errno = ENOSYS;
		return -1;
	}

	return pgetnameinfo(sa, salen, host, hostlen, serv, servlen, flags);
}

int getaddrinfo(const char* nodename, const char* servname, const struct addrinfo* hints, struct addrinfo** res)
{
	(void)htonl(0);		// trigger init if not done already
	if(!pgetaddrinfo)
	{
		errno = ENOSYS;
		return -1;
	}

	return pgetaddrinfo(nodename, servname, hints, res);
}

void freeaddrinfo(struct addrinfo* ai)
{
	// (no dummy htonl call or checking of the function pointer is needed
	// since getaddrinfo must succeed to get a valid addrinfo*.)

	pfreeaddrinfo(ai);
}


static void ImportOptionalFunctions()
{
	// (by the time we get here, ws2_32.dll will have been loaded, so
	// this isn't the only reference and can be freed immediately)
	HMODULE hWs2_32Dll = LoadLibrary("ws2_32.dll");
	pgetnameinfo = (Pgetnameinfo)GetProcAddress(hWs2_32Dll, "getnameinfo");
	pgetaddrinfo = (Pgetaddrinfo)GetProcAddress(hWs2_32Dll, "getaddrinfo");
	pfreeaddrinfo = (Pfreeaddrinfo)GetProcAddress(hWs2_32Dll, "freeaddrinfo");
	FreeLibrary(hWs2_32Dll);
}


//-----------------------------------------------------------------------------

static ModuleInitState initState;

// called from delay loader the first time a wsock function is called
// (shortly before the actual wsock function is called).
static LibError wsock_ActualInit()
{
	if(!ModuleShouldInitialize(&initState))
		return INFO::OK;

	char d[1024];
	int ret = WSAStartup(0x0002, d);	// want 2.0
	debug_assert(ret == 0);

	ImportOptionalFunctions();

	return INFO::OK;
}

static LibError wsock_Init()
{
	// trigger wsock_ActualInit when someone first calls a winsock function.
	static WdllLoadNotify loadNotify = { "ws2_32", wsock_ActualInit };
	wdll_add_notify(&loadNotify);
	return INFO::OK;
}


static LibError wsock_Shutdown()
{
	if(!ModuleShouldShutdown(&initState))
		return INFO::OK;

	int ret = WSACleanup();
	debug_assert(ret >= 0);

	return INFO::OK;
}
