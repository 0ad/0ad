#ifndef INCLUDED_STDAFX
#define INCLUDED_STDAFX

// Global definitions, since this is a convenient place:

#ifdef _WIN32
# define XP_WIN
#else
# define XP_UNIX
#endif // (we don't support XP_OS2 or XP_BEOS)


#ifdef _WIN32
# pragma warning (disable: 4100) // "unreferenced formal parameter"
# pragma warning (disable: 4267 4311 4312) // conversions between ints/pointers/etc
#endif

// Precompiled headers:

#ifdef USING_PCH
# define WX_PRECOMP
# include "common/main.h"
# include "wx/wxprec.h"
#endif

/*
// DISABLED: because normally this DLL gets unloaded before the leak reports, which means the
// __FILE__ strings are no longer in valid memory, which breaks everything.

#if defined(_MSC_VER) && !defined(NDEBUG)
// Include some STL headers that don't like 'new' being redefined
# include <map>
// then redefine 'new'
# define _CRTDBG_MAP_ALLOC
# include <crtdbg.h>
# define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
*/

#endif // INCLUDED_STDAFX
