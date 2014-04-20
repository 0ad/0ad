/* Copyright (C) 2014 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_JSINTERFACE_L10N
#define INCLUDED_JSINTERFACE_L10N

#include "scriptinterface/ScriptInterface.h"
#include "lib/external_libraries/icu.h"

namespace JSI_L10n
{
	void RegisterScriptFunctions(ScriptInterface& ScriptInterface);

	std::wstring Translate(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring sourceString);
	std::wstring TranslateWithContext(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string context, std::wstring sourceString);
	std::wstring TranslatePlural(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring singularSourceString, std::wstring pluralSourceString, int number);
	std::wstring TranslatePluralWithContext(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string context, std::wstring singularSourceString, std::wstring pluralSourceString, int number);
	std::wstring TranslateLines(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring sourceString);
	std::vector<std::wstring> TranslateArray(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::vector<std::wstring> sourceArray);
	std::wstring MarkToTranslate(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring sourceString);
	std::wstring FormatMillisecondsIntoDateString(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), UDate milliseconds, std::wstring formatString);
	std::wstring FormatDecimalNumberIntoString(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), double number);

	std::vector<std::string> GetSupportedLocaleBaseNames(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));
	std::vector<std::wstring> GetSupportedLocaleDisplayNames(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));
	std::string GetCurrentLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));
	std::vector<std::string> GetAllLocales(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));
	std::string GetDictionaryLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string configLocale);
	std::vector<std::wstring> GetDictionariesForDictLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);

	std::string GetLocaleLanguage(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);
	std::string GetLocaleBaseName(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);
	std::string GetLocaleCountry(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);
	std::string GetLocaleScript(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);

	bool ValidateLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);
	bool SaveLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);
	void ReevaluateCurrentLocaleAndReload(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));
}

#endif
