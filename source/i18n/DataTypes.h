#ifndef I18N_DATATYPES_H
#define I18N_DATATYPES_H

#include "StrImmutable.h"

namespace I18n
{
	// Allow translate("Hello $you")<<I18n::Name(playername), which
	// won't attempt to automatically translate the player's name.
	// Templated to allow char* and wchar_t*
	struct Name
	{
		template<typename T> Name(T d) : value(d) {}
		StrImW value;
	};
}


#endif // I18N_DATATYPES_H
