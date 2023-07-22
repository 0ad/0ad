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

/*
 * various Windows-specific utilities
 */

#ifndef INCLUDED_WUTIL
#define INCLUDED_WUTIL

#ifdef _MSC_VER
# pragma warning(disable:4091) // hides previous local declaration
#endif

#include "lib/os_path.h"
#include "lib/sysdep/os/win/win.h"

template<typename H>
bool wutil_IsValidHandle(H h)
{
	return h != 0 && h != INVALID_HANDLE_VALUE;
}


//-----------------------------------------------------------------------------
// dynamic linking

// define a function pointer (optionally prepend 'static')
#define WUTIL_FUNC(varName, ret, params)\
	ret (WINAPI* varName) params

// rationale:
// - splitting up WUTIL_FUNC and WUTIL_IMPORT is a bit verbose in
//   the common case of a local function pointer definition,
//   but allows one-time initialization of static variables.
// - differentiating between procName and varName allows searching
//   for the actual definition of the function pointer in the code.
// - a cast would require passing in ret/params.
// - writing a type-punned pointer breaks strict-aliasing rules.
#define WUTIL_IMPORT(hModule, procName, varName)\
	STMT(\
		const FARPROC f = GetProcAddress(hModule, #procName);\
		memcpy(&varName, &f, sizeof(FARPROC));\
	)

// note: Kernel32 is guaranteed to be loaded, so we don't
// need to LoadLibrary and FreeLibrary.
#define WUTIL_IMPORT_KERNEL32(procName, varName)\
	WUTIL_IMPORT(GetModuleHandleW(L"kernel32.dll"), procName, varName)

// note: ntdll is guaranteed to be loaded, so we don't
// need to LoadLibrary and FreeLibrary.
#define WUTIL_IMPORT_NTDLL(procName, varName)\
	WUTIL_IMPORT(GetModuleHandleW(L"ntdll.dll"), procName, varName)

//-----------------------------------------------------------------------------
// safe allocator

extern void* wutil_Allocate(size_t size);
extern void wutil_Free(void* p);


//-----------------------------------------------------------------------------
// locks

// critical sections used by win-specific code
enum WinLockId
{
	WDBG_SYM_CS,	// protects (non-reentrant) dbghelp.dll
	WDIR_WATCH_CS,

	NUM_CS
};

extern void wutil_Lock(WinLockId id);
extern void wutil_Unlock(WinLockId id);

// used in a desperate attempt to avoid deadlock in wseh.
extern bool wutil_IsLocked(WinLockId id);

class WinScopedLock
{
public:
	WinScopedLock(WinLockId id)
		: m_id(id)
	{
		wutil_Lock(m_id);
	}

	~WinScopedLock()
	{
		wutil_Unlock(m_id);
	}

private:
	WinLockId m_id;
};


//-----------------------------------------------------------------------------
// errors

/**
 * some WinAPI functions SetLastError(0) on success, which is bad because
 * it can hide previous errors. this class takes care of restoring the
 * previous value.
 **/
class WinScopedPreserveLastError
{
public:
	WinScopedPreserveLastError()
		: m_lastError(GetLastError())
	{
		SetLastError(0);
	}

	~WinScopedPreserveLastError()
	{
		if(m_lastError != 0 && GetLastError() == 0)
			SetLastError(m_lastError);
	}

private:
	DWORD m_lastError;
};


/**
 * @return the Status equivalent of GetLastError(), or ERR::FAIL if
 * there's no equivalent.
 * SetLastError(0) should be called before the Windows function to
 * make sure no stale errors are returned.
 **/
extern Status StatusFromWin();


//-----------------------------------------------------------------------------
// directories

extern OsPath wutil_LocalAppdataPath();
extern OsPath wutil_RoamingAppdataPath();
extern OsPath wutil_PersonalPath();


//-----------------------------------------------------------------------------

Status wutil_SetPrivilege(const wchar_t* privilege, bool enable);

/**
 * @return module handle of lib code (that of the main EXE if
 * linked statically, otherwise the DLL).
 * this is necessary for the error dialog.
 **/
extern HMODULE wutil_LibModuleHandle();


/**
 * @return handle to the first window owned by the current process, or
 * 0 if none exist (e.g. it hasn't yet created one).
 *
 * enumerates all top-level windows and stops if PID matches.
 * once this function returns a non-NULL handle, it will always
 * return that cached value.
 **/
extern HWND wutil_AppWindow();

extern void wutil_SetAppWindow(void* hwnd);

extern void wutil_EnableHiDPIOnWindows();

#endif	// #ifndef INCLUDED_WUTIL
