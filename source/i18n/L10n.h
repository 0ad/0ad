/* Copyright (c) 2014 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef L10N_H
#define L10N_H

#include <string>
#include <vector>

#include "lib/code_annotation.h"
#include "lib/external_libraries/icu.h"
#include "lib/external_libraries/tinygettext.h"

#include "lib/file/vfs/vfs_path.h"

/**
 * %Singleton for internationalization and localization.
 *
 * @sa http://trac.wildfiregames.com/wiki/Internationalization_and_Localization
 */
class L10n
{
	/**
	 * Marks the L10n class as ‘noncopyable’.
	 *
	 * This is required, as the class works as a singleton.
	 *
	 * @sa #NONCOPYABLE(className)
	 */
	NONCOPYABLE(L10n);
public:

	/**
	 * Creates an instance of L10n.
	 *
	 * L10n is a singleton. Use Instance() instead of creating you own instances
	 * of L10n.
	 */
	L10n();

	/**
	 * Handles the descruction of L10n.
	 *
	 * Never destroy the L10n singleton manually. It must run as long as the
	 * game runs, and it is destroyed automatically when you quit the game.
	 */
	~L10n();

	/**
	 * Types of dates.
	 *
	 * @sa LocalizeDateTime()
	 */
	enum DateTimeType {
		DateTime, ///< Both date and time.
		Date, ///< Only date.
		Time ///< Only time.
	};

	/**
	 * Returns the running instance of the internationalization and localization
	 * singleton.
	 *
	 * If there is no instance yet, Instance() creates one and returns it.
	 */
	static L10n& Instance();

	/**
	 * Returns the current locale.
	 *
	 * @sa GetCurrentLocaleString()
	 * @sa GetSupportedLocaleBaseNames()
	 * @sa GetAllLocales()
	 * @sa ReevaluateCurrentLocaleAndReload()
	 */
	Locale GetCurrentLocale();

	/**
	 * Returns the code of the current locale.
	 *
	 * A locale code is a string such as "de" or "pt_BR".
	 *
	 * @sa GetCurrentLocale()
	 * @sa GetSupportedLocaleBaseNames()
	 * @sa GetAllLocales()
	 * @sa ReevaluateCurrentLocaleAndReload()
	 */
	std::string GetCurrentLocaleString();

	/**
	 * Returns a vector of locale codes supported by ICU.
	 *
	 * A locale code is a string such as "de" or "pt_BR".
	 *
	 * @return Vector of supported locale codes.
	 *
	 * @sa GetSupportedLocaleBaseNames()
	 * @sa GetCurrentLocale()
	 *
	 * @sa http://www.icu-project.org/apiref/icu4c/classicu_1_1Locale.html#a073d70df8c9c8d119c0d42d70de24137
	 */
	std::vector<std::string> GetAllLocales();

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
	 * @param localeCode Locale to save to the configuration file.
	 * @return Whether the specified locale is valid (@c true) or not
	 *         (@c false).
	 */
	bool SaveLocale(const std::string& localeCode);

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
	 * @param locale Locale to save to the configuration file.
	 * @return Whether the specified locale is valid (@c true) or not
	 *         (@c false).
	 */
	bool SaveLocale(Locale locale);

	/**
	 * Returns an array of supported locale codes sorted alphabetically.
	 *
	 * A locale code is a string such as "de" or "pt_BR".
	 *
	 * If yours is a development copy (the ‘config/dev.cfg’ file is found in the
	 * virtual filesystem), the output array may include the special “long”
	 * locale code.
	 *
	 * @return Array of supported locale codes.
	 *
	 * @sa GetSupportedLocaleDisplayNames()
	 * @sa GetAllLocales()
	 * @sa GetCurrentLocale()
	 *
	 * @sa http://trac.wildfiregames.com/wiki/Implementation_of_Internationalization_and_Localization#LongStringsLocale
	 */
	std::vector<std::string> GetSupportedLocaleBaseNames();

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
	 * @return Array of supported locale codes.
	 *
	 * @sa GetSupportedLocaleBaseNames()
	 *
	 * @sa http://trac.wildfiregames.com/wiki/Implementation_of_Internationalization_and_Localization#LongStringsLocale
	 */
	std::vector<std::wstring> GetSupportedLocaleDisplayNames();

