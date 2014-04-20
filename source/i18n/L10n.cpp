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

#include "precompiled.h"
#include "i18n/L10n.h"

#include <iostream>
#include <string>
#include <boost/concept_check.hpp>

#include "lib/file/file_system.h"
#include "lib/utf8.h"

#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/GameSetup.h"


L10n& L10n::Instance()
{
	static L10n m_instance;
	return m_instance;
}

L10n::L10n()
	: currentLocaleIsOriginalGameLocale(false), dictionary(new tinygettext::Dictionary())
{
	LoadListOfAvailableLocales();
	ReevaluateCurrentLocaleAndReload();
}

L10n::~L10n()
{
	for (std::vector<Locale*>::iterator iterator = availableLocales.begin(); iterator != availableLocales.end(); ++iterator)
		delete *iterator;
	delete dictionary;
}

Locale L10n::GetCurrentLocale()
{
	return currentLocale;
}

bool L10n::SaveLocale(const std::string& localeCode)
{
	return SaveLocale(Locale(Locale::createCanonical(localeCode.c_str())));
}

bool L10n::SaveLocale(Locale locale)
{	
	// TODO: Use the ConfigDB functions exposed to js to change the config value
	// Save the new locale in the settings file.
	if (!ValidateLocale(locale))
		return false;

	g_ConfigDB.SetValueString(CFG_USER, "locale", locale.getName());
	g_ConfigDB.WriteFile(CFG_USER);
	return true;
}

bool L10n::ValidateLocale(const std::string& localeCode)
{
	return ValidateLocale(Locale::createCanonical(localeCode.c_str()));
}

// Returns true if both of these conditions are true:
//  1. ICU has resources for that locale (which also ensures it's a valid locale string) 
//  2. Either a dictionary for language_country or for language is available.
bool L10n::ValidateLocale(Locale locale)
{
	int32_t count;
	bool icuSupported = false;
	const Locale* icuSupportedLocales = Locale::getAvailableLocales(count);
	for (int i=0; i<count; ++i)
	{
		if (icuSupportedLocales[i] == locale)
		{
			icuSupported = true;
			break;
		}
	}
	if(!icuSupported)
		return false;
	
	for (std::vector<Locale*>::iterator iterator = availableLocales.begin(); iterator != availableLocales.end(); ++iterator)
	{
		if ((strcmp((*iterator)->getLanguage(), locale.getLanguage()) == 0 && strcmp((*iterator)->getCountry(), "") == 0) ||
			(strcmp((*iterator)->getLanguage(), locale.getLanguage()) == 0 && strcmp((*iterator)->getCountry(), locale.getCountry()) == 0))
		{
			return true;
		}
	}
	return false;
}

std::vector<std::wstring> L10n::GetDictionariesForDictLocale(const std::string& locale)
{
	std::wstring tmpLocale(locale.begin(), locale.end());
	std::vector<std::wstring> ret;
	VfsPaths filenames;
	if (vfs::GetPathnames(g_VFS, L"l10n/",  tmpLocale.append(L".*.po").c_str(), filenames) < 0)
		return ret;

	if (filenames.size() == 0)
	{
		Locale tmpLocale1 = Locale::createCanonical(locale.c_str());
		if (vfs::GetPathnames(g_VFS, L"l10n/", wstring_from_utf8(tmpLocale1.getLanguage()).append(L".*.po").c_str(), filenames) < 0)
			return ret;
	}

	for (VfsPaths::iterator it = filenames.begin(); it != filenames.end(); ++it)
	{
		VfsPath filepath = *it;
		ret.push_back(filepath.Filename().string());
	}
	return ret;
}

std::string L10n::GetDictionaryLocale(std::string configLocaleString)
{
	Locale out;
	GetDictionaryLocale(configLocaleString, out);
	return out.getName();
}

