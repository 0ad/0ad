// if precompiled headers are supported, include all headers we'd ever need
// that don't often change. if not supported, include nothing (would actually
// slow down the build, since unnecessary headers would be included).
// hence, all files include precompiled.h and then all the headers they'd
// normally lead => best build performance with or without PCH.

#include "config.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)	// function is deprecated
#pragma warning(disable:4786)	// identifier truncated to 255 chars
#endif

#ifdef HAVE_PCH

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <math.h>
#include <assert.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>
#include <wchar.h>

#include <list>
#include <map>
#include <vector>
#include <stack>
#include <string>
#include <set>
#include <deque>
#include <fstream>

#include <functional>
#include <algorithm>
#include <numeric>

// Nicer memory leak reporting in MSVC:
// (You've got to include all STL headers first to avoid lots of errors,
// so make sure they're in the list above and you compile with PCH)
#ifdef HAVE_DEBUGALLOC
# include <crtdbg.h>
# include <malloc.h>
// Can't define _CRTDBG_MAP_ALLOC because it has a broken 'new',
// so manually redefine the appropriate functions
# define   new               new(_NORMAL_BLOCK, __FILE__, __LINE__)
# define   malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
# define   calloc(c, s)      _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
# define   realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
# define   free(p)           _free_dbg(p, _NORMAL_BLOCK)
#endif // HAVE_DEBUGALLOC

#endif // #ifdef HAVE_PCH
