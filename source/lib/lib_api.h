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

#include "lib/sysdep/compiler.h"

// note: EXTERN_C cannot be used because shared_ptr is often returned
// by value, which requires C++ linkage.

#ifdef LIB_STATIC_LINK
# define LIB_API
#else
# if MSC_VERSION
#  ifdef LIB_BUILD
#   define LIB_API __declspec(dllexport)
#  else
#   define LIB_API __declspec(dllimport)
#   ifdef NDEBUG
#    pragma comment(lib, "lib.lib")
#   else
#    pragma comment(lib, "lib_d.lib")
#   endif
#  endif
# elif GCC_VERSION
#  ifdef LIB_BUILD
#   define LIB_API __attribute__ ((visibility("default")))
#  else
#   define LIB_API
#  endif
# else
#  error "Don't know how to define LIB_API for this compiler"
# endif
#endif
