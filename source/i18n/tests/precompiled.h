#ifndef NDEBUG

#include <vector>
#include <hash_map>
#include <string>
#include <sstream>
#include <map>

/*
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#define   malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   calloc(c, s)      _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _expand(p, s)     _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   free(p)           _free_dbg(p, _NORMAL_BLOCK)
*/
#endif

// Random stuff:

#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321

#define SDL_BYTE_ORDER SDL_LIL_ENDIAN

#include <cassert>
#define cassert(x) extern char cassert__##__LINE__ [x]
#define debug_warn(x) assert(0&&x)
#define debug_assert assert

#define XP_WIN

#include "ps/Errors.h"

#include <cstdio>
#define swprintf _snwprintf
