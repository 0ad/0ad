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

A reference-counted immutable string; more efficient than std::wstring
when returning strings from functions. (My i18n test program went ~20% faster,
which is not insignificant).

*/

#ifndef INCLUDED_I18N_STRIMMUTABLE
#define INCLUDED_I18N_STRIMMUTABLE

#include <wchar.h>
#include <string.h>

#include "lib/sysdep/cpu.h"

typedef unsigned short jschar;

namespace I18n
{
	struct strImW_data
	{
		int refs;
		wchar_t* data;
		strImW_data() : refs(1) {}
	};

	class StrImW
	{
	public:

		// I'm lazy (elsewhere), so allow construction from a variety
		// of data types:

		StrImW(const wchar_t* s)
		{
			ref = new strImW_data;
			size_t len = wcslen(s)+1;
			ref->data = new wchar_t[len];
			cpu_memcpy((void*)ref->data, s, len*sizeof(wchar_t));
		}

		StrImW(const char* s)
		{
			ref = new strImW_data;
			size_t len = strlen(s)+1;
			ref->data = new wchar_t[len];
			for (size_t i=0; i<len; ++i)
				ref->data[i] = s[i];
		}

// On non-MSVC, or on MSVC with a native wchar_t type, define jschar separately
#if !MSC_VERSION || defined(_NATIVE_WCHAR_T_DEFINED)
		StrImW(const jschar* s)
		{
			ref = new strImW_data;
			size_t len = 0;
			while (s[len] != '\0') ++len;
			++len; // include the \0
			ref->data = new wchar_t[len];
			for (size_t i=0; i<len; ++i)
				ref->data[i] = s[i];
		}
#endif

		StrImW(const jschar* s, size_t len)
		{
			ref = new strImW_data;
			ref->data = new wchar_t[len+1];
			for (size_t i=0; i<len; ++i)
				ref->data[i] = s[i];
			ref->data[len] = 0;
		}

		~StrImW()
		{
			if (--ref->refs == 0)
			{
				delete[] ref->data;
				delete ref;
			}
		}

		// Copy constructor
		StrImW(const StrImW& s)
		{
			ref = s.ref;
			++ref->refs;
		}

		const wchar_t* str() const
		{
			return ref->data;
		}

		bool operator== (const StrImW& s)
		{
			return s.ref==this->ref || wcscmp(s.ref->data, this->ref->data)==0;
		}

	private:
		strImW_data* ref;
	};
}

#endif // INCLUDED_I18N_STRIMMUTABLE
