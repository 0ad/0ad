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

/*
 * Win32 debug support code.
 */

#ifndef INCLUDED_WDBG
#define INCLUDED_WDBG

/**
 * same as debug_printf except that some type conversions aren't supported
 * (in particular, no floating point).
 *
 * this function does not allocate memory from the CRT heap, which makes it
 * safe to use from an allocation hook.
 **/
LIB_API void wdbg_printf(const wchar_t* fmt, ...);

/**
 * similar to debug_assert but safe to use during critical init or
 * while under the heap or dbghelp locks.
 **/
#define wdbg_assert(expr) STMT(if(!(expr)) debug_break();)

#endif	// #ifndef INCLUDED_WDBG
