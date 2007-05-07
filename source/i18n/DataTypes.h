#ifndef INCLUDED_I18N_DATATYPES
#define INCLUDED_I18N_DATATYPES

#include "StrImmutable.h"

namespace I18n
{
	// Use for names of objects that should be translated, e.g.
	// translate("Construct $obj")<<I18n::Noun(selectedobject.name)
	struct Noun
	{
		template<typename T> Noun(T d) : value(d) {}
		StrImW value;
	};

	// Allow translate("Hello $you")<<I18n::Name(playername), which
	// won't attempt to translate the player's name.
	// Templated to allow char* and wchar_t*
	struct Name
	{
		template<typename T> Name(T d) : value(d) {}
		StrImW value;
	};

	// Also allow I18n::Raw("english message"), which does the same
	// non-translation but makes more sense when writing e.g. error messages
	typedef Name Raw;
}


#endif // INCLUDED_I18N_DATATYPES
