// Things that are used by most I18n code:

#ifndef I18N_COMMON_H
#define I18N_COMMON_H

#include <string>

#include "Errors.h"

namespace I18n
{
	// Define an 'internal' string type, for no particular reason
	typedef std::wstring Str;
}

ERROR_GROUP(I18n);

// That was exciting.

#endif // I18N_COMMON_H
