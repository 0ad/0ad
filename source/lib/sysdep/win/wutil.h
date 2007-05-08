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


//-----------------------------------------------------------------------------
// locking

// critical sections used by win-specific code
enum
{
	ONCE_CS,
	WTIME_CS,
	WAIO_CS,
	WIN_CS,
	WDBG_CS,

	NUM_CS
};

extern void win_lock(uint idx);
extern void win_unlock(uint idx);

// used in a desperate attempt to avoid deadlock in wdbg_exception_handler.
extern int win_is_locked(uint idx);


//-----------------------------------------------------------------------------

extern void* win_alloc(size_t size);
extern void win_free(void* p);


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

#define WIN_SAVE_LAST_ERROR DWORD last_err__ = GetLastError();
#define WIN_RESTORE_LAST_ERROR STMT(if(last_err__ != 0 && GetLastError() == 0) SetLastError(last_err__););


// return the LibError equivalent of GetLastError(), or ERR::FAIL if
// there's no equal.
// you should SetLastError(0) before calling whatever will set ret
// to make sure we do not return any stale errors.
extern LibError LibError_from_win32(DWORD ret, bool warn_if_failed = true);


extern char win_sys_dir[MAX_PATH+1];
extern char win_exe_dir[MAX_PATH+1];

#endif	// #ifndef INCLUDED_WUTIL
