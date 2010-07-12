/* Copyright (c) 2010 Wildfire Games
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
 * emulate Berkeley sockets on Windows.
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/wposix/wsock.h"

#include "lib/sysdep/os/win/wdll_delay_load.h"
#include "lib/sysdep/os/win/wposix/wposix_internal.h"
#include "lib/sysdep/os/win/wposix/wsock_internal.h"
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
// the first call of these stubs must trigger OnLoad in case no
// other winsock function was called yet.
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
	HMODULE hWs2_32Dll = LoadLibraryW(L"ws2_32.dll");
	pgetnameinfo = (Pgetnameinfo)GetProcAddress(hWs2_32Dll, "getnameinfo");
	pgetaddrinfo = (Pgetaddrinfo)GetProcAddress(hWs2_32Dll, "getaddrinfo");
	pfreeaddrinfo = (Pfreeaddrinfo)GetProcAddress(hWs2_32Dll, "freeaddrinfo");
	FreeLibrary(hWs2_32Dll);
}


//-----------------------------------------------------------------------------

static LibError Init()
{
	char d[1024];
	int ret = WSAStartup(0x0002, d);	// want 2.0
	debug_assert(ret == 0);

	ImportOptionalFunctions();

	return INFO::OK;
}

static void Shutdown()
{
	int ret = WSACleanup();
	debug_assert(ret >= 0);
}

static ModuleInitState initState;

// called from delay loader the first time a wsock function is called
// (shortly before the actual wsock function is called).
static LibError OnLoad()
{
	return ModuleInit(&initState, Init);
}

static LibError wsock_Init()
{
	// trigger OnLoad when someone first calls a wsock function.
	static WdllLoadNotify loadNotify = { "ws2_32", OnLoad };
	wdll_add_notify(&loadNotify);
	return INFO::OK;
}

static LibError wsock_Shutdown()
{
	ModuleShutdown(&initState, Shutdown);
	return INFO::OK;
}
