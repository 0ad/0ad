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
 * various system-specific function implementations
 */

#ifndef INCLUDED_SYSDEP
#define INCLUDED_SYSDEP

#include "lib/debug.h"	// ErrorReactionInternal
#include "lib/os_path.h"


//
// output
//

/**
 * display a message.
 *
 * @param caption title message
 * @param msg message contents
 *
 * implemented as a MessageBox on Win32 and printf on Unix.
 * called from debug_DisplayMessage.
 **/
extern void sys_display_msg(const wchar_t* caption, const wchar_t* msg);

/**
 * show the error dialog.
 *
 * @param text to display (practically unlimited length)
 * @param flags: see DebugDisplayErrorFlags.
 * @return ErrorReactionInternal (except ERI_EXIT, which is acted on immediately)
 *
 * called from debug_DisplayError unless overridden by means of
 * ah_display_error.
 **/
extern ErrorReactionInternal sys_display_error(const wchar_t* text, size_t flags);


//
// misc
//

/**
 * @return whether a debugger is attached to the process
 * (if so, it is safe to use debug_break; otherwise, that would
 * raise an exception)
 **/
LIB_API bool sys_IsDebuggerPresent();

/**
 * @return a wide string conversion of the platform's encoding of main's argv.
 *
 * (NB: wseh.cpp defines a wmain that converts argv to UTF-8 and calls main(),
 * but only if LIB_STATIC_LINK)
 **/
LIB_API std::wstring sys_WideFromArgv(const char* argv_i);

/**
 * describe the current OS error state.
 *
 * @param err: if not 0, use that as the error code to translate; otherwise,
 * uses GetLastError or similar.
 * @param buf output buffer
 * @param max_chars
 *
 * rationale: it is expected to be rare that OS return/error codes are
 * actually seen by user code, but we leave the possibility open.
 **/
extern Status sys_StatusDescription(int err, wchar_t* buf, size_t max_chars);

/**
 * determine filename of the module to whom an address belongs.
 *
 * @param addr
 * @param pathname Full path to module (unchanged unless INFO::OK is returned).
 * @return Status
 *
 * note: this is useful for handling exceptions in other modules.
 **/
Status sys_get_module_filename(void* addr, OsPath& pathname);

/**
 * @return full pathname of the current executable.
 *
 * this is useful for determining installation directory, e.g. for VFS.
 **/
LIB_API OsPath sys_ExecutablePathname();

/**
 * Get the current user's login name.
 *
 * @return login name, or empty string on error
 */
extern std::wstring sys_get_user_name();

/**
 * Have the user choose a directory via OS dialog.
 *
 * @param path Path's input value determines the starting directory for
 *		  faster browsing. if INFO::OK is returned, it receives
 *		  chosen directory path.
 **/
extern Status sys_pick_directory(OsPath& path);

/**
 * Open the user's default web browser to the given URL.
 **/
extern Status sys_open_url(const std::string& url);

/**
 * return the largest sector size [bytes] of any storage medium
 * (HD, optical, etc.) in the system.
 *
 * this may be a bit slow to determine (iterates over all drives),
 * but caches the result so subsequent calls are free.
 * (caveat: device changes won't be noticed during this program run)
 *
 * sector size is relevant because Windows aio requires all IO
 * buffers, offsets and lengths to be a multiple of it. this requirement
 * is also carried over into the vfs / file.cpp interfaces for efficiency
 * (avoids the need for copying to/from align buffers).
 *
 * waio uses the sector size to (in some cases) align IOs if
 * they aren't already, but it's also needed by user code when
 * aligning their buffers to meet the requirements.
 *
 * the largest size is used so that we can read from any drive. while this
 * is a bit wasteful (more padding) and requires iterating over all drives,
 * it is the only safe way: this may be called before we know which
 * drives will be needed, and hardlinks may confuse things.
 **/
extern size_t sys_max_sector_size();

/**
 * generate high-quality random bytes.
 *
 * this should only be used with small numbers of bytes, to avoid
 * hogging the system's entropy.
 **/
LIB_API Status sys_generate_random_bytes(u8* buf, size_t count);

/**
 * get the proxy address for accessing the given HTTP URL.
 *
 * this may be very slow (tens of seconds).
 *
 * @return INFO::OK on success; INFO::SKIPPED if no proxy found.
 **/
LIB_API Status sys_get_proxy_config(const std::wstring& url, std::wstring& proxy);

/**
 * open a file like with fopen (but taking an OsPath argument).
 */
LIB_API FILE* sys_OpenFile(const OsPath& pathname, const char* mode);

/**
 * directory separation character
 **/
#if OS_WIN
# define SYS_DIR_SEP '\\'
#else
# define SYS_DIR_SEP '/'
#endif

#endif	// #ifndef INCLUDED_SYSDEP