// First, try to get a valid locale from the config, then check if the system locale can be used and otherwise fall back to en_US.
void L10n::GetDictionaryLocale(std::string configLocaleString, Locale& outLocale)
{
	bool validConfigLocale = false;
	if (!configLocaleString.empty())
	{
		Locale configLocale = Locale::createCanonical(configLocaleString.c_str());
		if (ValidateLocale(configLocale))
		{
			outLocale = configLocale;
			validConfigLocale = true;
		}
		else
			LOGWARNING(L"The configured locale is not valid or no translations are available. Falling back to another locale.");
	}
	
	if (!validConfigLocale)
	{
		Locale systemLocale = Locale::getDefault();
		if (ValidateLocale(systemLocale))
		{
			outLocale = systemLocale;
		}
		else
		{
			outLocale = Locale::getUS();
		}
	}
}


// Try to find the best disctionary locale based on user configuration and system locale, set the currentLocale and reload the dictionary.
void L10n::ReevaluateCurrentLocaleAndReload()
{
	std::string locale;
	CFG_GET_VAL("locale", String, locale);
	
	GetDictionaryLocale(locale, currentLocale);
	currentLocaleIsOriginalGameLocale = (currentLocale == Locale::getUS()) == TRUE;
	LoadDictionaryForCurrentLocale();
}

// Get all locales supported by ICU.
std::vector<std::string> L10n::GetAllLocales()
{
	std::vector<std::string> ret;
	int32_t count;
	const Locale* icuSupportedLocales = Locale::getAvailableLocales(count);
	for (int i=0; i<count; ++i)
		ret.push_back(icuSupportedLocales[i].getName());
	return ret;
}


std::vector<std::string> L10n::GetSupportedLocaleBaseNames()
{
	std::vector<std::string> supportedLocaleCodes;
	for (std::vector<Locale*>::iterator iterator = availableLocales.begin(); iterator != availableLocales.end(); ++iterator)
	{
		if (!InDevelopmentCopy() && strcmp((*iterator)->getBaseName(), "long") == 0)
			continue;
		supportedLocaleCodes.push_back((*iterator)->getBaseName());
	}
	return supportedLocaleCodes;
}

std::vector<std::wstring> L10n::GetSupportedLocaleDisplayNames()
{
	std::vector<std::wstring> supportedLocaleDisplayNames;
	for (std::vector<Locale*>::iterator iterator = availableLocales.begin(); iterator != availableLocales.end(); ++iterator)
	{
		if (strcmp((*iterator)->getBaseName(), "long") == 0)
		{
			if (InDevelopmentCopy())
				supportedLocaleDisplayNames.push_back(wstring_from_utf8(Translate("Long strings")));
			continue;
		}

		UnicodeString utf16LocaleDisplayName;
		(**iterator).getDisplayName(**iterator, utf16LocaleDisplayName);
		char localeDisplayName[512];
		CheckedArrayByteSink sink(localeDisplayName, ARRAY_SIZE(localeDisplayName));
		utf16LocaleDisplayName.toUTF8(sink);
		ENSURE(!sink.Overflowed());

		supportedLocaleDisplayNames.push_back(wstring_from_utf8(std::string(localeDisplayName, sink.NumberOfBytesWritten())));
	}
	return supportedLocaleDisplayNames;
}

std::string L10n::GetCurrentLocaleString()
{
	return currentLocale.getName();
}

std::string L10n::GetLocaleLanguage(const std::string& locale)
{
	Locale loc = Locale::createCanonical(locale.c_str());
	return loc.getLanguage();
}

std::string L10n::GetLocaleBaseName(const std::string& locale)
{
	Locale loc = Locale::createCanonical(locale.c_str());
	return loc.getBaseName();
}

std::string L10n::GetLocaleCountry(const std::string& locale)
{
	Locale loc = Locale::createCanonical(locale.c_str());
	return loc.getCountry();
}

std::string L10n::GetLocaleScript(const std::string& locale)
{
	Locale loc = Locale::createCanonical(locale.c_str());
	return loc.getScript();
}

std::string L10n::Translate(const std::string& sourceString)
{
	if (!currentLocaleIsOriginalGameLocale)
		return dictionary->translate(sourceString);

	return sourceString;
}

