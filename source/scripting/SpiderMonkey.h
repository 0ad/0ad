// Master header for the SpiderMonkey Javascript library.
//
// Include this instead of accessing any <js*.h> headers directly.
// Rationale: they require an OS macro to be set (XP_*).
// Since this is specific to SpiderMonkey, we don't want to saddle
// another obscure header with it (would make reuse harder).
// Instead, do it here where the purpose is clear.
//
// This doesn't go in ScriptingHost.h because some other files
// (notably precompiled.h and i18n/ScriptInterface.cpp) want only these
// definitions without pulling in the whole of ScriptingHost.

// jstypes.h (included via jsapi.h) requires we define
// "one of XP_BEOS, XP_MAC, XP_OS2, XP_WIN or XP_UNIX".
#include "lib/config.h"
#if OS_WIN
# define XP_WIN
#elif OS_MAC
# define XP_MAC
#elif OS_BEOS
# define XP_BEOS
#else
# define XP_UNIX
#endif

#include <js/jsapi.h>
#include <js/jsatom.h>
#ifndef NDEBUG
# include <js/jsdbgapi.h>
#endif

// include any further required headers here


// Make JS debugging a little easier by automatically naming GC roots
// Don't simply #define NAME_ALL_GC_ROOTS, because jsapi.h is horridly broken
#ifndef NDEBUG
# define JS_AddRoot(cx, rp) JS_AddNamedRoot((cx), (rp), __FILE__ )
#endif
