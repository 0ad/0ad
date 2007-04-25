/*

A reference-counted immutable string; more efficient than std::wstring
when returning strings from functions. (My i18n test program went ~20% faster,
which is not insignificant).

*/

#ifndef I18N_STRIMMUTABLE_H
#define I18N_STRIMMUTABLE_H

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

#endif // I18N_STRIMMUTABLE_H
