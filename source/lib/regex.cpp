/**
* =========================================================================
* File        : regex.cpp
* Project     : 0 A.D.
* Description : minimal regex implementation
* =========================================================================
*/

// license: GPL; see lib/license.txt

#include "precompiled.h"


int match_wildcard(const char* s, const char* w)
{
	if(!w)
		return 1;

	// saved position in both strings, used to expand '*':
	// s2 is advanced until match.
	// initially 0 - we abort on mismatch before the first '*'.
	const char* s2 = 0;
	const char* w2 = 0;

	while(*s)
	{
		const int wc = *w;
		if(wc == '*')
		{
			// wildcard string ended with * => match.
			if(*++w == '\0')
				return 1;

			w2 = w;
			s2 = s+1;
		}
		// match one character
		else if(toupper(wc) == toupper(*s) || wc == '?')
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

int match_wildcardw(const wchar_t* s, const wchar_t* w)
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
