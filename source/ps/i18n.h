#include "i18n/Interface.h"

extern I18n::CLocale_interface* g_CurrentLocale;

#define translate(s) g_CurrentLocale->Translate(s)

namespace I18n
{
	bool LoadLanguage(const char* name);
	const char* CurrentLanguageName();
	void Shutdown();
}
