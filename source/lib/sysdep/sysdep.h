/**
 * =========================================================================
 * File        : sysdep.h
 * Project     : 0 A.D.
 * Description : various system-specific function implementations
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2007 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef INCLUDED_SYSDEP
#define INCLUDED_SYSDEP

#include "lib/config.h"
#include "lib/debug.h"	// ErrorReaction

#include <cstdarg>	// needed for vsnprintf2


//-----------------------------------------------------------------------------
// C99 / SUSv3 emulation where needed
//-----------------------------------------------------------------------------

// vsnprintf2: doesn't quite follow the standard for vsnprintf, but works
// better across compilers:
// - handles positional parameters and %lld
// - always null-terminates the buffer
// - returns -1 on overflow (if the output string (including null) does not fit in the buffer)
extern int vsnprintf2(char* buffer, size_t count, const char* format, va_list argptr);

#if !MSC_VERSION
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif



//-----------------------------------------------------------------------------
// sysdep API
//-----------------------------------------------------------------------------

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
extern ErrorReaction sys_display_error(const wchar_t* text, uint flags);


//
// clipboard
//

// "copy" text into the clipboard. replaces previous contents.
extern LibError sys_clipboard_set(const wchar_t* text);

// allow "pasting" from clipboard. returns the current contents if they
// can be represented as text, otherwise 0.
// when it is no longer needed, the returned pointer must be freed via
// sys_clipboard_free. (NB: not necessary if zero, but doesn't hurt)
extern wchar_t* sys_clipboard_get(void);

// frees memory used by <copy>, which must have been returned by
// sys_clipboard_get. see note above.
extern LibError sys_clipboard_free(wchar_t* copy);


//
// mouse cursor
//

// note: these do not warn on error; that is left to the caller.

// creates a cursor from the given image.
// w, h specify image dimensions [pixels]. limit is implementation-
//   dependent; 32x32 is typical and safe.
// bgra_img is the cursor image (BGRA format, bottom-up).
//   it is no longer needed and can be freed after this call returns.
// hotspot (hx,hy) is the offset from its upper-left corner to the
//   position where mouse clicks are registered.
// cursor is only valid when INFO::OK is returned; in that case, it must be
//   sys_cursor_free-ed when no longer needed.
extern LibError sys_cursor_create(uint w, uint h, void* bgra_img,
	uint hx, uint hy, void** cursor);

// create a fully transparent cursor (i.e. one that when passed to set hides
// the system cursor)
extern LibError sys_cursor_create_empty(void **cursor);

// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
extern LibError sys_cursor_set(void* cursor);

// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
extern LibError sys_cursor_free(void* cursor);


//
// misc
//

/**
 * allocate on stack, automatically free when current function returns.
 **/
extern void* sys_alloca(size_t size);

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
