// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef __STDAFX_H__
#define __STDAFX_H__


#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>
#include <hash_set>
#include <hash_map>

#include <boost/random.hpp>

#ifdef WIN32
#define XP_WIN
#else
#define XP_UNIX		// TODO: Someone should actually test this on Linux
#endif
#include "jsapi.h"

const float PI = acos(-1.0f);

#endif