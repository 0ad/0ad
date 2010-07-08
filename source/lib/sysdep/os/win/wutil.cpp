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
 * various Windows-specific utilities
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/wutil.h"

#include <stdio.h>
#include <stdlib.h>	// __argc

#include "lib/file/file.h"
#include "lib/file/vfs/vfs.h"
#include "lib/posix/posix.h"
#include "lib/sysdep/os/win/win.h"
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
LibError LibError_from_GLE(bool warn_if_failed)
{
	LibError err = ERR::FAIL;
	switch(GetLastError())
	{
	case ERROR_OUTOFMEMORY:
	case ERROR_NOT_ENOUGH_MEMORY:
		err = ERR::NO_MEM;
		break;
	case ERROR_INVALID_HANDLE:
	case ERROR_INVALID_PARAMETER:
	case ERROR_BAD_ARGUMENTS:
		err = ERR::INVALID_PARAM;
		break;
	case ERROR_INSUFFICIENT_BUFFER:
		err = ERR::BUF_SIZE;
		break;
	case ERROR_ACCESS_DENIED:
		err = ERR::FILE_ACCESS;
		break;
	case ERROR_NOT_SUPPORTED:
		err = ERR::NOT_SUPPORTED;
		break;
	case ERROR_CALL_NOT_IMPLEMENTED:
		err = ERR::NOT_IMPLEMENTED;
		break;
	case ERROR_PROC_NOT_FOUND:
		err = ERR::NO_SYS;
		break;
	case ERROR_BUSY:
		err = ERR::AGAIN;
		break;
	case ERROR_FILE_NOT_FOUND:
		err = ERR::VFS_FILE_NOT_FOUND;
		break;
	case ERROR_PATH_NOT_FOUND:
		err = ERR::VFS_DIR_NOT_FOUND;
		break;
	default:
		break;	// err already set above
	}

	if(warn_if_failed)
		DEBUG_WARN_ERR(err);
	return err;
}


LibError LibError_from_win32(DWORD ret, bool warn_if_failed)
{
	if(ret != FALSE)
		return INFO::OK;
	return LibError_from_GLE(warn_if_failed);
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
	debug_assert(s_argv);
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

fs::wpath wutil_DetectExecutablePath()
{
	wchar_t buf[MAX_PATH+1] = {0};
	const DWORD len = GetModuleFileNameW(GetModuleHandle(0), buf, MAX_PATH);
	debug_assert(len != 0);
	const fs::wpath modulePathname(buf);
	return modulePathname.branch_path();
}

// (NB: wutil_Init is called before static ctors => use placement new)
static fs::wpath* systemPath;
static fs::wpath* executablePath;
static fs::wpath* appdataPath;

const fs::wpath& wutil_SystemPath()
{
	return *systemPath;
}

const fs::wpath& wutil_ExecutablePath()
{
	return *executablePath;
}

const fs::wpath& wutil_AppdataPath()
{
	return *appdataPath;
}


static void GetDirectories()
{
	WinScopedPreserveLastError s;
	wchar_t path[MAX_PATH+1] = {0};

	// system directory
	{
		const UINT charsWritten = GetSystemDirectoryW(path, MAX_PATH);
		debug_assert(charsWritten != 0);
		systemPath = new(wutil_Allocate(sizeof(fs::wpath))) fs::wpath(path);
	}

	// executable's directory
	executablePath = new(wutil_Allocate(sizeof(fs::wpath))) fs::wpath(wutil_DetectExecutablePath());

	// application data
	{
		HWND hwnd = 0;	// ignored unless a dial-up connection is needed to access the folder
		HANDLE token = 0;
		const HRESULT ret = SHGetFolderPathW(hwnd, CSIDL_APPDATA, token, 0, path);
		debug_assert(SUCCEEDED(ret));
		appdataPath = new(wutil_Allocate(sizeof(fs::wpath))) fs::wpath(path);
	}
}


static void FreeDirectories()
{
	systemPath->~basic_path();
	wutil_Free(systemPath);
	executablePath->~basic_path();
	wutil_Free(executablePath);
	appdataPath->~basic_path();
	wutil_Free(appdataPath);
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
// version

static wchar_t windowsVersionString[20];
static size_t windowsVersion;	// see WUTIL_VERSION_*

static void DetectWindowsVersion()
{
	// note: don't use GetVersion[Ex] because it gives the version of the
	// emulated OS when running an app with compatibility shims enabled.
	HKEY hKey;
	if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		DWORD size = sizeof(windowsVersionString);
		(void)RegQueryValueExW(hKey, L"CurrentVersion", 0, 0, (LPBYTE)windowsVersionString, &size);

		int major = 0, minor = 0;
		int ret = swscanf_s(windowsVersionString, L"%d.%d", &major, &minor);
		debug_assert(ret == 2);
		debug_assert(major <= 0xFF && minor <= 0xFF);
		windowsVersion = (major << 8) | minor;

		RegCloseKey(hKey);
	}
	else
		debug_assert(0);
}


const wchar_t* wutil_WindowsFamily()
{
	debug_assert(windowsVersion != 0);
	switch(windowsVersion)
	{
	case WUTIL_VERSION_2K:
		return L"Win2k";
	case WUTIL_VERSION_XP:
		return L"WinXP";
	case WUTIL_VERSION_XP64:
		return L"WinXP64";
	case WUTIL_VERSION_VISTA:
		return L"Vista";
	case WUTIL_VERSION_7:
		return L"Win7";
	default:
		return L"Windows";
	}
}


const wchar_t* wutil_WindowsVersionString()
{
	debug_assert(windowsVersionString[0] != '\0');
	return windowsVersionString;
}

size_t wutil_WindowsVersion()
{
	debug_assert(windowsVersion != 0);
	return windowsVersion;
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
// module handle

#ifndef LIB_STATIC_LINK

static HMODULE s_hModule;

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD UNUSED(reason), LPVOID UNUSED(reserved))
{
	DisableThreadLibraryCalls(hInstance);
	s_hModule = hInstance;
	return TRUE;	// success (ignored unless reason == DLL_PROCESS_ATTACH)
}

HMODULE wutil_LibModuleHandle()
{
	return s_hModule;
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
	(void)GetWindowThreadProcessId(hWnd, &pid);	// (function always succeeds)

	if(pid == GetCurrentProcessId())
	{
		hAppWindow = hWnd;
		return FALSE;	// done
	}

	return TRUE;	// keep calling
}


HWND wutil_AppWindow()
{
	if(!hAppWindow)
	{
		// to avoid wasting time, FindAppWindowByPid returns FALSE after
		// finding the desired window, which causes EnumWindows to 'fail'.
		// we detect actual errors by checking GetLastError.
		WinScopedPreserveLastError s;
		SetLastError(0);
		(void)EnumWindows(FindAppWindowByPid, 0);	// (see above)
		debug_assert(GetLastError() == 0);
	}

	return hAppWindow;
}


//-----------------------------------------------------------------------------

static LibError wutil_Init()
{
	InitLocks();

	ForciblyLoadUser32Dll();

	EnableLowFragmentationHeap();

	ReadCommandLine();

	GetDirectories();

	DetectWindowsVersion();

	ImportWow64Functions();
	DetectWow64();

	return INFO::OK;
}


static LibError wutil_Shutdown()
{
	FreeCommandLine();

	FreeUser32Dll();

	ShutdownLocks();

	FreeDirectories();

	return INFO::OK;
}
