#ifndef INCLUDED_WPOSIX_INTERNAL
#define INCLUDED_WPOSIX_INTERNAL

#include "lib/sysdep/win/win.h"
#include "lib/sysdep/win/winit.h"
#include "lib/sysdep/win/wutil.h"

// cast intptr_t to HANDLE; centralized for easier changing, e.g. avoiding
// warnings. i = -1 converts to INVALID_HANDLE_VALUE (same value).
inline HANDLE HANDLE_from_intptr(intptr_t i)
{
	return (HANDLE)((char*)0 + i);
}

#endif	// #ifndef INCLUDED_WPOSIX_INTERNAL
