#include "precompiled.h"

#include "ps/i18n.h"
#include "ps/CVFSFile.h"
#include "scripting/ScriptingHost.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "i18n"

I18n::CLocale_interface* g_CurrentLocale = NULL;

bool I18n::LoadLanguage(const char* name)
{
	CLocale_interface* locale_ptr = I18n::NewLocale(g_ScriptingHost.getContext());

	if (! locale_ptr)
	{
		debug_warn("Failed to create locale");
		return false;
	}

	std::auto_ptr<CLocale_interface> locale (locale_ptr);

	{
		CVFSFile strings;

		CStr filename = CStr("language/")+name+"/phrases.lng";

		if (strings.Load(filename) != PSRETURN_OK)
		{
			LOG(ERROR, LOG_CATEGORY, "Error opening language string file '%s'", filename.c_str());
			return false;
		}

		if (! locale->LoadStrings( (const char*) strings.GetBuffer() ))
		{
			LOG(ERROR, LOG_CATEGORY, "Error loading language string file '%s'", filename.c_str());
			return false;
		}
	}

	// Free any previously loaded data
	delete g_CurrentLocale;

	g_CurrentLocale = locale.release();
	return true;
}

void I18n::Shutdown()
{
	delete g_CurrentLocale;
}
