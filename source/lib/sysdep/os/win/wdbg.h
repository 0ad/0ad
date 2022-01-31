/* Copyright (C) 2022 Wildfire Games.
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
 * Win32 debug support code.
 */

#ifndef INCLUDED_WDBG
#define INCLUDED_WDBG

/**
 * same as debug_printf except that some type conversions aren't supported
 * (in particular, no floating point) and output is limited to 1024+1 characters.
 *
 * this function does not allocate memory from the CRT heap, which makes it
 * safe to use from an allocation hook.
 **/
void wdbg_printf(const wchar_t* fmt, ...);

/**
 * similar to ENSURE but safe to use during critical init or
 * while under the heap or dbghelp locks.
 **/
#define wdbg_assert(expr) STMT(if(!(expr)) debug_break();)

#endif	// #ifndef INCLUDED_WDBG
