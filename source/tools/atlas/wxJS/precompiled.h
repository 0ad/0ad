#ifndef INCLUDED_STDAFX
#define INCLUDED_STDAFX

// Global definitions, since this is a convenient place:

#ifdef _WIN32
# define XP_WIN
# define WIN32 // SpiderMonkey expects this

// The jsval struct type causes crashes due to weird miscompilation
// issues in (at least) VC2008, so force it to be the less-type-safe
// non-struct type instead
# define JS_NO_JSVAL_JSID_STRUCT_TYPES

#else
# define XP_UNIX
#endif // (we don't support XP_OS2 or XP_BEOS)


#ifdef _WIN32
# pragma warning (disable: 4100) // "unreferenced formal parameter"
# pragma warning (disable: 4267 4311 4312) // conversions between ints/pointers/etc
#endif

// Precompiled headers:

#ifndef _WIN32
// GCC 4.4+ CONFIG=Release triggers linker errors on wxKeyEvent::Clone() in treeevt.h
// with hidden visibility, so switch to default visibility when first including the
// relevant wx headers
// (TODO: might need to do something extra for the non-USING_PCH case)
#pragma GCC visibility push(default)
#endif

#ifdef USING_PCH
# define WX_PRECOMP
# include "common/main.h"
# include "wx/wxprec.h"
#endif

#ifndef _WIN32
#pragma GCC visibility pop
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
