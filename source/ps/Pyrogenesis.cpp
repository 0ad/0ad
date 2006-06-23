#include "precompiled.h"

#include "Pyrogenesis.h"
#include "ps/i18n.h"

DEFINE_ERROR(PS_OK, "OK");
DEFINE_ERROR(PS_FAIL, "Fail");

// overrides ah_translate. registered in GameSetup.cpp
const wchar_t* psTranslate(const wchar_t* text)
{
	// TODO: leaks memory returned by wcsdup

	// make sure i18n system is (already|still) initialized.
	if(g_CurrentLocale)
	{
		// be prepared for this to fail, because translation potentially
		// involves script code and the JS context might be corrupted.
		try
		{
			CStrW ret = I18n::translate(text);
			const wchar_t* text2 = wcsdup(ret.c_str());
			// only overwrite if wcsdup succeeded, i.e. not out of memory.
			if(text2)
				text = text2;
		}
		catch(...)
		{
		}
	}

	return text;
}
