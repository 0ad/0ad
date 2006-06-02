/*

Returned by Translate(), and used to collect variables from the user
before converting into a string.

*/

#ifndef I18N_STRINGBUF_H
#define I18N_STRINGBUF_H

#include "Common.h"
#include "TranslatedString.h"
#include "ps/CStr.h"

namespace I18n
{
	class CLocale;
	class BufferVariable;

	template<typename T> BufferVariable* NewBufferVariable(T v);

	class StringBuffer
	{
		friend class CLocale;

	public:
		// Builds and returns the finished string
		operator Str();
		operator CStrW() { return CStrW( (Str)*this ); }

		// Stores the variable inside the StringBuffer,
		// for later conversion into a string
		template<typename T> StringBuffer& operator<< (T v)
		{
			// This BufferVariable is freed by the caching system (yuck)
			Variables.push_back(NewBufferVariable(v));
			return *this;
		}

		// Returns a simple hash of the contents of Variables
		u32 Hash();

	private:
		TranslatedString* String;
			// pointer instead of reference allows assignment.
			// this class is returned by value, so it's nicer for the
			// compiler-generated copy ctor to be used.
		std::vector<BufferVariable*> Variables;
		CLocale* Locale;

		StringBuffer(TranslatedString* str, CLocale* loc)
			: String(str), Locale(loc)
		{
		}
	};

}

#endif // I18N_STRINGBUF_H
