// remove all of mmgr.h's memory allocation "hooks" -
// but only if we actually defined them!
#if CONFIG_USE_MMGR || HAVE_VC_DEBUG_ALLOC
# undef new
# undef delete
# undef malloc
# undef calloc
# undef realloc
# undef free
# undef strdup
# undef wcsdup
# undef getcwd
#endif

// sanity check
#ifdef new
#error "nommgr.h - something is wrong, new is still defined"
#endif