	/**
	 * Returns the ISO-639 language code of the specified locale code.
	 *
	 * For example, if you specify the ‘en_US’ locate, it returns ‘en’.
	 *
	 * @param locale Locale code.
	 * @return Language code.
	 *
	 * @sa http://www.icu-project.org/apiref/icu4c/classicu_1_1Locale.html#af36d821adced72a870d921ebadd0ca93
	 */
	std::string GetLocaleLanguage(const std::string& locale);

	/**
	 * Returns the programmatic code of the entire locale without keywords.
	 *
	 * @param locale Locale code.
	 * @return Locale code without keywords.
	 *
	 * @sa http://www.icu-project.org/apiref/icu4c/classicu_1_1Locale.html#a4c1acbbdf95dc15599db5f322fa4c4d0
	 */
	std::string GetLocaleBaseName(const std::string& locale);

	/**
	 * Returns the ISO-3166 country code of the specified locale code.
	 *
	 * For example, if you specify the ‘en_US’ locate, it returns ‘US’.
	 *
	 * @param locale Locale code.
	 * @return Country code.
	 *
	 * @sa http://www.icu-project.org/apiref/icu4c/classicu_1_1Locale.html#ae3f1fc415c00d4f0ab33288ceadccbf9
	 */
	std::string GetLocaleCountry(const std::string& locale);

	/**
	 * Returns the ISO-15924 abbreviation script code of the specified locale code.
	 *
	 * @param locale Locale code.
	 * @return Script code.
	 *
	 * @sa http://www.icu-project.org/apiref/icu4c/classicu_1_1Locale.html#a5e0145a339d30794178a1412dcc55abe
	 */
	std::string GetLocaleScript(const std::string& locale);

	/**
	 * Returns @c true if the current locale is the special “Long Strings”
	 * locale. It returns @c false otherwise.
	 *
	 * @return Whether the current locale is the special “Long Strings”
	 *         (@c true) or not (@c false).
	 */
	bool UseLongStrings();

	/**
	 * Returns an array of paths to files in the virtual filesystem that provide
	 * translations for the specified locale code.
	 *
	 * @param locale Locale code.
	 * @return Array of paths to files in the virtual filesystem that provide
	 * translations for @p locale.
	 */
	std::vector<std::wstring> GetDictionariesForLocale(const std::string& locale);
	
	std::wstring GetFallbackToAvailableDictLocale(const Locale& locale);
	
	std::wstring GetFallbackToAvailableDictLocale(const std::string& locale);
	
	struct CheckLangAndCountry
	{
		CheckLangAndCountry(const Locale& locale);
		const Locale& m_MatchLocale;
		
		bool operator()(const Locale* const locale) const;

	private:
		const CheckLangAndCountry& operator=(const CheckLangAndCountry&);
	};
	
	struct CheckLang
	{
		CheckLang(const Locale& locale);
		const Locale& m_MatchLocale;
		
		bool operator()(const Locale* const locale) const;

	private:
		const CheckLang& operator=(const CheckLang&);
	};

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
	 * @param configLocaleString Locale to check for support first. Pass an
	 *        empty string to check the system locale directly.
	 * @return Code of a locale that the game supports.
	 *
	 * @sa http://trac.wildfiregames.com/wiki/Implementation_of_Internationalization_and_Localization#LongStringsLocale
	 */
	std::string GetDictionaryLocale(std::string configLocaleString);

	/**
	 * Saves an instance of the recommended locale for the current user that the
	 * game supports in the specified variable.
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
	 * @param configLocaleString Locale to check for support first. Pass an
	 *        empty string to check the system locale directly.
	 * @param outLocale Variable where GetDictionaryLocale(std::string, Locale&)
	 *        must store the recommended locale.
	 *
	 * @sa http://trac.wildfiregames.com/wiki/Implementation_of_Internationalization_and_Localization#LongStringsLocale
	 */
	void GetDictionaryLocale(std::string configLocaleString, Locale& outLocale);

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
	 * @sa GetCurrentLocale()
	 */
	void ReevaluateCurrentLocaleAndReload();

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
	 * @param locale Locale to check.
	 * @return Whether @p locale is supported by both ICU and the game (@c true)
	 *         or not (@c false).
	 */
	bool ValidateLocale(Locale locale);

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
	 * @param localeCode Code of the locale to check.
	 * @return Whether @p locale is supported by both ICU and the game (@c true)
	 *         or not (@c false).
	 */
	bool ValidateLocale(const std::string& localeCode);

