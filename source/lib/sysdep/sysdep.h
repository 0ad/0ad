#ifndef SYSDEP_H__
#define SYSDEP_H__

#include "config.h"

#include "sysdep/debug.h"

#ifdef _WIN32
#include "win/win.h"
#include "win/wdbg.h"
#elif defined(OS_UNIX)
#include "unix/unix.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


extern void display_msg(const char* caption, const char* msg);
extern void wdisplay_msg(const wchar_t* caption, const wchar_t* msg);


extern int clipboard_set(const wchar_t* text);
extern wchar_t* clipboard_get();
extern int clipboard_free(wchar_t* copy);


extern int get_executable_name(char* n_path, size_t buf_size);


#ifdef _MSC_VER
extern double round(double);
#endif

#ifndef HAVE_C99
extern float fminf(float a, float b);
extern float fmaxf(float a, float b);
#endif

#ifndef _MSC_VER
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#ifdef __cplusplus
}
#endif

#endif	// #ifndef SYSDEP_H__
