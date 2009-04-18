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
	A basic_string derivative that works with uint16_t as its underlying char
	type.
*/
#ifndef INCLUDED_UTF16STRING
#define INCLUDED_UTF16STRING

// On Windows, wchar_t is typedef'ed to unsigned short, which conflicts
// with uint16_t (which is also an unsigned short), so just use std::wstring
#ifdef _MSC_VER

typedef wchar_t utf16_t;
typedef std::wstring utf16string;
typedef std::wstringstream utf16stringstream;

// On Linux, wchar_t is 32-bit, so define a new version of it
#else

#include <string>
#include "types.h"

typedef uint16_t utf16_t;
typedef std::basic_string<utf16_t> utf16string;
typedef std::basic_stringstream<utf16_t> utf16stringstream;

namespace std {
	template<>
	struct char_traits<utf16_t>
	{
		typedef utf16_t		char_type;
		typedef int			int_type;
		typedef streampos	pos_type;
		typedef streamoff	off_type;
		typedef mbstate_t	state_type;

		static void assign(char_type& c1, const char_type& c2)
		{ c1 = c2; }

		static bool eq(const char_type& c1, const char_type& c2)
		{ return c1 == c2; }

		static bool lt(const char_type& c1, const char_type& c2)
		{ return c1 < c2; }

		static int compare(const char_type* s1, const char_type* s2, size_t n)
		{
			return memcmp(s1, s2, n*sizeof(char_type));
		}

		static size_t length(const char_type* s)
		{
			const char_type* end=s;
			while (*end) end++;
			return end-s;
		}

		static const char_type* find(const char_type* s, size_t n, const char_type& a)
		{
			size_t i;
			for (i=0;i<n;i++)
			{
				if (s[i] == a) return s+i;
			}
			return NULL;
		}

		static char_type* move(char_type* s1, const char_type* s2, size_t n)
		{
			return (char_type *)memmove(s1, s2, n*sizeof(char_type));
		}

		static char_type* copy(char_type* s1, const char_type* s2, size_t n)
		{
			return (char_type *)memcpy(s1, s2, n*sizeof(char_type));
		}

		static char_type* assign(char_type* s, size_t n, char_type a)
		{
			while (n--)
			{
				s[n]=a;
			}
			return s;
		}

		static char_type to_char_type(const int_type& c)
		{ return (char_type)c; }

		static int_type to_int_type(const char_type& c)
		{ return (int_type)c; }

		static bool eq_int_type(const int_type& c1, const int_type& c2)
		{ return c1 == c2; }

		static int_type eof()
		{ return -1; }

		static int_type not_eof(const int_type& c)
		{ return (c == -1) ? 0 : c; }
	};
}

#endif // #ifdef _MSC_VER / #else

#endif
