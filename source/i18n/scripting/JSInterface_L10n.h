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

/**
 * Namespace for the functions of the JavaScript interface for
 * internationalization and localization.
 *
 * This namespace defines JavaScript interfaces to functions defined in L10n and
 * related helper functions.
 *
 * @sa http://trac.wildfiregames.com/wiki/Internationalization_and_Localization
 */
namespace JSI_L10n
{
	/**
	 * Registers the functions of the JavaScript interface for
	 * internationalization and localization into the specified JavaScript
	 * context.
	 *
	 * @param ScriptInterface JavaScript context where RegisterScriptFunctions()
	 *        registers the functions.
	 *
	 * @sa GuiScriptingInit()
	 */
	void RegisterScriptFunctions(ScriptInterface& ScriptInterface);

	/**
	 * Returns the translation of the specified string to the
	 * @link L10n::GetCurrentLocale() current locale@endlink.
	 *
	 * This is a JavaScript interface to L10n::Translate().
	 *
	 * @param pCxPrivate   JavaScript context.
	 * @param sourceString String to translate to the current locale.
	 * @return Translation of @p sourceString to the current locale, or
	 *         @p sourceString if there is no translation available.
	 */
	std::wstring Translate(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring sourceString);

	/**
	 * Returns the translation of the specified string to the
	 * @link L10n::GetCurrentLocale() current locale@endlink in the specified
	 * context.
	 *
	 * This is a JavaScript interface to L10n::TranslateWithContext().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param context Context where the string is used. See
	 *        http://www.gnu.org/software/gettext/manual/html_node/Contexts.html
	 * @param sourceString String to translate to the current locale.
	 * @return Translation of @p sourceString to the current locale in the
	 *         specified @p context, or @p sourceString if there is no
	 *         translation available.
	 */
	std::wstring TranslateWithContext(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string context, std::wstring sourceString);

	/**
	 * Returns the translation of the specified string to the
	 * @link L10n::GetCurrentLocale() current locale@endlink based on the
	 * specified number.
	 *
	 * This is a JavaScript interface to L10n::TranslatePlural().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param singularSourceString String to translate to the current locale,
	 *        in English’ singular form.
	 * @param pluralSourceString String to translate to the current locale, in
	 *        English’ plural form.
	 * @param number Number that determines the required form of the translation
	 *        (or the English string if no translation is available).
	 * @return Translation of the source string to the current locale for the
	 *         specified @p number, or either @p singularSourceString (if
	 *         @p number is 1) or @p pluralSourceString (if @p number is not 1)
	 *         if there is no translation available.
	 */
	std::wstring TranslatePlural(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring singularSourceString, std::wstring pluralSourceString, int number);

	/**
	 * Returns the translation of the specified string to the
	 * @link L10n::GetCurrentLocale() current locale@endlink in the specified
	 * context, based on the specified number.
	 *
	 * This is a JavaScript interface to L10n::TranslatePluralWithContext().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param context Context where the string is used. See
	 *        http://www.gnu.org/software/gettext/manual/html_node/Contexts.html
	 * @param singularSourceString String to translate to the current locale,
	 *        in English’ singular form.
	 * @param pluralSourceString String to translate to the current locale, in
	 *        English’ plural form.
	 * @param number Number that determines the required form of the translation
	 *        (or the English string if no translation is available).	 *
	 * @return Translation of the source string to the current locale in the
	 *         specified @p context and for the specified @p number, or either
	 *         @p singularSourceString (if @p number is 1) or
	 *         @p pluralSourceString (if @p number is not 1) if there is no
	 *         translation available.
	 */
	std::wstring TranslatePluralWithContext(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string context, std::wstring singularSourceString, std::wstring pluralSourceString, int number);

	/**
	 * Translates a text line by line to the
	 * @link L10n::GetCurrentLocale() current locale@endlink.
	 *
	 * TranslateLines() is helpful when you need to translate a plain text file
	 * after you load it.
	 *
	 * This is a JavaScript interface to L10n::TranslateLines().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param sourceString Text to translate to the current locale.
	 * @return Line by line translation of @p sourceString to the current
	 *         locale. Some of the lines in the returned text may be in English
	 *         because there was not translation available for them.
	 */
	std::wstring TranslateLines(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring sourceString);

	/**
	 * Translate each of the strings of a JavaScript array to the
	 * @link L10n::GetCurrentLocale() current locale@endlink.
	 *
	 * This is a helper function that loops through the items of the input array
	 * and calls L10n::Translate() on each of them.
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param sourceArray JavaScript array of strings to translate to the
	 *        current locale.
	 * @return Item by item translation of @p sourceArray to the current locale.
	 *         Some of the items in the returned array may be in English because
	 *         there was not translation available for them.
	 */
	std::vector<std::wstring> TranslateArray(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::vector<std::wstring> sourceArray);

