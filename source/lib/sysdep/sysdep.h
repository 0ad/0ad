#ifndef SYSDEP_H__
#define SYSDEP_H__

#include "win/win.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void display_msg(const wchar_t* caption, const wchar_t* msg);
extern void debug_out(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef SYSDEP_H__