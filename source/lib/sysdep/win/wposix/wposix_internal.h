#ifndef INCLUDED_WPOSIX_INTERNAL
#define INCLUDED_WPOSIX_INTERNAL

#include "lib/lib.h"
#include "../win_internal.h"

#include "werrno.h"


// we define some CRT functions (e.g. access), because they're otherwise
// only brought in by win-specific headers (here, <io.h>).
// define correctly for static or DLL CRT in case the original header
// is included, to avoid conflict warnings.
//
// note: try to avoid redefining CRT functions - if building against the
// DLL CRT, the declarations will be incompatible. adding _CRTIMP to the decl
// is a last resort (e.g. if the regular CRT headers would conflict).
#ifndef _CRTIMP
# ifdef  _DLL
#  define _CRTIMP __declspec(dllimport)
# else
#  define _CRTIMP
# endif 
#endif


// cast intptr_t to HANDLE; centralized for easier changing, e.g. avoiding
// warnings. i = -1 converts to INVALID_HANDLE_VALUE (same value).
inline HANDLE HANDLE_from_intptr(intptr_t i)
{
	return (HANDLE)((char*)0 + i);
}

#endif	// #ifndef INCLUDED_WPOSIX_INTERNAL
