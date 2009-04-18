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

#include "precompiled.h"

#include <cstdio>
#include <cstdarg>

// See declaration in sysdep.h for explanation of need

int sys_vsnprintf(char* buffer, size_t count, const char* format, va_list argptr)
{
	int ret = vsnprintf(buffer, count, format, argptr);

	/*
	"The glibc implementation of the functions snprintf() and vsnprintf() conforms
	to the C99 standard ... since glibc version 2.1. Until glibc 2.0.6 they would
	return -1 when the output was truncated."
	- man printf

	MSVC's _vsnprintf still returns -1, so we want this one to do the same (for
	compatibility), if the output (including the terminating null) is truncated.
	*/

	if (ret >= (int)count)
		return -1;

	return ret;
}
