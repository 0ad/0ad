/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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
