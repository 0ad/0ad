/**
 * =========================================================================
 * File        : wutil.h
 * Project     : 0 A.D.
 * Description : various Windows-specific utilities
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WUTIL
#define INCLUDED_WUTIL

#if !OS_WIN
#error "wutil.h: do not include if not compiling for Windows"
#endif

#include "win.h"


//
// safe allocator
//

extern void* win_alloc(size_t size);
extern void win_free(void* p);


//
// locks
//

// critical sections used by win-specific code
enum WinLockId
{
	WAIO_CS,
	WDBG_SYM_CS,	// protects (non-reentrant) dbghelp.dll

	NUM_CS
};

extern void win_lock(WinLockId id);
extern void win_unlock(WinLockId id);

// used in a desperate attempt to avoid deadlock in wseh.
extern bool win_is_locked(WinLockId id);

class WinScopedLock
{
public:
	WinScopedLock(WinLockId id)
		: m_id(id)
	{
		win_lock(m_id);
	}

	~WinScopedLock()
	{
		win_unlock(m_id);
	}

private:
	WinLockId m_id;
};


//
// errors
//

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
 * @return the LibError equivalent of GetLastError(), or ERR::FAIL if
 * there's no equivalent.
 * you should SetLastError(0) before calling whatever will set ret
 * to make sure we do not report any stale errors.
 *
 * @param warn_if_failed if set, raises an error dialog that reports
 * the LibError.
 **/
LibError LibError_from_GLE(bool warn_if_failed = true);

#define WARN_WIN32_ERR (void)LibError_from_GLE(true)

/// if ret is false, returns LibError_from_GLE.
extern LibError LibError_from_win32(DWORD ret, bool warn_if_failed = true);


//
// command line
//

extern int wutil_argc;
extern char** wutil_argv;

extern bool wutil_HasCommandLineArgument(const char* arg);


//
// directories
//

// neither of these end in a slash.
extern char win_sys_dir[MAX_PATH+1];
extern char win_exe_dir[MAX_PATH+1];


//
// version
//

extern const char* wutil_WindowsVersionString();

// (same format as WINVER)
const size_t WUTIL_VERSION_2K    = 0x0500;
const size_t WUTIL_VERSION_XP    = 0x0501;
const size_t WUTIL_VERSION_XP64  = 0x0502;
const size_t WUTIL_VERSION_VISTA = 0x0600;

/**
 * @return short textual representation of the appropriate WUTIL_VERSION
 **/
extern const char* wutil_WindowsFamily();

extern size_t wutil_WindowsVersion();


//
// Wow64
//

extern bool wutil_IsWow64();

class WinScopedDisableWow64Redirection
{
public:
	WinScopedDisableWow64Redirection();
	~WinScopedDisableWow64Redirection();

private:
	void* m_wasRedirectionEnabled;
};


/**
 * module handle of lib code (that of the main EXE if linked statically,
 * otherwise the DLL).
 * this is necessary for the error dialog.
 **/
extern HMODULE wutil_LibModuleHandle;


/**
 * @return handle to the first window owned by the current process, or
 * 0 if none exist (e.g. it hasn't yet created one).
 *
 * enumerates all top-level windows and stops if PID matches.
 * once this function returns a non-NULL handle, it will always
 * return that cached value.
 **/
extern HWND wutil_AppWindow();

#endif	// #ifndef INCLUDED_WUTIL
