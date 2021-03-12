/* Copyright (C) 2021 Wildfire Games.
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

#include "precompiled.h"

#include "i18n/L10n.h"

#include "gui/GUIManager.h"
#include "lib/external_libraries/tinygettext.h"
#include "lib/file/file_system.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/GameSetup.h"

#include <boost/algorithm/string.hpp>
#include <boost/concept_check.hpp>
#include <sstream>
#include <string>

namespace
{

Status ReloadChangedFileCB(void* param, const VfsPath& path)
{
	return static_cast<L10n*>(param)->ReloadChangedFile(path);
}

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
void ReadPoIntoDictionary(const std::string& poContent, tinygettext::Dictionary* dictionary)
{
	try
	{
		std::istringstream inputStream(poContent);
		tinygettext::POParser::parse("virtual PO file", inputStream, *dictionary);
	}
	catch (std::exception& e)
	{
		LOGERROR("[Localization] Exception while reading virtual PO file: %s", e.what());
	}
}

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
icu::DateFormat* CreateDateTimeInstance(const L10n::DateTimeType& type, const icu::DateFormat::EStyle& style, const icu::Locale& locale)
{
	switch (type)
	{
	case L10n::Date:
		return icu::SimpleDateFormat::createDateInstance(style, locale);

	case L10n::Time:
		return icu::SimpleDateFormat::createTimeInstance(style, locale);

	case L10n::DateTime:
	default:
		return icu::SimpleDateFormat::createDateTimeInstance(style, style, locale);
	}
}

} // anonymous namespace

void L10n::DictionaryDeleter::operator()(tinygettext::Dictionary* dictionary)
{
	delete dictionary;
}

L10n::L10n()
	: m_Dictionary(new tinygettext::Dictionary()), currentLocaleIsOriginalGameLocale(false), useLongStrings(false)
{
	// Determine whether or not to print tinygettext messages to the standard
	// error output, which it tinygettext's default behavior, but not ours.
	bool tinygettext_debug = false;
	CFG_GET_VAL("tinygettext.debug", tinygettext_debug);
	if (!tinygettext_debug)
	{
		tinygettext::Log::log_info_callback = 0;
		tinygettext::Log::log_warning_callback = 0;
		tinygettext::Log::log_error_callback = 0;
	}

	LoadListOfAvailableLocales();
	ReevaluateCurrentLocaleAndReload();

	// Handle hotloading
	RegisterFileReloadFunc(ReloadChangedFileCB, this);
}

L10n::~L10n()
{
	UnregisterFileReloadFunc(ReloadChangedFileCB, this);
}

icu::Locale L10n::GetCurrentLocale() const
{
	return currentLocale;
}

bool L10n::SaveLocale(const std::string& localeCode) const
{
	if (localeCode == "long" && InDevelopmentCopy())
	{
		g_ConfigDB.SetValueString(CFG_USER, "locale", "long");
		return true;
	}
	return SaveLocale(icu::Locale(icu::Locale::createCanonical(localeCode.c_str())));
}

bool L10n::SaveLocale(const icu::Locale& locale) const
{
	if (!ValidateLocale(locale))
		return false;

	g_ConfigDB.SetValueString(CFG_USER, "locale", locale.getName());
	return g_ConfigDB.WriteValueToFile(CFG_USER, "locale", locale.getName());
}

bool L10n::ValidateLocale(const std::string& localeCode) const
{
	return ValidateLocale(icu::Locale::createCanonical(localeCode.c_str()));
}

// Returns true if both of these conditions are true:
//  1. ICU has resources for that locale (which also ensures it's a valid locale string)
//  2. Either a dictionary for language_country or for language is available.
bool L10n::ValidateLocale(const icu::Locale& locale) const
{
	if (locale.isBogus())
		return false;

	return !GetFallbackToAvailableDictLocale(locale).empty();
}

std::vector<std::wstring> L10n::GetDictionariesForLocale(const std::string& locale) const
{
	std::vector<std::wstring> ret;
	VfsPaths filenames;

	std::wstring dictName = GetFallbackToAvailableDictLocale(icu::Locale::createCanonical(locale.c_str()));
	vfs::GetPathnames(g_VFS, L"l10n/", dictName.append(L".*.po").c_str(), filenames);

	for (const VfsPath& path : filenames)
		ret.push_back(path.Filename().string());

	return ret;
}

std::wstring L10n::GetFallbackToAvailableDictLocale(const std::string& locale) const
{
	return GetFallbackToAvailableDictLocale(icu::Locale::createCanonical(locale.c_str()));
}

std::wstring L10n::GetFallbackToAvailableDictLocale(const icu::Locale& locale) const
{
	std::wstringstream stream;

	auto checkLangAndCountry = [&locale](const std::unique_ptr<icu::Locale>& l) {
		return strcmp(locale.getLanguage(), l->getLanguage()) == 0
		       && strcmp(locale.getCountry(), l->getCountry()) == 0;
	};

	if (strcmp(locale.getCountry(), "") != 0
	    && std::find_if(availableLocales.begin(), availableLocales.end(), checkLangAndCountry) != availableLocales.end())
	{
		stream << locale.getLanguage() << L"_" << locale.getCountry();
		return stream.str();
	}

	auto checkLang = [&locale](const std::unique_ptr<icu::Locale>& l) {
		return strcmp(locale.getLanguage(), l->getLanguage()) == 0;
	};

	if (std::find_if(availableLocales.begin(), availableLocales.end(), checkLang) != availableLocales.end())
	{
		stream << locale.getLanguage();
		return stream.str();
	}

	return L"";
}

std::string L10n::GetDictionaryLocale(const std::string& configLocaleString) const
{
	icu::Locale out;
	GetDictionaryLocale(configLocaleString, out);
	return out.getName();
}

// First, try to get a valid locale from the config, then check if the system locale can be used and otherwise fall back to en_US.
void L10n::GetDictionaryLocale(const std::string& configLocaleString, icu::Locale& outLocale) const
{
	if (!configLocaleString.empty())
	{
		icu::Locale configLocale = icu::Locale::createCanonical(configLocaleString.c_str());
		if (ValidateLocale(configLocale))
		{
			outLocale = configLocale;
			return;
		}
		else
			LOGWARNING("The configured locale is not valid or no translations are available. Falling back to another locale.");
	}

	icu::Locale systemLocale = icu::Locale::getDefault();
	if (ValidateLocale(systemLocale))
		outLocale = systemLocale;
	else
		outLocale = icu::Locale::getUS();
}

// Try to find the best dictionary locale based on user configuration and system locale, set the currentLocale and reload the dictionary.
void L10n::ReevaluateCurrentLocaleAndReload()
{
	std::string locale;
	CFG_GET_VAL("locale", locale);

	if (locale == "long")
	{
		// Set ICU to en_US to have a valid language for displaying dates
		currentLocale = icu::Locale::getUS();
		currentLocaleIsOriginalGameLocale = false;
		useLongStrings = true;
	}
	else
	{
		GetDictionaryLocale(locale, currentLocale);
		currentLocaleIsOriginalGameLocale = (currentLocale == icu::Locale::getUS()) == 1;
		useLongStrings = false;
	}
	LoadDictionaryForCurrentLocale();
}

// Get all locales supported by ICU.
std::vector<std::string> L10n::GetAllLocales() const
{
	std::vector<std::string> ret;
	int32_t count;
	const icu::Locale* icuSupportedLocales = icu::Locale::getAvailableLocales(count);
	for (int i=0; i<count; ++i)
		ret.push_back(icuSupportedLocales[i].getName());
	return ret;
}


bool L10n::UseLongStrings() const
{
	return useLongStrings;
};

std::vector<std::string> L10n::GetSupportedLocaleBaseNames() const
{
	std::vector<std::string> supportedLocaleCodes;
	for (const std::unique_ptr<icu::Locale>& locale : availableLocales)
	{
		if (!InDevelopmentCopy() && strcmp(locale->getBaseName(), "long") == 0)
			continue;
		supportedLocaleCodes.push_back(locale->getBaseName());
	}
	return supportedLocaleCodes;
}

std::vector<std::wstring> L10n::GetSupportedLocaleDisplayNames() const
{
	std::vector<std::wstring> supportedLocaleDisplayNames;
	for (const std::unique_ptr<icu::Locale>& locale : availableLocales)
	{
		if (strcmp(locale->getBaseName(), "long") == 0)
		{
			if (InDevelopmentCopy())
				supportedLocaleDisplayNames.push_back(wstring_from_utf8(Translate("Long strings")));
			continue;
		}

		icu::UnicodeString utf16LocaleDisplayName;
		locale->getDisplayName(*locale, utf16LocaleDisplayName);
		char localeDisplayName[512];
		icu::CheckedArrayByteSink sink(localeDisplayName, ARRAY_SIZE(localeDisplayName));
		utf16LocaleDisplayName.toUTF8(sink);
		ENSURE(!sink.Overflowed());

		supportedLocaleDisplayNames.push_back(wstring_from_utf8(std::string(localeDisplayName, sink.NumberOfBytesWritten())));
	}
	return supportedLocaleDisplayNames;
}

std::string L10n::GetCurrentLocaleString() const
{
	return currentLocale.getName();
}

std::string L10n::GetLocaleLanguage(const std::string& locale) const
{
	icu::Locale loc = icu::Locale::createCanonical(locale.c_str());
	return loc.getLanguage();
}

std::string L10n::GetLocaleBaseName(const std::string& locale) const
{
	icu::Locale loc = icu::Locale::createCanonical(locale.c_str());
	return loc.getBaseName();
}

std::string L10n::GetLocaleCountry(const std::string& locale) const
{
	icu::Locale loc = icu::Locale::createCanonical(locale.c_str());
	return loc.getCountry();
}

std::string L10n::GetLocaleScript(const std::string& locale) const
{
	icu::Locale loc = icu::Locale::createCanonical(locale.c_str());
	return loc.getScript();
}

std::string L10n::Translate(const std::string& sourceString) const
{
	if (!currentLocaleIsOriginalGameLocale)
		return m_Dictionary->translate(sourceString);

	return sourceString;
}

std::string L10n::TranslateWithContext(const std::string& context, const std::string& sourceString) const
{
	if (!currentLocaleIsOriginalGameLocale)
		return m_Dictionary->translate_ctxt(context, sourceString);

	return sourceString;
}

std::string L10n::TranslatePlural(const std::string& singularSourceString, const std::string& pluralSourceString, int number) const
{
	if (!currentLocaleIsOriginalGameLocale)
		return m_Dictionary->translate_plural(singularSourceString, pluralSourceString, number);

	if (number == 1)
		return singularSourceString;

	return pluralSourceString;
}

std::string L10n::TranslatePluralWithContext(const std::string& context, const std::string& singularSourceString, const std::string& pluralSourceString, int number) const
{
	if (!currentLocaleIsOriginalGameLocale)
		return m_Dictionary->translate_ctxt_plural(context, singularSourceString, pluralSourceString, number);

	if (number == 1)
		return singularSourceString;

	return pluralSourceString;
}

std::string L10n::TranslateLines(const std::string& sourceString) const
{
	std::string targetString;
	std::stringstream stringOfLines(sourceString);
	std::string line;

	while (std::getline(stringOfLines, line))
	{
		if (!line.empty())
			targetString.append(Translate(line));
		targetString.append("\n");
	}

	return targetString;
}

UDate L10n::ParseDateTime(const std::string& dateTimeString, const std::string& dateTimeFormat, const icu::Locale& locale) const
{
	UErrorCode success = U_ZERO_ERROR;
	icu::UnicodeString utf16DateTimeString = icu::UnicodeString::fromUTF8(dateTimeString.c_str());
	icu::UnicodeString utf16DateTimeFormat = icu::UnicodeString::fromUTF8(dateTimeFormat.c_str());

	icu::DateFormat* dateFormatter = new icu::SimpleDateFormat(utf16DateTimeFormat, locale, success);
	UDate date = dateFormatter->parse(utf16DateTimeString, success);
	delete dateFormatter;

	return date;
}

std::string L10n::LocalizeDateTime(const UDate dateTime, const DateTimeType& type, const icu::DateFormat::EStyle& style) const
{
	icu::UnicodeString utf16Date;

	icu::DateFormat* dateFormatter = CreateDateTimeInstance(type, style, currentLocale);
	dateFormatter->format(dateTime, utf16Date);
	char utf8Date[512];
	icu::CheckedArrayByteSink sink(utf8Date, ARRAY_SIZE(utf8Date));
	utf16Date.toUTF8(sink);
	ENSURE(!sink.Overflowed());
	delete dateFormatter;

	return std::string(utf8Date, sink.NumberOfBytesWritten());
}

std::string L10n::FormatMillisecondsIntoDateString(const UDate milliseconds, const std::string& formatString, bool useLocalTimezone) const
{
	UErrorCode status = U_ZERO_ERROR;
	icu::UnicodeString dateString;
	std::string resultString;

	icu::UnicodeString unicodeFormat = icu::UnicodeString::fromUTF8(formatString.c_str());
	icu::SimpleDateFormat* dateFormat = new icu::SimpleDateFormat(unicodeFormat, status);
	if (U_FAILURE(status))
		LOGERROR("Error creating SimpleDateFormat: %s", u_errorName(status));

	status = U_ZERO_ERROR;
	icu::Calendar* calendar = useLocalTimezone ?
		icu::Calendar::createInstance(currentLocale, status) :
		icu::Calendar::createInstance(*icu::TimeZone::getGMT(), currentLocale, status);

	if (U_FAILURE(status))
		LOGERROR("Error creating calendar: %s", u_errorName(status));

	dateFormat->adoptCalendar(calendar);
	dateFormat->format(milliseconds, dateString);
	delete dateFormat;

	dateString.toUTF8String(resultString);
	return resultString;
}

std::string L10n::FormatDecimalNumberIntoString(double number) const
{
	UErrorCode success = U_ZERO_ERROR;
	icu::UnicodeString utf16Number;
	icu::NumberFormat* numberFormatter = icu::NumberFormat::createInstance(currentLocale, UNUM_DECIMAL, success);
	numberFormatter->format(number, utf16Number);
	char utf8Number[512];
	icu::CheckedArrayByteSink sink(utf8Number, ARRAY_SIZE(utf8Number));
	utf16Number.toUTF8(sink);
	ENSURE(!sink.Overflowed());

	return std::string(utf8Number, sink.NumberOfBytesWritten());
}

VfsPath L10n::LocalizePath(const VfsPath& sourcePath) const
{
	VfsPath localizedPath = sourcePath.Parent() / L"l10n" / wstring_from_utf8(currentLocale.getLanguage()) / sourcePath.Filename();
	if (!VfsFileExists(localizedPath))
		return sourcePath;

	return localizedPath;
}

Status L10n::ReloadChangedFile(const VfsPath& path)
{
	if (!boost::algorithm::starts_with(path.string(), L"l10n/"))
		return INFO::OK;

	if (path.Extension() != L".po")
		return INFO::OK;

	// If the file was deleted, ignore it
	if (!VfsFileExists(path))
		return INFO::OK;

	std::wstring dictName = GetFallbackToAvailableDictLocale(currentLocale);
	if (useLongStrings)
		dictName = L"long";
	if (dictName.empty())
		return INFO::OK;

	// Only the currently used language is loaded, so ignore all others
	if (path.string().rfind(dictName) == std::string::npos)
		return INFO::OK;

	LOGMESSAGE("Hotloading translations from '%s'", path.string8());

	CVFSFile file;
	if (file.Load(g_VFS, path) != PSRETURN_OK)
	{
		LOGERROR("Failed to read translations from '%s'", path.string8());
		return ERR::FAIL;
	}

	std::string content = file.DecodeUTF8();
	ReadPoIntoDictionary(content, m_Dictionary.get());

	if (g_GUI)
		g_GUI->ReloadAllPages();

	return INFO::OK;
}

void L10n::LoadDictionaryForCurrentLocale()
{
	m_Dictionary.reset(new tinygettext::Dictionary());
	VfsPaths filenames;

	if (useLongStrings)
	{
		if (vfs::GetPathnames(g_VFS, L"l10n/", L"long.*.po", filenames) < 0)
			return;
	}
	else
	{
		std::wstring dictName = GetFallbackToAvailableDictLocale(currentLocale);
		if (vfs::GetPathnames(g_VFS, L"l10n/", dictName.append(L".*.po").c_str(), filenames) < 0)
		{
			LOGERROR("No files for the dictionary found, but at this point the input should already be validated!");
			return;
		}
	}

	for (const VfsPath& path : filenames)
	{
		CVFSFile file;
		file.Load(g_VFS, path);
		std::string content = file.DecodeUTF8();
		ReadPoIntoDictionary(content, m_Dictionary.get());
	}
}

void L10n::LoadListOfAvailableLocales()
{
	availableLocales.clear();

	// US is always available.
	availableLocales.emplace_back(new icu::Locale(icu::Locale::getUS()));

	VfsPaths filenames;
	if (vfs::GetPathnames(g_VFS, L"l10n/", L"*.po", filenames) < 0)
		return;

	for (const VfsPath& path : filenames)
	{
		// Note: PO files follow this naming convention: "l10n/<locale code>.<mod name>.po". For example: "l10n/gl.public.po".
		std::string filename = utf8_from_wstring(path.string()).substr(strlen("l10n/"));
		size_t lengthToFirstDot = filename.find('.');
		std::string localeCode = filename.substr(0, lengthToFirstDot);
		std::unique_ptr<icu::Locale> locale = std::make_unique<icu::Locale>(icu::Locale::createCanonical(localeCode.c_str()));
		auto it = std::find_if(availableLocales.begin(), availableLocales.end(), [&locale](const std::unique_ptr<icu::Locale>& l) {
			return *locale == *l;
		});
		if (it != availableLocales.end())
			continue;

		availableLocales.push_back(std::move(locale));
	}
}
