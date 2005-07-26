#define XP_WIN

#include <jsapi.h>
#include <jsatom.h>
#ifndef NDEBUG
# include <jsdbgapi.h>
#endif

// Make JS debugging a little easier by automatically naming GC roots
// Don't simply #define NAME_ALL_GC_ROOTS, because jsapi.h is horridly broken
#ifndef NDEBUG
# define JS_AddRoot(cx, rp) JS_AddNamedRoot((cx), (rp), __FILE__ )
#endif
