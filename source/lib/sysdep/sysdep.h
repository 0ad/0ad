/**
 * =========================================================================
 * File        : sysdep.h
 * Project     : 0 A.D.
 * Description : various system-specific function implementations
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_SYSDEP
#define INCLUDED_SYSDEP

#include "lib/debug.h"	// ErrorReaction

#include <cstdarg>	// needed for sys_vsnprintf


//
// output
//

// raise a message box with the given text or (depending on platform)
// otherwise inform the user.
// called from debug_display_msgw.
extern void sys_display_msg(const char* caption, const char* msg);
extern void sys_display_msgw(const wchar_t* caption, const wchar_t* msg);

// show the error dialog. flags: see DebugDisplayErrorFlags.
// called from debug_display_error.
// can be overridden by means of ah_display_error.
extern ErrorReaction sys_display_error(const wchar_t* text, int flags);


//
// misc
//

// sys_vsnprintf: doesn't quite follow the standard for vsnprintf, but works
// better across compilers:
// - handles positional parameters and %lld
// - always null-terminates the buffer
// - returns -1 on overflow (if the output string (including null) does not fit in the buffer)
extern int sys_vsnprintf(char* buffer, size_t count, const char* format, va_list argptr);


// describe the current OS error state.
//
// err: if not 0, use that as the error code to translate; otherwise,
// uses GetLastError or similar.
// rationale: it is expected to be rare that OS return/error codes are
// actually seen by user code, but we leave the possibility open.
extern LibError sys_error_description_r(int err, char* buf, size_t max_chars);

// determine filename of the module to whom the given address belongs.
// useful for handling exceptions in other modules.
// <path> receives full path to module; it must hold at least MAX_PATH chars.
// on error, it is set to L"".
// return path for convenience.
wchar_t* sys_get_module_filename(void* addr, wchar_t* path);

// store full path to the current executable.
// useful for determining installation directory, e.g. for VFS.
extern LibError sys_get_executable_name(char* n_path, size_t buf_size);

// have the user specify a directory via OS dialog.
// stores its full path in the given buffer, which must hold at least
// PATH_MAX chars.
extern LibError sys_pick_directory(char* n_path, size_t buf_size);


// return the largest sector size [bytes] of any storage medium
// (HD, optical, etc.) in the system.
//
// this may be a bit slow to determine (iterates over all drives),
// but caches the result so subsequent calls are free.
// (caveat: device changes won't be noticed during this program run)
//
// sector size is relevant because Windows aio requires all IO
// buffers, offsets and lengths to be a multiple of it. this requirement
// is also carried over into the vfs / file.cpp interfaces for efficiency
// (avoids the need for copying to/from align buffers).
//
// waio uses the sector size to (in some cases) align IOs if
// they aren't already, but it's also needed by user code when
// aligning their buffers to meet the requirements.
//
// the largest size is used so that we can read from any drive. while this
// is a bit wasteful (more padding) and requires iterating over all drives,
// it is the only safe way: this may be called before we know which
// drives will be needed, and hardlinks may confuse things.
extern size_t sys_max_sector_size();

#if OS_WIN
# define SYS_DIR_SEP '\\'
#else
# define SYS_DIR_SEP '/'
#endif

#endif	// #ifndef INCLUDED_SYSDEP