std::string L10n::TranslateWithContext(const std::string& context, const std::string& sourceString)
{
	if (!currentLocaleIsOriginalGameLocale)
		return dictionary->translate_ctxt(context, sourceString);

	return sourceString;
}

std::string L10n::TranslatePlural(const std::string& singularSourceString, const std::string& pluralSourceString, int number)
{
	if (!currentLocaleIsOriginalGameLocale)
		return dictionary->translate_plural(singularSourceString, pluralSourceString, number);

	if (number == 1)
		return singularSourceString;

	return pluralSourceString;
}

std::string L10n::TranslatePluralWithContext(const std::string& context, const std::string& singularSourceString, const std::string& pluralSourceString, int number)
{
	if (!currentLocaleIsOriginalGameLocale)
		return dictionary->translate_ctxt_plural(context, singularSourceString, pluralSourceString, number);

	if (number == 1)
		return singularSourceString;

	return pluralSourceString;
}

std::string L10n::TranslateLines(const std::string& sourceString)
{
	std::string targetString;
	std::stringstream stringOfLines(sourceString);
	std::string line;

	while (std::getline(stringOfLines, line)) {
		targetString.append(Translate(line));
		targetString.append("\n");
	}

	return targetString;
}

UDate L10n::ParseDateTime(const std::string& dateTimeString, const std::string& dateTimeFormat, const Locale& locale)
{
	UErrorCode success = U_ZERO_ERROR;
	UnicodeString utf16DateTimeString = UnicodeString::fromUTF8(dateTimeString.c_str());
	UnicodeString utf16DateTimeFormat = UnicodeString::fromUTF8(dateTimeFormat.c_str());

	DateFormat* dateFormatter = new SimpleDateFormat(utf16DateTimeFormat, locale, success);
	UDate date = dateFormatter->parse(utf16DateTimeString, success);
	delete dateFormatter;

	return date;
}

std::string L10n::LocalizeDateTime(const UDate& dateTime, DateTimeType type, DateFormat::EStyle style)
{
	UnicodeString utf16Date;

	DateFormat* dateFormatter = CreateDateTimeInstance(type, style, currentLocale);
	dateFormatter->format(dateTime, utf16Date);
	char utf8Date[512];
	CheckedArrayByteSink sink(utf8Date, ARRAY_SIZE(utf8Date));
	utf16Date.toUTF8(sink);
	ENSURE(!sink.Overflowed());
	delete dateFormatter;

	return std::string(utf8Date, sink.NumberOfBytesWritten());
}

std::string L10n::FormatMillisecondsIntoDateString(UDate milliseconds, const std::string& formatString)
{
	UErrorCode success = U_ZERO_ERROR;
	UnicodeString utf16Date;
	UnicodeString utf16LocalizedDateTimeFormat = UnicodeString::fromUTF8(formatString.c_str());

	// The format below should never reach the user, the one that matters is the
	// one from the formatString parameter.
	UnicodeString utf16SourceDateTimeFormat = UnicodeString::fromUTF8("No format specified (you should not be seeing this string!)");

	SimpleDateFormat* dateFormatter = new SimpleDateFormat(utf16SourceDateTimeFormat, currentLocale, success);
	dateFormatter->applyLocalizedPattern(utf16LocalizedDateTimeFormat, success);
	dateFormatter->format(milliseconds, utf16Date);
	delete dateFormatter;

	char utf8Date[512];
	CheckedArrayByteSink sink(utf8Date, ARRAY_SIZE(utf8Date));
	utf16Date.toUTF8(sink);
	ENSURE(!sink.Overflowed());

	return std::string(utf8Date, sink.NumberOfBytesWritten());
}

std::string L10n::FormatDecimalNumberIntoString(double number)
{
	UErrorCode success = U_ZERO_ERROR;
	UnicodeString utf16Number;
	NumberFormat* numberFormatter = NumberFormat::createInstance(currentLocale, UNUM_DECIMAL, success);
	numberFormatter->format(number, utf16Number);
	char utf8Number[512];
	CheckedArrayByteSink sink(utf8Number, ARRAY_SIZE(utf8Number));
	utf16Number.toUTF8(sink);
	ENSURE(!sink.Overflowed());

	return std::string(utf8Number, sink.NumberOfBytesWritten());
}

