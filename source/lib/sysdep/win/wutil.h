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
enum
{
	ONCE_CS,
	WAIO_CS,
	WDBG_CS,
	WDBG_SYM_CS,

	NUM_CS
};

extern void win_lock(uint idx);
extern void win_unlock(uint idx);

// used in a desperate attempt to avoid deadlock in wdbg_exception_handler.
extern int win_is_locked(uint idx);

// thread safe, usable in constructors
#define WIN_ONCE(code)\
{\
	win_lock(ONCE_CS);\
	static bool ONCE_init_;	/* avoid name conflict */\
	if(!ONCE_init_)\
	{\
		ONCE_init_ = true;\
		code;\
	}\
	win_unlock(ONCE_CS);\
}


//
// error codes
//

#define WIN_SAVE_LAST_ERROR DWORD last_err__ = GetLastError();
#define WIN_RESTORE_LAST_ERROR STMT(if(last_err__ != 0 && GetLastError() == 0) SetLastError(last_err__););

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
const uint WUTIL_VERSION_2K    = 0x0500;
const uint WUTIL_VERSION_XP    = 0x0501;
const uint WUTIL_VERSION_XP64  = 0x0502;
const uint WUTIL_VERSION_VISTA = 0x0600;

/**
 * @return short textual representation of the appropriate WUTIL_VERSION
 **/
extern const char* wutil_WindowsFamily();

extern uint wutil_WindowsVersion();


//
// Wow64
//

extern bool wutil_IsWow64();
extern void wutil_DisableWow64Redirection(void*& wasRedirectionEnabled);
extern void wutil_RevertWow64Redirection(void* wasRedirectionEnabled);


#endif	// #ifndef INCLUDED_WUTIL
