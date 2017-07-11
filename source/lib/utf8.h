/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_UTF8
#define INCLUDED_UTF8

// note: error codes are returned via optional output parameter.
namespace ERR
{
	const Status UTF8_SURROGATE     = -100700;
	const Status UTF8_OUTSIDE_BMP   = -100701;
	const Status UTF8_NONCHARACTER  = -100702;
	const Status UTF8_INVALID_UTF8  = -100703;
}

/**
 * convert UTF-8 to a wide string (UTF-16 or UCS-4, depending on the
 * platform's wchar_t).
 *
 * @param s input string (UTF-8)
 * @param err if nonzero, this receives the first error encountered
 * (the rest may be subsequent faults) or INFO::OK if all went well.
 * otherwise, the function raises a warning dialog for every
 * error/warning.
 **/
LIB_API std::wstring wstring_from_utf8(const std::string& s, Status* err = 0);

/**
 * opposite of wstring_from_utf8
 **/
LIB_API std::string utf8_from_wstring(const std::wstring& s, Status* err = 0);

#endif	// #ifndef INCLUDED_UTF8
