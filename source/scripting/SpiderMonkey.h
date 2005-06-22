// master header for the SpiderMonkey Javascript library.

// include this instead of accessing any <js*.h> headers directly.
// rationale: these headers require an OS macro to be set (XP_*).
// since this is specific to SpiderMonkey, we don't want to hide the fact in
// some obscure header that's pulled in via PCH (would make reuse harder).
// we take care of it below.

// jstypes.h (included via jsapi.h) requires we define
// "one of XP_BEOS, XP_MAC, XP_OS2, XP_WIN or XP_UNIX".
#include "config.h"
#if defined(OS_WIN)
# define XP_WIN
#elif defined(OS_MAC)
# define XP_MAC
#elif defined(OS_BEOS)
# define XP_BEOS
#else
# define XP_UNIX
#endif

#include <jsapi.h>
#include <jsatom.h>
#ifndef NDEBUG
# include <jsdbgapi.h>
#endif

// include any further required headers here


// Make JS debugging a little easier by automatically naming GC roots
// Don't simply #define NAME_ALL_GC_ROOTS, because jsapi.h is horridly broken
#ifndef NDEBUG
# define JS_AddRoot(cx, rp) JS_AddNamedRoot((cx), (rp), __FILE__ )
#endif
