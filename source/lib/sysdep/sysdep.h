#ifndef SYSDEP_H__
#define SYSDEP_H__

#ifdef _WIN32
#include "win/win.h"
#endif

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void display_msg(const char* caption, const char* msg);
extern void wdisplay_msg(const wchar_t* caption, const wchar_t* msg);
extern void debug_out(const char* fmt, ...);

extern void check_heap();

#ifdef _MSC_VER
extern double round(double);
#endif

#ifndef HAVE_C99
extern float fminf(float a, float b);
extern float fmaxf(float a, float b);
#endif


#ifdef __cplusplus
}
#endif

#endif	// #ifndef SYSDEP_H__