	/**
	 * Returns the translation of the specified string to the
	 * @link L10n::GetCurrentLocale() current locale@endlink.
	 *
	 * @param sourceString String to translate to the current locale.
	 * @return Translation of @p sourceString to the current locale, or
	 *         @p sourceString if there is no translation available.
	 */
	std::string Translate(const std::string& sourceString);

	/**
	 * Returns the translation of the specified string to the
	 * @link L10n::GetCurrentLocale() current locale@endlink in the specified
	 * context.
	 *
	 * @param context Context where the string is used. See
	 *        http://www.gnu.org/software/gettext/manual/html_node/Contexts.html
	 * @param sourceString String to translate to the current locale.
	 * @return Translation of @p sourceString to the current locale in the
	 *         specified @p context, or @p sourceString if there is no
	 *         translation available.
	 */
	std::string TranslateWithContext(const std::string& context, const std::string& sourceString);

	/**
	 * Returns the translation of the specified string to the
	 * @link L10n::GetCurrentLocale() current locale@endlink based on the
	 * specified number.
	 *
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
	std::string TranslatePlural(const std::string& singularSourceString, const std::string& pluralSourceString, int number);

	/**
	 * Returns the translation of the specified string to the
	 * @link L10n::GetCurrentLocale() current locale@endlink in the specified
	 * context, based on the specified number.
	 *
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
	std::string TranslatePluralWithContext(const std::string& context, const std::string& singularSourceString, const std::string& pluralSourceString, int number);

	/**
	 * Translates a text line by line to the
	 * @link L10n::GetCurrentLocale() current locale@endlink.
	 *
	 * TranslateLines() is helpful when you need to translate a plain text file
	 * after you load it.
	 *
	 * @param sourceString Text to translate to the current locale.
	 * @return Line by line translation of @p sourceString to the current
	 *         locale. Some of the lines in the returned text may be in English
	 *         because there was not translation available for them.
	 */
	std::string TranslateLines(const std::string& sourceString);

	/**
	 * Parses the date in the input string using the specified date format, and
	 * returns the parsed date as a UNIX timestamp in milliseconds (not
	 * seconds).
	 *
	 * @param dateTimeString String containing the date to parse.
	 * @param dateTimeFormat Date format string to parse the input date, defined
	 *        using ICU date formatting symbols.
	 * @param locale Locale to use when parsing the input date.
	 * @return Specified date as a UNIX timestamp in milliseconds (not seconds).
	 *
	 * @sa GetCurrentLocale()
	 *
	 * @sa http://en.wikipedia.org/wiki/Unix_time
	 * @sa https://sites.google.com/site/icuprojectuserguide/formatparse/datetime?pli=1#TOC-Date-Field-Symbol-Table
	 */
	UDate ParseDateTime(const std::string& dateTimeString, const std::string& dateTimeFormat, const Locale& locale);

	/**
	 * Returns the specified date using the specified date format.
	 *
	 * @param dateTime Date specified as a UNIX timestamp in milliseconds
	 *        (not seconds).
	 * @param type Whether the formatted date must show both the date and the
	 *        time, only the date or only the time.
	 * @param style ICU style for the formatted date.
	 * @return String containing the specified date with the specified date
	 *         format.
	 *
	 * @sa http://en.wikipedia.org/wiki/Unix_time
	 * @sa http://icu-project.org/apiref/icu4c521/classicu_1_1DateFormat.html
	 */
	std::string LocalizeDateTime(const UDate& dateTime, DateTimeType type, DateFormat::EStyle style);

	/**
	 * Returns the specified date using the specified date format.
	 *
	 * @param milliseconds Date specified as a UNIX timestamp in milliseconds
	 *        (not seconds).
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
	std::string FormatMillisecondsIntoDateString(UDate milliseconds, const std::string& formatString);

	/**
	 * Returns the specified floating-point number as a string, with the number
	 * formatted as a decimal number using the
	 * @link L10n::GetCurrentLocale() current locale@endlink.
	 *
	 * @param number Number to format.
	 * @return Decimal number formatted using the current locale.
	 */
	std::string FormatDecimalNumberIntoString(double number);

