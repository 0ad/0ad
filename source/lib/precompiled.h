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

#include <functional>
#include <algorithm>
#include <numeric>

#endif



