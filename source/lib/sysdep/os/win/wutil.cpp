/* Copyright (c) 2015 Wildfire Games
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
 * various Windows-specific utilities
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
#include "lib/sysdep/os/win/winit.h"

#include <shlobj.h>	// SHGetFolderPath


WINIT_REGISTER_EARLY_INIT(wutil_Init);
WINIT_REGISTER_LATE_SHUTDOWN(wutil_Shutdown);


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
// command line

// copy of GetCommandLine string. will be tokenized and then referenced by
// the argv pointers.
static wchar_t* argvContents;

int s_argc = 0;
wchar_t** s_argv = 0;

static void ReadCommandLine()
{
	const wchar_t* commandLine = GetCommandLineW();
	// (this changes as quotation marks are removed)
	size_t numChars = wcslen(commandLine);
	argvContents = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, (numChars+1)*sizeof(wchar_t));
	wcscpy_s(argvContents, numChars+1, commandLine);

	// first pass: tokenize string and count number of arguments
	bool ignoreSpace = false;
	for(size_t i = 0; i < numChars; i++)
	{
		switch(argvContents[i])
		{
		case '"':
			ignoreSpace = !ignoreSpace;
			// strip the " character
			memmove(argvContents+i, argvContents+i+1, (numChars-i)*sizeof(wchar_t));
			numChars--;
			i--;
			break;

		case ' ':
			if(!ignoreSpace)
			{
				argvContents[i] = '\0';
				s_argc++;
			}
			break;
		}
	}
	s_argc++;

	// have argv entries point into the tokenized string
	s_argv = (wchar_t**)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, s_argc*sizeof(wchar_t*));
	wchar_t* nextArg = argvContents;
	for(int i = 0; i < s_argc; i++)
	{
		s_argv[i] = nextArg;
		nextArg += wcslen(nextArg)+1;
	}
}


int wutil_argc()
{
	return s_argc;
}

wchar_t** wutil_argv()
{
	ENSURE(s_argv);
	return s_argv;
}


static void FreeCommandLine()
{
	HeapFree(GetProcessHeap(), 0, s_argv);
	HeapFree(GetProcessHeap(), 0, argvContents);
}


bool wutil_HasCommandLineArgument(const wchar_t* arg)
{
	for(int i = 0; i < s_argc; i++)
	{
		if(!wcscmp(s_argv[i], arg))
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// directories

// (NB: wutil_Init is called before static ctors => use placement new)
static OsPath* systemPath;
static OsPath* executablePath;
static OsPath* localAppdataPath;
static OsPath* roamingAppdataPath;
static OsPath* personalPath;

const OsPath& wutil_SystemPath()
{
	return *systemPath;
}

const OsPath& wutil_ExecutablePath()
{
	return *executablePath;
}

const OsPath& wutil_LocalAppdataPath()
{
	return *localAppdataPath;
}

const OsPath& wutil_RoamingAppdataPath()
{
	return *roamingAppdataPath;
}

const OsPath& wutil_PersonalPath()
{
	return *personalPath;
}

// Helper to avoid duplicating this setup
static OsPath* GetFolderPath(int csidl)
{
	HWND hwnd = 0;	// ignored unless a dial-up connection is needed to access the folder
	HANDLE token = 0;
	wchar_t path[MAX_PATH];	// mandated by SHGetFolderPathW
	const HRESULT ret = SHGetFolderPathW(hwnd, csidl, token, 0, path);
	if (!SUCCEEDED(ret))
	{
		debug_printf("SHGetFolderPathW failed with HRESULT = 0x%08lx for csidl = 0x%04x\n", ret, csidl);
		debug_warn("SHGetFolderPathW failed (see debug output)");
	}
	if(GetLastError() == ERROR_NO_TOKEN)	// avoid polluting last error
		SetLastError(0);
	return new(wutil_Allocate(sizeof(OsPath))) OsPath(path);
}

static void GetDirectories()
{
	WinScopedPreserveLastError s;

	// system directory
	{
		const UINT length = GetSystemDirectoryW(0, 0);
		ENSURE(length != 0);
		std::wstring path(length, '\0');
		const UINT charsWritten = GetSystemDirectoryW(&path[0], length);
		ENSURE(charsWritten == length-1);
		systemPath = new(wutil_Allocate(sizeof(OsPath))) OsPath(path);
	}

	// executable's directory
	executablePath = new(wutil_Allocate(sizeof(OsPath))) OsPath(sys_ExecutablePathname().Parent());

	// roaming application data
	roamingAppdataPath = GetFolderPath(CSIDL_APPDATA);

	// local application data
	localAppdataPath = GetFolderPath(CSIDL_LOCAL_APPDATA);

	// my documents
	personalPath = GetFolderPath(CSIDL_PERSONAL);
}


static void FreeDirectories()
{
	systemPath->~OsPath();
	wutil_Free(systemPath);
	executablePath->~OsPath();
	wutil_Free(executablePath);
	localAppdataPath->~OsPath();
	wutil_Free(localAppdataPath);
	roamingAppdataPath->~OsPath();
	wutil_Free(roamingAppdataPath);
	personalPath->~OsPath();
	wutil_Free(personalPath);
}


//-----------------------------------------------------------------------------
// user32 fix

// HACK: make sure a reference to user32 is held, even if someone
// decides to delay-load it. this fixes bug #66, which was the
// Win32 mouse cursor (set via user32!SetCursor) appearing as a
// black 32x32(?) rectangle. the underlying cause was as follows:
// powrprof.dll was the first client of user32, causing it to be
// loaded. after we were finished with powrprof, we freed it, in turn
// causing user32 to unload. later code would then reload user32,
// which apparently terminally confused the cursor implementation.
//
// since we hold a reference here, user32 will never unload.
// of course, the benefits of delay-loading are lost for this DLL,
// but that is unavoidable. it is safer to force loading it, rather
// than documenting the problem and asking it not be delay-loaded.
static HMODULE hUser32Dll;

static void ForciblyLoadUser32Dll()
{
	hUser32Dll = LoadLibraryW(L"user32.dll");
}

// avoids Boundschecker warning
static void FreeUser32Dll()
{
	FreeLibrary(hUser32Dll);
}


//-----------------------------------------------------------------------------
// memory

static void EnableLowFragmentationHeap()
{
	if(IsDebuggerPresent())
	{
		// and the debug heap isn't explicitly disabled,
		char* var = getenv("_NO_DEBUG_HEAP");
		if(!var || var[0] != '1')
			return;	// we can't enable the LFH
	}

#if WINVER >= 0x0501
	WUTIL_FUNC(pHeapSetInformation, BOOL, (HANDLE, HEAP_INFORMATION_CLASS, void*, size_t));
	WUTIL_IMPORT_KERNEL32(HeapSetInformation, pHeapSetInformation);
	if(pHeapSetInformation)
	{
		ULONG flags = 2;	// enable LFH
		pHeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, &flags, sizeof(flags));
	}
#endif	// #if WINVER >= 0x0501
}


//-----------------------------------------------------------------------------
// Wow64

// Wow64 'helpfully' redirects all 32-bit apps' accesses of
// %windir%\\system32\\drivers to %windir%\\system32\\drivers\\SysWOW64.
// that's bad, because the actual drivers are not in the subdirectory. to
// work around this, provide for temporarily disabling redirection.

static WUTIL_FUNC(pIsWow64Process, BOOL, (HANDLE, PBOOL));
static WUTIL_FUNC(pWow64DisableWow64FsRedirection, BOOL, (PVOID*));
static WUTIL_FUNC(pWow64RevertWow64FsRedirection, BOOL, (PVOID));

static bool isWow64;

static void ImportWow64Functions()
{
	WUTIL_IMPORT_KERNEL32(IsWow64Process, pIsWow64Process);
	WUTIL_IMPORT_KERNEL32(Wow64DisableWow64FsRedirection, pWow64DisableWow64FsRedirection);
	WUTIL_IMPORT_KERNEL32(Wow64RevertWow64FsRedirection, pWow64RevertWow64FsRedirection);
}

static void DetectWow64()
{
	// function not found => running on 32-bit Windows
	if(!pIsWow64Process)
	{
		isWow64 = false;
		return;
	}

	BOOL isWow64Process = FALSE;
	const BOOL ok = pIsWow64Process(GetCurrentProcess(), &isWow64Process);
	WARN_IF_FALSE(ok);
	isWow64 = (isWow64Process == TRUE);
}

bool wutil_IsWow64()
{
	return isWow64;
}


WinScopedDisableWow64Redirection::WinScopedDisableWow64Redirection()
{
	// note: don't just check if the function pointers are valid. 32-bit
	// Vista includes them but isn't running Wow64, so calling the functions
	// would fail. since we have to check if actually on Wow64, there's no
	// more need to verify the pointers (their existence is implied).
	if(!wutil_IsWow64())
		return;
	const BOOL ok = pWow64DisableWow64FsRedirection(&m_wasRedirectionEnabled);
	WARN_IF_FALSE(ok);
}

WinScopedDisableWow64Redirection::~WinScopedDisableWow64Redirection()
{
	if(!wutil_IsWow64())
		return;
	const BOOL ok = pWow64RevertWow64FsRedirection(m_wasRedirectionEnabled);
	WARN_IF_FALSE(ok);
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

#ifndef LIB_STATIC_LINK

#include "lib/sysdep/os/win/wdll_main.h"

HMODULE wutil_LibModuleHandle()
{
	HMODULE hModule;
	const DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
	const BOOL ok = GetModuleHandleEx(flags, (LPCWSTR)&wutil_LibModuleHandle, &hModule);
	// (avoid ENSURE etc. because we're called from debug_DisplayError)
	wdbg_assert(ok);
	return hModule;
}

#else

HMODULE wutil_LibModuleHandle()
{
	return GetModuleHandle(0);
}

#endif



//-----------------------------------------------------------------------------
// find main window

// this is required by the error dialog and clipboard code.
// note that calling from wutil_Init won't work, because the app will not
// have created its window by then.

static HWND hAppWindow;

static BOOL CALLBACK FindAppWindowByPid(HWND hWnd, LPARAM UNUSED(lParam))
{
	DWORD pid;
	DWORD tid = GetWindowThreadProcessId(hWnd, &pid);
	UNUSED2(tid);

	if(pid == GetCurrentProcessId())
		hAppWindow = hWnd;

	return TRUE;	// keep calling
}

HWND wutil_AppWindow()
{
	if(!hAppWindow)
	{
		WARN_IF_FALSE(EnumWindows(FindAppWindowByPid, 0));
		// (hAppWindow may still be 0 if we haven't created a window yet)
	}

	return hAppWindow;
}


//-----------------------------------------------------------------------------

static Status wutil_Init()
{
	InitLocks();

	ForciblyLoadUser32Dll();

	EnableLowFragmentationHeap();

	ReadCommandLine();

	GetDirectories();

	ImportWow64Functions();
	DetectWow64();

	return INFO::OK;
}


static Status wutil_Shutdown()
{
	FreeCommandLine();

	FreeUser32Dll();

	ShutdownLocks();

	FreeDirectories();

	return INFO::OK;
}
