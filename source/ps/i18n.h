#include "i18n/Interface.h"

extern I18n::CLocale_interface* g_CurrentLocale;

namespace I18n
{
	inline StringBuffer translate(const wchar_t* s) { return g_CurrentLocale->Translate(s); }

	bool LoadLanguage(const char* name);
	const char* CurrentLanguageName();
	void Shutdown();
}
