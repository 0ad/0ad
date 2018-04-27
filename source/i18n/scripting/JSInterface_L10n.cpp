/* Copyright (C) 2018 Wildfire Games.
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

#include "precompiled.h"

#include "JSInterface_L10n.h"

#include "i18n/L10n.h"
#include "lib/utf8.h"
#include "ps/Profile.h"
#include "scriptinterface/ScriptInterface.h"

// Returns a translation of the specified English string into the current language.
std::wstring JSI_L10n::Translate(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& sourceString)
{
	return wstring_from_utf8(g_L10n.Translate(utf8_from_wstring(sourceString)));
}

// Returns a translation of the specified English string, for the specified context.
std::wstring JSI_L10n::TranslateWithContext(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& context, const std::wstring& sourceString)
{
	return wstring_from_utf8(g_L10n.TranslateWithContext(context, utf8_from_wstring(sourceString)));
}

// Return a translated version of the given strings (singular and plural) depending on an integer value.
std::wstring JSI_L10n::TranslatePlural(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& singularSourceString, const std::wstring& pluralSourceString, int number)
{
	return wstring_from_utf8(g_L10n.TranslatePlural(utf8_from_wstring(singularSourceString), utf8_from_wstring(pluralSourceString), number));
}

// Return a translated version of the given strings (singular and plural) depending on an integer value, for the specified context.
std::wstring JSI_L10n::TranslatePluralWithContext(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& context, const std::wstring& singularSourceString, const std::wstring& pluralSourceString, int number)
{
	return wstring_from_utf8(g_L10n.TranslatePluralWithContext(context, utf8_from_wstring(singularSourceString), utf8_from_wstring(pluralSourceString), number));
}

// Return a translated version of the given string, localizing it line by line.
std::wstring JSI_L10n::TranslateLines(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& sourceString)
{
	return wstring_from_utf8(g_L10n.TranslateLines(utf8_from_wstring(sourceString)));
}

// Return a translated version of the items in the specified array.
std::vector<std::wstring> JSI_L10n::TranslateArray(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::vector<std::wstring>& sourceArray)
{
	std::vector<std::wstring> translatedArray;
	for (const std::wstring& elem : sourceArray)
		translatedArray.push_back(wstring_from_utf8(g_L10n.Translate(utf8_from_wstring(elem))));

	return translatedArray;
}

std::wstring JSI_L10n::GetFallbackToAvailableDictLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& locale)
{
	return g_L10n.GetFallbackToAvailableDictLocale(locale);
}

// Return a localized version of a time given in milliseconds.
std::wstring JSI_L10n::FormatMillisecondsIntoDateStringLocal(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), UDate milliseconds, const std::wstring& formatString)
{
	return wstring_from_utf8(g_L10n.FormatMillisecondsIntoDateString(milliseconds, utf8_from_wstring(formatString), true));
}

// Return a localized version of a duration or a time in GMT given in milliseconds.
std::wstring JSI_L10n::FormatMillisecondsIntoDateStringGMT(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), UDate milliseconds, const std::wstring& formatString)
{
	return wstring_from_utf8(g_L10n.FormatMillisecondsIntoDateString(milliseconds, utf8_from_wstring(formatString), false));
}

// Return a localized version of the given decimal number.
std::wstring JSI_L10n::FormatDecimalNumberIntoString(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), double number)
{
	return wstring_from_utf8(g_L10n.FormatDecimalNumberIntoString(number));
}

std::vector<std::string> JSI_L10n::GetSupportedLocaleBaseNames(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_L10n.GetSupportedLocaleBaseNames();
}

std::vector<std::wstring> JSI_L10n::GetSupportedLocaleDisplayNames(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_L10n.GetSupportedLocaleDisplayNames();
}

std::string JSI_L10n::GetCurrentLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_L10n.GetCurrentLocaleString();
}

bool JSI_L10n::UseLongStrings(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_L10n.UseLongStrings();
}

std::vector<std::string> JSI_L10n::GetAllLocales(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_L10n.GetAllLocales();
}

std::string JSI_L10n::GetDictionaryLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& configLocale)
{
	return g_L10n.GetDictionaryLocale(configLocale);
}

std::vector<std::wstring> JSI_L10n::GetDictionariesForLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& locale)
{
	return g_L10n.GetDictionariesForLocale(locale);
}

std::string JSI_L10n::GetLocaleLanguage(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& locale)
{
	return g_L10n.GetLocaleLanguage(locale);
}

std::string JSI_L10n::GetLocaleBaseName(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& locale)
{
	return g_L10n.GetLocaleBaseName(locale);
}

std::string JSI_L10n::GetLocaleCountry(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& locale)
{
	return g_L10n.GetLocaleCountry(locale);
}

std::string JSI_L10n::GetLocaleScript(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& locale)
{
	return g_L10n.GetLocaleScript(locale);
}

bool JSI_L10n::ValidateLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& locale)
{
	return g_L10n.ValidateLocale(locale);
}

bool JSI_L10n::SaveLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& locale)
{
	return g_L10n.SaveLocale(locale);
}

void JSI_L10n::ReevaluateCurrentLocaleAndReload(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	g_L10n.ReevaluateCurrentLocaleAndReload();
}


void JSI_L10n::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<std::wstring, std::wstring, &Translate>("Translate");
	scriptInterface.RegisterFunction<std::wstring, std::string, std::wstring, &TranslateWithContext>("TranslateWithContext");
	scriptInterface.RegisterFunction<std::wstring, std::wstring, std::wstring, int, &TranslatePlural>("TranslatePlural");
	scriptInterface.RegisterFunction<std::wstring, std::string, std::wstring, std::wstring, int, &TranslatePluralWithContext>("TranslatePluralWithContext");
	scriptInterface.RegisterFunction<std::wstring, std::wstring, &TranslateLines>("TranslateLines");
	scriptInterface.RegisterFunction<std::vector<std::wstring>, std::vector<std::wstring>, &TranslateArray>("TranslateArray");
	scriptInterface.RegisterFunction<std::wstring, UDate, std::wstring, &FormatMillisecondsIntoDateStringLocal>("FormatMillisecondsIntoDateStringLocal");
	scriptInterface.RegisterFunction<std::wstring, UDate, std::wstring, &FormatMillisecondsIntoDateStringGMT>("FormatMillisecondsIntoDateStringGMT");
	scriptInterface.RegisterFunction<std::wstring, double, &FormatDecimalNumberIntoString>("FormatDecimalNumberIntoString");

	scriptInterface.RegisterFunction<std::vector<std::string>, &GetSupportedLocaleBaseNames>("GetSupportedLocaleBaseNames");
	scriptInterface.RegisterFunction<std::vector<std::wstring>, &GetSupportedLocaleDisplayNames>("GetSupportedLocaleDisplayNames");
	scriptInterface.RegisterFunction<std::string, &GetCurrentLocale>("GetCurrentLocale");
	scriptInterface.RegisterFunction<std::vector<std::string>, &GetAllLocales>("GetAllLocales");
	scriptInterface.RegisterFunction<std::string, std::string, &GetDictionaryLocale>("GetDictionaryLocale");
	scriptInterface.RegisterFunction<std::vector<std::wstring>, std::string, &GetDictionariesForLocale>("GetDictionariesForLocale");

	scriptInterface.RegisterFunction<bool, &UseLongStrings>("UseLongStrings");
	scriptInterface.RegisterFunction<std::string, std::string, &GetLocaleLanguage>("GetLocaleLanguage");
	scriptInterface.RegisterFunction<std::string, std::string, &GetLocaleBaseName>("GetLocaleBaseName");
	scriptInterface.RegisterFunction<std::string, std::string, &GetLocaleCountry>("GetLocaleCountry");
	scriptInterface.RegisterFunction<std::string, std::string, &GetLocaleScript>("GetLocaleScript");
	scriptInterface.RegisterFunction<std::wstring, std::string, &GetFallbackToAvailableDictLocale>("GetFallbackToAvailableDictLocale");

	scriptInterface.RegisterFunction<bool, std::string, &ValidateLocale>("ValidateLocale");
	scriptInterface.RegisterFunction<bool, std::string, &SaveLocale>("SaveLocale");
	scriptInterface.RegisterFunction<void, &ReevaluateCurrentLocaleAndReload>("ReevaluateCurrentLocaleAndReload");
}