	/**
	 * Returns the specified date using the specified date format.
	 *
	 * This is a JavaScript interface to
	 * L10n::FormatMillisecondsIntoDateString().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param milliseconds Date specified as a UNIX timestamp in milliseconds
	 *        (not seconds). If you have a JavaScript @c ​Date object, you can
	 *        use @c ​Date.getTime() to obtain the UNIX time in milliseconds.
	 * @param formatString Date format string defined using ICU date formatting
	 *        symbols. Usually, you internationalize the format string and
	 *        get it translated before you pass it to
	 *        FormatMillisecondsIntoDateString().
	 * @return String containing the specified date with the specified date
	 *         format.
	 *
	 * @sa http://en.wikipedia.org/wiki/Unix_time
	 * @sa https://sites.google.com/site/icuprojectuserguide/formatparse/datetime?pli=1#TOC-Date-Field-Symbol-Table
	 */
	std::wstring FormatMillisecondsIntoDateString(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), UDate milliseconds, std::wstring formatString);

	/**
	 * Returns the specified floating-point number as a string, with the number
	 * formatted as a decimal number using the
	 * @link L10n::GetCurrentLocale() current locale@endlink.
	 *
	 * This is a JavaScript interface to L10n::FormatDecimalNumberIntoString().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param number Number to format.
	 * @return Decimal number formatted using the current locale.
	 */
	std::wstring FormatDecimalNumberIntoString(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), double number);

	/**
	 * Returns an array of supported locale codes sorted alphabetically.
	 *
	 * A locale code is a string such as "de" or "pt_BR".
	 *
	 * If yours is a development copy (the ‘config/dev.cfg’ file is found in the
	 * virtual filesystem), the output array may include the special “long”
	 * locale code.
	 *
	 * This is a JavaScript interface to L10n::GetSupportedLocaleBaseNames().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @return Array of supported locale codes.
	 *
	 * @sa GetSupportedLocaleDisplayNames()
	 * @sa GetAllLocales()
	 * @sa GetCurrentLocale()
	 *
	 * @sa http://trac.wildfiregames.com/wiki/Implementation_of_Internationalization_and_Localization#LongStringsLocale
	 */
	std::vector<std::string> GetSupportedLocaleBaseNames(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));

	/**
	 * Returns an array of supported locale names sorted alphabetically by
	 * locale code.
	 *
	 * A locale code is a string such as "de" or "pt_BR".
	 *
	 * If yours is a development copy (the ‘config/dev.cfg’ file is found in the
	 * virtual filesystem), the output array may include the special “Long
	 * Strings” locale name.
	 *
	 * This is a JavaScript interface to L10n::GetSupportedLocaleDisplayNames().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @return Array of supported locale codes.
	 *
	 * @sa GetSupportedLocaleBaseNames()
	 *
	 * @sa http://trac.wildfiregames.com/wiki/Implementation_of_Internationalization_and_Localization#LongStringsLocale
	 */
	std::vector<std::wstring> GetSupportedLocaleDisplayNames(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));

	/**
	 * Returns the code of the current locale.
	 *
	 * A locale code is a string such as "de" or "pt_BR".
	 *
	 * This is a JavaScript interface to L10n::GetCurrentLocaleString().
	 *
	 * @param pCxPrivate JavaScript context.
	 *
	 * @sa GetSupportedLocaleBaseNames()
	 * @sa GetAllLocales()
	 * @sa ReevaluateCurrentLocaleAndReload()
	 */
	std::string GetCurrentLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));

	/**
	 * Returns an array of locale codes supported by ICU.
	 *
	 * A locale code is a string such as "de" or "pt_BR".
	 *
	 * This is a JavaScript interface to L10n::GetAllLocales().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @return Array of supported locale codes.
	 *
	 * @sa GetSupportedLocaleBaseNames()
	 * @sa GetCurrentLocale()
	 *
	 * @sa http://www.icu-project.org/apiref/icu4c/classicu_1_1Locale.html#a073d70df8c9c8d119c0d42d70de24137
	 */
	std::vector<std::string> GetAllLocales(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));

	/**
	 * Returns the code of the recommended locale for the current user that the
	 * game supports.
	 *
	 * “That the game supports” means both that a translation file is available
	 * for that locale and that the locale code is either supported by ICU or
	 * the special “long” locale code.
	 *
	 * The mechanism to select a recommended locale follows this logic:
	 *     1. First see if the game supports the specified locale,\n
	 *       @p configLocale.
	 *     2. Otherwise, check the system locale and see if the game supports\n
	 *       that locale.
	 *     3. Else, return the default locale, ‘en_US’.
	 *
	 * This is a JavaScript interface to L10n::GetDictionaryLocale(std::string).
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param configLocale Locale to check for support first. Pass an empty
	 *        string to check the system locale directly.
	 * @return Code of a locale that the game supports.
	 *
	 * @sa http://trac.wildfiregames.com/wiki/Implementation_of_Internationalization_and_Localization#LongStringsLocale
	 */
	std::string GetDictionaryLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string configLocale);

	/**
	 * Returns an array of paths to files in the virtual filesystem that provide
	 * translations for the specified locale code.
	 *
	 * This is a JavaScript interface to L10n::GetDictionariesForLocale().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param locale Locale code.
	 * @return Array of paths to files in the virtual filesystem that provide
	 * translations for @p locale.
	 */
	std::vector<std::wstring> GetDictionariesForLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);

	/**
	 * Returns the ISO-639 language code of the specified locale code.
	 *
	 * For example, if you specify the ‘en_US’ locate, it returns ‘en’.
	 *
	 * This is a JavaScript interface to L10n::GetLocaleLanguage().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param locale Locale code.
	 * @return Language code.
	 *
	 * @sa http://www.icu-project.org/apiref/icu4c/classicu_1_1Locale.html#af36d821adced72a870d921ebadd0ca93
	 */
	std::string GetLocaleLanguage(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);

	/**
	 * Returns the programmatic code of the entire locale without keywords.
	 *
	 * This is a JavaScript interface to L10n::GetLocaleBaseName().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param locale Locale code.
	 * @return Locale code without keywords.
	 *
	 * @sa http://www.icu-project.org/apiref/icu4c/classicu_1_1Locale.html#a4c1acbbdf95dc15599db5f322fa4c4d0
	 */
	std::string GetLocaleBaseName(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);

	/**
	 * Returns the ISO-3166 country code of the specified locale code.
	 *
	 * For example, if you specify the ‘en_US’ locate, it returns ‘US’.
	 *
	 * This is a JavaScript interface to L10n::GetLocaleCountry().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param locale Locale code.
	 * @return Country code.
	 *
	 * @sa http://www.icu-project.org/apiref/icu4c/classicu_1_1Locale.html#ae3f1fc415c00d4f0ab33288ceadccbf9
	 */
	std::string GetLocaleCountry(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);

	/**
	 * Returns the ISO-15924 abbreviation script code of the specified locale code.
	 *
	 * This is a JavaScript interface to L10n::GetLocaleScript().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param locale Locale code.
	 * @return Script code.
	 *
	 * @sa http://www.icu-project.org/apiref/icu4c/classicu_1_1Locale.html#a5e0145a339d30794178a1412dcc55abe
	 */
	std::string GetLocaleScript(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);

	
	std::wstring GetFallbackToAvailableDictLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);

	/**
	 * Returns @c true if the current locale is the special “Long Strings”
	 * locale. It returns @c false otherwise.
	 *
	 * This is a JavaScript interface to L10n::UseLongStrings().
	 *
	 * @param pCxPrivate JavaScript context.	 *
	 * @return Whether the current locale is the special “Long Strings”
	 *         (@c true) or not (@c false).
	 */
	bool UseLongStrings(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));

	/**
	 * Returns @c true if the locale is supported by both ICU and the game. It
	 * returns @c false otherwise.
	 *
	 * It returns @c true if both of these conditions are true:
	 *     1. ICU has resources for that locale (which also ensures it’s a valid\n
	 *       locale string).
	 *     2. Either a dictionary for language_country or for language is\n
	 *       available.
	 *
	 * This is a JavaScript interface to L10n::ValidateLocale(const std::string&).
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param locale Locale to check.
	 * @return Whether @p locale is supported by both ICU and the game (@c true)
	 *         or not (@c false).
	 */
	bool ValidateLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);

	/**
	 * Saves the specified locale in the game configuration file.
	 *
	 * The next time that the game starts, the game uses the locale in the
	 * configuration file if there are translation files available for it.
	 *
	 * SaveLocale() checks the validity of the specified locale with
	 * ValidateLocale(). If the specified locale is not valid, SaveLocale()
	 * returns @c false and does not save the locale to the configuration file.
	 *
	 * This is a JavaScript interface to L10n::SaveLocale().
	 *
	 * @param pCxPrivate JavaScript context.
	 * @param locale Locale to save to the configuration file.
	 * @return Whether the specified locale is valid (@c true) or not
	 *         (@c false).
	 */
	bool SaveLocale(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::string locale);

	/**
	 * Determines the best, supported locale for the current user, makes it the
	 * current game locale and reloads the translations dictionary with
	 * translations for that locale.
	 *
	 * To determine the best locale, ReevaluateCurrentLocaleAndReload():
	 *     1. Checks the user game configuration.
	 *     2. If the locale is not defined there, it checks the system locale.
	 *     3. If none of those locales are supported by the game, the default\n
	 *       locale, ‘en_US’, is used.
	 *
	 * This is a JavaScript interface to L10n::ReevaluateCurrentLocaleAndReload().
	 *
	 * @param pCxPrivate JavaScript context.
	 *
	 * @sa GetCurrentLocale()
	 */
	void ReevaluateCurrentLocaleAndReload(ScriptInterface::CxPrivate* UNUSED(pCxPrivate));
}

#endif