VfsPath L10n::LocalizePath(VfsPath sourcePath)
{
	VfsPath path = sourcePath;

	VfsPath localizedPath = sourcePath.Parent() / L"l10n" / wstring_from_utf8(currentLocale.getLanguage()) / sourcePath.Filename();
	if (VfsFileExists(localizedPath))
		path = localizedPath;

	return path;
}

void L10n::LoadDictionaryForCurrentLocale()
{
	delete dictionary;
	dictionary = new tinygettext::Dictionary();

	VfsPaths filenames;
	if (vfs::GetPathnames(g_VFS, L"l10n/", (wstring_from_utf8(currentLocale.getBaseName()) + L".*.po").c_str(), filenames) < 0)
		return;
	
	// If not matching country is found, try to fall back to a matching language	
	if (filenames.size() == 0)
	{
		if (vfs::GetPathnames(g_VFS, L"l10n/", (wstring_from_utf8(currentLocale.getLanguage()) + L".*.po").c_str(), filenames) < 0)
			return;
	}

	for (VfsPaths::iterator it = filenames.begin(); it != filenames.end(); ++it)
	{
		VfsPath filename = *it;
		CVFSFile file;
		file.Load(g_VFS, filename);
		std::string content = file.DecodeUTF8();
		ReadPoIntoDictionary(content, dictionary);
	}
}

void L10n::LoadListOfAvailableLocales()
{
	for (std::vector<Locale*>::iterator iterator = availableLocales.begin(); iterator != availableLocales.end(); ++iterator)
		delete *iterator;

	availableLocales.clear();

	Locale* defaultLocale = new Locale(Locale::getUS());
	availableLocales.push_back(defaultLocale); // Always available.

	VfsPaths filenames;
	if (vfs::GetPathnames(g_VFS, L"l10n/", L"*.po", filenames) < 0)
		return;

	for (VfsPaths::iterator it = filenames.begin(); it != filenames.end(); ++it)
	{
		// Note: PO files follow this naming convention: “l10n/<locale code>.<mod name>.po”. For example: “l10n/gl.public.po”.
		VfsPath filepath = *it;
		std::string filename = utf8_from_wstring(filepath.string()).substr(strlen("l10n/"));
		std::size_t lengthToFirstDot = filename.find('.');
		std::string localeCode = filename.substr(0, lengthToFirstDot);
		Locale* locale = new Locale(Locale::createCanonical(localeCode.c_str()));

		bool localeIsAlreadyAvailable = false;
		for (std::vector<Locale*>::iterator iterator = availableLocales.begin(); iterator != availableLocales.end(); ++iterator)
		{
			if (*locale == **iterator)
			{
				localeIsAlreadyAvailable = true;
				break;
			}
		}

		if (!localeIsAlreadyAvailable)
			availableLocales.push_back(locale);
		else 
			delete locale;
	}
}

void L10n::ReadPoIntoDictionary(const std::string& poContent, tinygettext::Dictionary* dictionary)
{
	try
	{
		std::istringstream inputStream(poContent);
		tinygettext::POParser::parse("virtual PO file", inputStream, *dictionary, false);
	}
	catch(std::exception& e)
	{
		LOGERROR(L"[Localization] Exception while reading virtual PO file: %hs", e.what());
	}
}

DateFormat* L10n::CreateDateTimeInstance(L10n::DateTimeType type, DateFormat::EStyle style, const Locale& locale)
{
	switch(type)
	{
	case Date:
		return SimpleDateFormat::createDateInstance(style, locale);

	case Time:
		return SimpleDateFormat::createTimeInstance(style, locale);

	case DateTime:
	default:
		return SimpleDateFormat::createDateTimeInstance(style, style, locale);
	}
}
