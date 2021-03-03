/* Copyright (C) 2021 Wildfire Games.
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
#include "ps/CLogger.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptInterface.h"

namespace JSI_L10n
{
L10n* L10nGetter(const ScriptRequest&, JS::CallArgs&)
{
	if (!g_L10n.IsInitialised())
	{
		LOGERROR("Trying to access g_L10n when it's not initialized!");
		return nullptr;
	}
	return &g_L10n.GetSingleton();
}

std::vector<std::string> TranslateArray(const std::vector<std::string>& sourceArray)
{
	std::vector<std::string> translatedArray;
	if (g_L10n.IsInitialised())
		for (const std::string& elem : sourceArray)
			translatedArray.push_back(g_L10n.Translate(elem));

	return translatedArray;
}

// Return a localized version of a time given in milliseconds.
std::string FormatMillisecondsIntoDateStringLocal(UDate milliseconds, const std::string& formatString)
{
	return g_L10n.FormatMillisecondsIntoDateString(milliseconds, formatString, true);
}

// Return a localized version of a duration or a time in GMT given in milliseconds.
std::string FormatMillisecondsIntoDateStringGMT(UDate milliseconds, const std::string& formatString)
{
	return g_L10n.FormatMillisecondsIntoDateString(milliseconds, formatString, false);
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
#define REGISTER_L10N(name) \
	ScriptFunction::Register<&L10n::name, &L10nGetter>(rq, #name);
#define REGISTER_L10N_FUNC(func, name) \
	ScriptFunction::Register<func, &L10nGetter>(rq, name);

	REGISTER_L10N(Translate)
	REGISTER_L10N(TranslateWithContext)
	REGISTER_L10N(TranslatePlural)
	REGISTER_L10N(TranslatePluralWithContext)
	REGISTER_L10N(TranslateLines)
	ScriptFunction::Register<&TranslateArray>(rq, "TranslateArray");
	ScriptFunction::Register<&FormatMillisecondsIntoDateStringLocal>(rq, "FormatMillisecondsIntoDateStringLocal");
	ScriptFunction::Register<&FormatMillisecondsIntoDateStringGMT>(rq, "FormatMillisecondsIntoDateStringGMT");
	REGISTER_L10N(FormatDecimalNumberIntoString)

	REGISTER_L10N(GetSupportedLocaleBaseNames)
	REGISTER_L10N(GetSupportedLocaleDisplayNames)
	REGISTER_L10N_FUNC(&L10n::GetCurrentLocaleString, "GetCurrentLocale");
	REGISTER_L10N(GetAllLocales)
	// Select the appropriate overload.
	REGISTER_L10N_FUNC(static_cast<std::string(L10n::*)(const std::string&) const>(&L10n::GetDictionaryLocale), "GetDictionaryLocale");
	REGISTER_L10N(GetDictionariesForLocale)

	REGISTER_L10N(UseLongStrings)
	REGISTER_L10N(GetLocaleLanguage)
	REGISTER_L10N(GetLocaleBaseName)
	REGISTER_L10N(GetLocaleCountry)
	REGISTER_L10N(GetLocaleScript)
	// Select the appropriate overload.
	REGISTER_L10N_FUNC(static_cast<std::wstring(L10n::*)(const std::string&) const>(&L10n::GetFallbackToAvailableDictLocale), "GetFallbackToAvailableDictLocale");

	// Select the appropriate overloads.
	REGISTER_L10N_FUNC(static_cast<bool(L10n::*)(const std::string&) const>(&L10n::ValidateLocale), "ValidateLocale");
	REGISTER_L10N_FUNC(static_cast<bool(L10n::*)(const std::string&) const>(&L10n::SaveLocale), "SaveLocale");
	REGISTER_L10N(ReevaluateCurrentLocaleAndReload)
#undef REGISTER_L10N
#undef REGISTER_L10N_FUNC
}
}
