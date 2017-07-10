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

#include "precompiled.h"
#include "lib/posix/posix.h"


#if EMULATE_WCSDUP
wchar_t* wcsdup(const wchar_t* str)
{
	const size_t num_chars = wcslen(str);
	wchar_t* dst = (wchar_t*)malloc((num_chars+1)*sizeof(wchar_t));	// note: wcsdup is required to use malloc
	if(!dst)
		return 0;
	wcscpy_s(dst, num_chars+1, str);
	return dst;
}
#endif

#if EMULATE_WCSCASECMP
int wcscasecmp (const wchar_t* s1, const wchar_t* s2)
{
	wint_t a1, a2;

	if (s1 == s2)
		return 0;

	do
	{
		a1 = towlower(*s1++);
		a2 = towlower(*s2++);
		if (a1 == L'\0')
			break;
	}
	while (a1 == a2);

	return a1 - a2;
}
#endif
