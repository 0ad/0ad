/*

Returned by Translate(), and used to collect variables from the user
before converting into a string.

*/

#ifndef I18N_STRINGBUF_H
#define I18N_STRINGBUF_H

#include "Common.h"
#include "TranslatedString.h"
#include "CStr.h"

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
		StringBuffer(TranslatedString&, CLocale*);
		TranslatedString& String;
		std::vector<BufferVariable*> Variables;
		CLocale* Locale;
	};

}

#endif // I18N_STRINGBUF_H
