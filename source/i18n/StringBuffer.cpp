#include "precompiled.h"

#include "CLocale.h"
#include "TSComponent.h"
#include "StringBuffer.h"

// Vaguely useful utility function for deleting stuff
template<typename T> void delete_fn(T* v) { delete v; }


#include <iostream>

#include "ps/CLogger.h"
#define LOG_CATEGORY "i18n"

using namespace I18n;

#ifdef I18NDEBUG
namespace I18n { bool g_UsedCache; }
#endif

I18n::StringBuffer::operator Str()
{
	#ifdef I18NDEBUG
	g_UsedCache = false;
	#endif

	if (Variables.size() != String->VarCount)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "I18n: Incorrect number of parameters passed to Translate");

		// Tidy up
		std::for_each(Variables.begin(), Variables.end(), delete_fn<BufferVariable>);

		return L"(translation error)";
	}

	if (String->VarCount == 0)
	{
		if (String->Parts.size())
			return String->Parts[0]->ToString(Locale, Variables).str();
		else
			return Str();
	}

	Str ret;

	if (Locale->ReadCached(this, ret))
	{
		// Found in cache

		#ifdef I18NDEBUG
		g_UsedCache = true;
		#endif

		// Clean up the current allocated data
		std::for_each(Variables.begin(), Variables.end(), delete_fn<BufferVariable>);
		// Return the cached string
		return ret;
	}

	// Not in cache - construct the string
	for (std::vector<const TSComponent*>::iterator it = String->Parts.begin(),
		end = String->Parts.end();
		it != end; ++it)
	{
		ret += (*it)->ToString(Locale, Variables).str();
	}

	Locale->AddToCache(this, ret);

	return ret;
}

u32 I18n::StringBuffer::Hash()
{
	u32 hash = 0;
	size_t max = Variables.size();
	for (size_t i = 0; i < max; ++i)
		hash += Variables[i]->Hash();
	return hash;
}
