/* Copyright (C) 2020 Wildfire Games.
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

// minimal regex implementation

#include "precompiled.h"

#include <cwctype>

int match_wildcard(const wchar_t* s, const wchar_t* w)
{
	if(!w)
		return 1;

	// saved position in both strings, used to expand '*':
	// s2 is advanced until match.
	// initially 0 - we abort on mismatch before the first '*'.
	const wchar_t* s2 = 0;
	const wchar_t* w2 = 0;

	while(*s)
	{
		const wchar_t wc = *w;
		if(wc == '*')
		{
			// wildcard string ended with * => match.
			if(*++w == '\0')
				return 1;

			w2 = w;
			s2 = s+1;
		}
		// match one character
		else if(towupper(wc) == towupper(*s) || wc == '?')
		{
			w++;
			s++;
		}
		// mismatched character
		else
		{
			// no '*' found yet => mismatch.
			if(!s2)
				return 0;

			// resume at previous position+1
			w = w2;
			s = s2++;
		}
	}

	// strip trailing * in wildcard string
	while(*w == '*')
		w++;

	return (*w == '\0');
}