	/**
	 * Returns the localized version of the specified path if there is one for
	 * the @link L10n::GetCurrentLocale() current locale@endlink.
	 *
	 * If there is no localized version of the specified path, it returns the
	 * specified path.
	 *
	 * For example, if the code of the current locale is ‘de_DE’, LocalizePath()
	 * splits the input path into folder path and file name, and checks whether
	 * the ‘<folder>/l10n/de/<file>’ file exists. If it does, it returns that
	 * path. Otherwise, it returns the input path, verbatim.
	 *
	 * This function is used for file localization (for example, image
	 * localization).
	 *
	 * @param sourcePath %Path to localize.
	 * @return Localized path if it exists, @c sourcePath otherwise.
	 *
	 * @sa http://trac.wildfiregames.com/wiki/Localization#LocalizingImages
	 */
	VfsPath LocalizePath(VfsPath sourcePath);

private:

	/**
	 * Dictionary that contains the translations for the
	 * @link L10n::GetCurrentLocale() current locale@endlink and the matching
	 * English strings, including contexts and plural forms.
	 *
	 * @sa LoadDictionaryForCurrentLocale()
	 */
	tinygettext::Dictionary* dictionary;

	/**
	 * Locale that the game is currently using.
	 *
	 * To get the current locale, use its getter: GetCurrentLocale(). You can
	 * also use GetCurrentLocaleString() to get the locale code of the current
	 * locale.
	 *
	 * To change the value of this variable:
	 *   1. Save a new locale to the game configuration file with SaveLocale().
	 *   2. Reload the translation dictionary with\n
	 *      ReevaluateCurrentLocaleAndReload().
	 */
	Locale currentLocale;

	/**
	 * Vector with the locales that the game supports.
	 *
	 * The list of available locales is calculated when the game starts. Call
	 * LoadListOfAvailableLocales() to refresh the list.
	 *
	 * @sa GetSupportedLocaleBaseNames()
	 * @sa GetSupportedLocaleDisplayNames()
	 */
	std::vector<Locale*> availableLocales;

	/**
	 * Whether the game is using the default game locale (@c true), ‘en_US’, or
	 * not (@c false).
	 *
	 * This variable is used in the L10n implementation for performance reasons.
	 * Many localization steps can be skipped when this variable is @c true.
	 */
	bool currentLocaleIsOriginalGameLocale;

	/**
	 * Whether the game is using the special game locale with the longest
	 * strings of each translation (@c true) or not (@c false).
	 *
	 * @sa http://trac.wildfiregames.com/wiki/Implementation_of_Internationalization_and_Localization#LongStringsLocale
	 */
	bool useLongStrings;

	/**
	 * Loads the translation files for the
	 * @link L10n::GetCurrentLocale() current locale@endlink.
	 *
	 * This method loads every file in the ‘l10n’ folder of the game virtual
	 * filesystem that is prefixed with the code of the current locale followed
	 * by a dot.
	 *
	 * For example, if the code of the current locale code is ‘de’,
	 * LoadDictionaryForCurrentLocale() loads the ‘l10n/de.engine.po’ and
	 * ‘l10n/de.public.po’ translation files.
	 *
	 * @sa dictionary
	 * @sa ReadPoIntoDictionary()
	 */
	void LoadDictionaryForCurrentLocale();

	/**
	 * Determines the list of locales that the game supports.
	 *
	 * LoadListOfAvailableLocales() checks the locale codes of the translation
	 * files in the ‘l10n’ folder of the virtual filesystem. If it finds a
	 * translation file prefixed with a locale code followed by a dot, it
	 * determines that the game supports that locale.
	 *
	 * @sa availableLocales
	 */
	void LoadListOfAvailableLocales();

	/**
	 * Loads the specified content of a PO file into the specified dictionary.
	 *
	 * Used by LoadDictionaryForCurrentLocale() to add entries to the game
	 * translations @link dictionary.
	 *
	 * @param poContent Content of a PO file as a string.
	 * @param dictionary Dictionary where the entries from the PO file should be
	 *        stored.
	 */
	void ReadPoIntoDictionary(const std::string& poContent, tinygettext::Dictionary* dictionary);

	/**
	 * Creates an ICU date formatted with the specified settings.
	 *
	 * @param type Whether formatted dates must show both the date and the time,
	 *        only the date or only the time.
	 * @param style ICU style to format dates by default.
	 * @param locale Locale that the date formatter should use to parse strings.
	 *        It has no relevance for date formatting, only matters for date
	 *        parsing.
	 * @return ICU date formatter.
	 */
	DateFormat* CreateDateTimeInstance(DateTimeType type, DateFormat::EStyle style, const Locale& locale);
};

#endif // L10N_H
