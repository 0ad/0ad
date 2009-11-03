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
* File        : regex.cpp
* Project     : 0 A.D.
* Description : minimal regex implementation
* =========================================================================
*/

#include "precompiled.h"


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
