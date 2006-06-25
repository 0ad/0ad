#include "precompiled.h"

#include "Pyrogenesis.h"
#include "ps/i18n.h"

DEFINE_ERROR(PS_OK, "OK");
DEFINE_ERROR(PS_FAIL, "Fail");


static const wchar_t* translate_no_mem = L"(no mem)";

// overrides ah_translate. registered in GameSetup.cpp
const wchar_t* psTranslate(const wchar_t* text)
{
	// make sure i18n system is (already|still) initialized.
	if(g_CurrentLocale)
	{
		// be prepared for this to fail, because translation potentially
		// involves script code and the JS context might be corrupted.
		try
		{
			CStrW ret = I18n::translate(text);
			const wchar_t* ret_dup = wcsdup(ret.c_str());
			return ret_dup? ret_dup : translate_no_mem;
		}
		catch(...)
		{
		}
	}

	// i18n not available: at least try and return the text (unchanged)
	const wchar_t* ret_dup = wcsdup(text);
	return ret_dup? ret_dup : translate_no_mem;
}

void psTranslateFree(const wchar_t* text)
{
	if(text != translate_no_mem)
		free((void*)text);
}
