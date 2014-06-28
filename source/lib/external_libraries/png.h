/* Copyright (c) 2014 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * bring in LibPNG header+library, with compatibility fixes
 */

#ifndef INCLUDED_PNG
#define INCLUDED_PNG

// <png.h> includes <zlib.h>, which requires some fixes by our header.
#include "lib/external_libraries/zlib.h"

#include <png.h>

// automatically link against the required library
#if MSC_VERSION
# ifdef NDEBUG
#  pragma comment(lib, "libpng16.lib")
# else
#  pragma comment(lib, "libpng16d.lib")
# endif	// NDEBUG
#endif	// MSC_VERSION

#endif	//	#ifndef INCLUDED_PNG
