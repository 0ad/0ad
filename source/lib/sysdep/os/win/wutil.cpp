/* Copyright (C) 2023 Wildfire Games.
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
#include "lib/sysdep/os/win/wutil.h"

#include <stdio.h>
#include <stdlib.h>	// __argc

#include "lib/file/file.h"
#include "lib/posix/posix.h"
#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wdbg.h"	// wdbg_assert

#include <shlobj.h>	// SHGetFolderPath

#include <SDL_loadso.h>
#include <SDL_syswm.h>


//-----------------------------------------------------------------------------
// safe allocator

// may be used independently of libc malloc
// (in particular, before _cinit and while calling static dtors).
// used by wpthread critical section code.

void* wutil_Allocate(size_t size)
{
	const DWORD flags = HEAP_ZERO_MEMORY;
	return HeapAlloc(GetProcessHeap(), flags, size);
}

void wutil_Free(void* p)
{
	const DWORD flags = 0;
	HeapFree(GetProcessHeap(), flags, p);
}


//-----------------------------------------------------------------------------
// locks

// several init functions are before called before _cinit.
// POSIX static mutex init may not have been done by then,
// so we need our own lightweight functions.

static CRITICAL_SECTION cs[NUM_CS];
static bool cs_valid;

void wutil_Lock(WinLockId id)
{
	if(!cs_valid)
		return;
	EnterCriticalSection(&cs[id]);
}

void wutil_Unlock(WinLockId id)
{
	if(!cs_valid)
		return;
	LeaveCriticalSection(&cs[id]);
}

bool wutil_IsLocked(WinLockId id)
{
	if(!cs_valid)
		return false;
	const BOOL successfullyEntered = TryEnterCriticalSection(&cs[id]);
	if(!successfullyEntered)
		return true;	// still locked
	LeaveCriticalSection(&cs[id]);
	return false;	// probably not locked
}


static void InitLocks()
{
	for(int i = 0; i < NUM_CS; i++)
		InitializeCriticalSection(&cs[i]);

	cs_valid = true;
}

static void ShutdownLocks()
{
	cs_valid = false;

	for(int i = 0; i < NUM_CS; i++)
		DeleteCriticalSection(&cs[i]);
	memset(cs, 0, sizeof(cs));
}


//-----------------------------------------------------------------------------
// error codes

// only call after a Win32 function indicates failure.
Status StatusFromWin()
{
	switch(GetLastError())
	{
	case ERROR_BUSY:
	case WAIT_TIMEOUT:
		return ERR::AGAIN;
	case ERROR_OPERATION_ABORTED:
		return ERR::ABORTED;

	case ERROR_INVALID_HANDLE:
		return ERR::INVALID_HANDLE;
	case ERROR_INSUFFICIENT_BUFFER:
		return ERR::INVALID_SIZE;
	case ERROR_INVALID_PARAMETER:
	case ERROR_BAD_ARGUMENTS:
		return ERR::INVALID_PARAM;

	case ERROR_OUTOFMEMORY:
	case ERROR_NOT_ENOUGH_MEMORY:
		return ERR::NO_MEM;
	case ERROR_NOT_SUPPORTED:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_PROC_NOT_FOUND:
		return ERR::NOT_SUPPORTED;

	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		return ERR::FILE_NOT_FOUND;
	case ERROR_ACCESS_DENIED:
		return ERR::FILE_ACCESS;

	default:
		return ERR::FAIL;
	}
}

//-----------------------------------------------------------------------------
// directories

// Helper to avoid duplicating this setup
static OsPath GetFolderPath(int csidl)
{
	WinScopedPreserveLastError s;
	HWND hwnd = 0;	// ignored unless a dial-up connection is needed to access the folder
	HANDLE token = 0;
	wchar_t path[MAX_PATH];	// mandated by SHGetFolderPathW
	const HRESULT ret = SHGetFolderPathW(hwnd, csidl, token, 0, path);
	if (!SUCCEEDED(ret))
	{
		debug_printf("SHGetFolderPathW failed with HRESULT = 0x%08lx for csidl = 0x%04x\n", ret, csidl);
		debug_warn("SHGetFolderPathW failed (see debug output)");
	}
	if (GetLastError() == ERROR_NO_TOKEN)	// avoid polluting last error
		SetLastError(0);
	return OsPath(path);
}

OsPath wutil_LocalAppdataPath()
{
	// Local application data.
	return GetFolderPath(CSIDL_LOCAL_APPDATA);
}

OsPath wutil_RoamingAppdataPath()
{
	// Roaming application data.
	return GetFolderPath(CSIDL_APPDATA);
}

OsPath wutil_PersonalPath()
{
	// My documents.
	return GetFolderPath(CSIDL_PERSONAL);
}

//-----------------------------------------------------------------------------

Status wutil_SetPrivilege(const wchar_t* privilege, bool enable)
{
	WinScopedPreserveLastError s;

	HANDLE hToken;
	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken))
		return ERR::_1;

	TOKEN_PRIVILEGES tp;
	if (!LookupPrivilegeValueW(NULL, privilege, &tp.Privileges[0].Luid))
		return ERR::_2;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = enable? SE_PRIVILEGE_ENABLED : 0;

	SetLastError(0);
	const BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, 0, 0);
	if(!ok || GetLastError() != 0)
		return ERR::_3;

	WARN_IF_FALSE(CloseHandle(hToken));
	return INFO::OK;
}


//-----------------------------------------------------------------------------
// module handle

HMODULE wutil_LibModuleHandle()
{
	return GetModuleHandle(0);
}

//-----------------------------------------------------------------------------
// find main window

// this is required by the error dialog and clipboard code.
// note that calling from wutil_Init won't work, because the app will not
// have created its window by then.

static HWND hAppWindow;

void wutil_SetAppWindow(SDL_Window* window)
{
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (SDL_GetWindowWMInfo(window, &wmInfo) && wmInfo.subsystem == SDL_SYSWM_WINDOWS)
		hAppWindow = wmInfo.info.win.window;
}

void* wutil_GetAppHDC()
{
	return GetDC(hAppWindow);
}

void wutil_SetAppWindow(void* hwnd)
{
	hAppWindow = reinterpret_cast<HWND>(hwnd);
}

HWND wutil_AppWindow()
{
	if (hAppWindow)
	{
		// In case of an assertion we might not receive a notification about the
		// closed window. So check it in-place.
		if (IsWindow(hAppWindow))
		{
			// There is a chance that a new window might be opened with the
			// same handle.
			DWORD pid;
			GetWindowThreadProcessId(hAppWindow, &pid);
			if (pid != GetCurrentProcessId())
				hAppWindow = 0;
		}
		else
			hAppWindow = 0;
	}
	return hAppWindow;
}

void wutil_EnableHiDPIOnWindows()
{
	// We build with VS using XP toolkit which doesn't support DPI awareness.
	// It was introduced in 8.1.
	// https://docs.microsoft.com/en-us/windows/win32/api/shellscalingapi/ne-shellscalingapi-process_dpi_awareness
	typedef enum PROCESS_DPI_AWARENESS {
		PROCESS_DPI_UNAWARE,
		PROCESS_SYSTEM_DPI_AWARE,
		PROCESS_PER_MONITOR_DPI_AWARE
	};

	// https://docs.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-setprocessdpiawareness
	using SetProcessDpiAwarenessFunc = HRESULT(WINAPI *)(PROCESS_DPI_AWARENESS);
	void* shcoreDLL = SDL_LoadObject("SHCORE.DLL");
	if (!shcoreDLL)
		return;

	SetProcessDpiAwarenessFunc SetProcessDpiAwareness =
		reinterpret_cast<SetProcessDpiAwarenessFunc>(SDL_LoadFunction(shcoreDLL, "SetProcessDpiAwareness"));
	if (SetProcessDpiAwareness)
		SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	SDL_UnloadObject(shcoreDLL);
}

//-----------------------------------------------------------------------------

Status wutil_Init()
{
	InitLocks();

	return INFO::OK;
}


Status wutil_Shutdown()
{
	ShutdownLocks();

	return INFO::OK;
}
