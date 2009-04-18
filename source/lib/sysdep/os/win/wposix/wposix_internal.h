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

#ifndef INCLUDED_WPOSIX_INTERNAL
#define INCLUDED_WPOSIX_INTERNAL

#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/winit.h"
#include "lib/sysdep/os/win/wutil.h"

// cast intptr_t to HANDLE; centralized for easier changing, e.g. avoiding
// warnings. i = -1 converts to INVALID_HANDLE_VALUE (same value).
inline HANDLE HANDLE_from_intptr(intptr_t i)
{
	return (HANDLE)((char*)0 + i);
}

#endif	// #ifndef INCLUDED_WPOSIX_INTERNAL
