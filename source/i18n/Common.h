// Things that are used by most I18n code:

#ifndef INCLUDED_I18N_COMMON
#define INCLUDED_I18N_COMMON

#include <string>

#include "ps/Errors.h"

namespace I18n
{
	// Define an 'internal' string type, for no particular reason
	typedef std::wstring Str;
}

ERROR_GROUP(I18n);

// That was exciting.

#endif // INCLUDED_I18N_COMMON
