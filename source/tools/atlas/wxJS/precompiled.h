#ifndef INCLUDED_STDAFX
#define INCLUDED_STDAFX

// Global definitions, since this is a convenient place:

#ifdef _WIN32
# define XP_WIN
#else
# define XP_UNIX
#endif // (we don't support XP_OS2 or XP_BEOS)

#define JS_THREADSAFE


#ifdef _WIN32
# pragma warning (disable: 4100) // "unreferenced formal parameter"
# pragma warning (disable: 4267 4311 4312) // conversions between ints/pointers/etc
#endif

// Precompiled headers:

#ifdef USING_PCH

#define WX_PRECOMP
#include "common/main.h"
#include "wx/wxprec.h"

#endif

#endif // INCLUDED_STDAFX
