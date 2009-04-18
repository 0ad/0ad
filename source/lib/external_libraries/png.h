/**
 * =========================================================================
 * File        : png.h
 * Project     : 0 A.D.
 * Description : bring in LibPNG header+library, with compatibility fixes
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_PNG
#define INCLUDED_PNG

// <png.h> includes <zlib.h>, which requires some fixes by our header.
#include "lib/external_libraries/zlib.h"

#include <png.h>

// automatically link against the required library
#if MSC_VERSION
# ifdef NDEBUG
#  pragma comment(lib, "libpng13.lib")
# else
#  pragma comment(lib, "libpng13d.lib")
# endif	// NDEBUG
#endif	// MSC_VERSION

#endif	//	#ifndef INCLUDED_PNG
