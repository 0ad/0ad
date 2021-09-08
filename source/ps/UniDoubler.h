/* Copyright (C) 2021 Wildfire Games.
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

// Make sure we have the argument (UNIDOUBLER_HEADER), and that we're not
// called from within another unidoubler execution (now that's just asking for
// trouble)
#if defined(UNIDOUBLER_HEADER) && !defined(IN_UNIDOUBLER)

#define IN_UNIDOUBLER

// When compiling CStr.cpp with PCH, the unidoubler stuff gets rather
// confusing because of all the nested inclusions, but this makes it work:
#undef CStr

// First, set up the environment for the Unicode version
#ifndef _UNICODE
#define _UNICODE
#endif
#define CStr CStrW
#define tstring wstring

// Include the unidoubled file
#include UNIDOUBLER_HEADER

// Clean up all the macros
#undef _UNICODE
#undef CStr
#undef tstring

// Now include the 8-bit version under the name CStr8
#define CStr CStr8
#define tstring string

#include UNIDOUBLER_HEADER

// Clean up the macros again, to minimise namespace pollution
#undef CStr
#undef tstring

// To please the file that originally include CStr.h, make CStr an alias for CStr8:
#define CStr CStr8

#undef IN_UNIDOUBLER
#undef UNIDOUBLER_HEADER

#endif
