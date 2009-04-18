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
 * File        : libjpeg.h
 * Project     : 0 A.D.
 * Description : bring in libjpeg header+library, with compatibility fixes
 * =========================================================================
 */

#ifndef INCLUDED_LIBJPEG
#define INCLUDED_LIBJPEG

extern "C" {
// we are not a core library module, so don't define JPEG_INTERNALS
#include <jpeglib.h>
#include <jerror.h>
}

// automatically link against the required library
#if MSC_VERSION
# ifdef NDEBUG
#  pragma comment(lib, "jpeg-6b.lib")
# else
#  pragma comment(lib, "jpeg-6bd.lib")
# endif	// #ifdef NDEBUG
#endif	// #ifdef MSC_VERSION

#endif	// #ifndef INCLUDED_LIBJPEG
