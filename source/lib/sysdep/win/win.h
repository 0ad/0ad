#if !defined(__WIN_H__) && defined(_WIN32)
#define __WIN_H__

#include <wchar.h>

// C99
#define snprintf _snprintf
#define swprintf _snwprintf
#define vsnprintf _vsnprintf
#define vsnwprintf _vsnwprintf

#include <stddef.h>	// wchar_t

// libpng.h -> zlib.h -> zconf.h includes <windows.h>, which causes conflicts.
// inhibit this, and define what they actually need from windows.h
// incidentally, this requires all windows dependencies to include
// sysdep/win_internal.h
#define _WINDOWS_	// windows.h include guard
#define WINAPI __stdcall
#define WINAPIV __cdecl


extern "C" void win_debug_break(void);

#endif	// #ifndef __WIN_H__
