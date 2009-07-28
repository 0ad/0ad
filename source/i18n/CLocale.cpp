/* Copyright (C) 2009 Wildfire Games.
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

#include "CLocale.h"
#include "TSComponent.h"
#include "ps/StringConvert.h"


#include <algorithm>

#include "ps/CLogger.h"
#define LOG_CATEGORY "i18n"

using namespace I18n;

// Vaguely useful utility function for deleting stuff
template<typename T> void delete_fn(T* v) { delete v; }

// These could be optimised for little-endian sizeof(wchar_t)==2 systems:
static inline void ReadWString8_(const char*& data, Str& str)
{
	u8 length = *(u8*)data;
	data += 1;
	StringConvert::ucs2le_to_wstring(data, data+length, str);
	data += length;
}
static inline void ReadWString16_(const char*& data, Str& str)
{
	u16 length = *(u16*)data;
	data += 2;
	StringConvert::ucs2le_to_wstring(data, data+length, str);
	data += length;
}

#define ReadWString8(s) Str s; ReadWString8_(data, s);
#define ReadWString16(s) Str s; ReadWString16_(data, s);


bool CLocale::LoadStrings(const char* data)
{
	// TODO: More robust file format (so errors can be detected in a
	// nicer way than watching for access violations)

	u16 PhraseCount = *(u16*)data;
	data += 2;

	for (int i = 0; i < PhraseCount; ++i)
	{
		ReadWString16(Key);

		u8 VarCount = *(u8*)data;
		data += 1;

		// Get the relevant entry in the string hash, creating it if it doesn't exist
		TranslatedString* String = Strings[Key];
		if (! String)
			String = Strings[Key] = new TranslatedString;
		// If this is a redefined string, make sure it's empty
		String->Parts.clear();
		// Store the number of variables, so translate(x)<<y<<z can check for validity
		String->VarCount = VarCount;

		u8 SectionCount = *(u8*)data;
		data += 1;

		for (int j = 0; j < SectionCount; ++j)
		{
			u8 SectionType = *(u8*)data;
			data += 1;

			switch (SectionType)
			{
			case 0: // Constant string
				{
					ReadWString16(StringText);

					String->Parts.push_back(new TSComponentString(StringText.c_str()));
					break;
				}

			case 1: // Variable
				{
					u8 VarID = *(u8*)data;
					data += 1;

					String->Parts.push_back(new TSComponentVariable(VarID));

					break;
				}

			case 2: // Function
				{
					u8 NameLength = *(u8*)data;
					data += 1;

					std::string NameText ((const char*)data, (const char*)(data + NameLength));
					data += NameLength;

					u8 ParamCount = *(u8*)data;
					data += 1;

					TSComponentFunction* Func = new TSComponentFunction(NameText.c_str());

					for (int k = 0; k < ParamCount; ++k)
					{
						u8 ParamType = *(u8*)data;
						data += 1;

						switch (ParamType)
						{
						case 0: // String
							{
								ReadWString8(StrText);

								Func->AddParam(new ScriptValueString(Script, StrText.c_str()));

								break;
							}

						case 1: // Variable
							{
								u8 ID = *(u8*)data;
								data += 1;

								Func->AddParam(new ScriptValueVariable(Script, ID));

								break;
							}

						case 2: // Integer
							{
								u32 Num = *(u32*)data;
								data += 4;

								Func->AddParam(new ScriptValueInteger(Script, Num));

								break;
							}

						default: // Argh!
							debug_warn("Invalid function parameter type");
						}
					}

					String->Parts.push_back(Func);

					break;
				}

			default: // Argh!
				debug_warn("Invalid translation string section type");
			}
		}
	}

	return true;
}


bool CLocale::LoadFunctions(const char* data, size_t len, const char* filename)
{
	// Insist on little-endian UTF16 files containing a BOM (e.g. as generated
	// by Notepad when saving in Unicode format)

	// TODO: Support more Unicode file formats

	if (len < 2)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "I18n: Functions file '%s' is too short", filename);
		return false;
	}

	if (*(jschar*)data != 0xFEFF)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "I18n: Functions file '%s' has invalid Unicode format (lacking little-endian BOM)", filename);
		return false;
	}

	if (! Script.ExecuteCode((jschar*)(data+2), len/2, filename))
	{
		LOG(CLogger::Error, LOG_CATEGORY, "I18n: JS errors in functions file '%s'", filename);
		return false;
	}

	return true;
}


bool CLocale::LoadDictionary(const char* data)
{
	ReadWString8(DictName);

	u8 PropertyCount = *(u8*)data;
	data += 1;

	DictData& dict = Dictionaries[DictName];

	if (dict.DictProperties.size() && PropertyCount != dict.DictProperties.size())
	{
		LOG(CLogger::Error, LOG_CATEGORY, "I18n: Multiple dictionary files loaded with the name ('%ls') and different properties", DictName.c_str());
		return false;
		// TODO: Check headings to make sure they're identical (or handle them more cleverly)
	}

	// Read the names of the properties

	int i;

	for (i = 0; i < PropertyCount; ++i)
	{
		ReadWString8(Property);
		dict.DictProperties[Property] = i;
	}

	u16 ValueCount = *(u16*)data;
	data += 2;

	// Read each 'value' (word + properties)

	for (i = 0; i < ValueCount; ++i)
	{
		ReadWString8(Word);

		std::vector<std::wstring>& props = dict.DictWords[Word];

		for (int j = 0; j < PropertyCount; ++j)
		{
			ReadWString8(Value);
			props.push_back(Value);
		}
	}

	return true;
}

void CLocale::UnloadDictionaries()
{
	Dictionaries.clear();
}


const CLocale::LookupType* CLocale::LookupWord(const Str& dictname, const Str& word)
{
	std::map<Str, DictData>::const_iterator dictit = Dictionaries.find(dictname);
	if (dictit == Dictionaries.end())
	{
		LOG(CLogger::Warning, LOG_CATEGORY, "I18n: Non-loaded dictionary '%ls' accessed", dictname.c_str());
		return NULL;
	}
	std::map<Str, std::vector<Str> >::const_iterator wordit = dictit->second.DictWords.find(word);
	if (wordit == dictit->second.DictWords.end())
	{
		// Word not found. Respond quietly, so JS code can handle missing
		// words in a more appropriate way.
		return NULL;
	}

	// Return some data that can later be passed to LookupProperty
	return new LookupType(&dictit->second, &wordit->second);
}

bool CLocale::LookupProperty(const LookupType* data, const Str& property, Str& result)
{
	std::map<Str, int>::const_iterator propit = data->first->DictProperties.find(property);
	if (propit == data->first->DictProperties.end())
		return false;

	// Return the appropriate string
	result = (*data->second)[propit->second];
	return true;
}




const StrImW CLocale::CallFunction(const char* name, const std::vector<BufferVariable*>& vars, const std::vector<ScriptValue*>& params)
{
	return Script.CallFunction(name, vars, params);
}



StringBuffer CLocale::Translate(const wchar_t* id)
{
	if (++CacheAge > CacheAgeLimit)
	{
		CacheAge = 0;
		ClearCache();
	}

	StringsType::iterator TransStr = Strings.find(Str(id));
	if (TransStr == Strings.end())
	{
		LOG(CLogger::Normal, LOG_CATEGORY, "I18n: No translation found for string '%ls'", id);

		// Just use the ID string directly, and remember it for the future
		return StringBuffer(&AddDefaultString(id), this);
	}

	return StringBuffer((*TransStr).second, this);
}


void CLocale::AddToCache(StringBuffer* sb, Str& str)
{
	CacheData& d = TranslationCache[sb->String];

	// Clean up any earlier cache entry
	std::for_each(d.vars.begin(), d.vars.end(), delete_fn<BufferVariable>);

	// Set the data for the new cache entry
	d.hash = sb->Hash();
	d.vars = sb->Variables;
	d.output = str;
}

bool CLocale::ReadCached(StringBuffer* sb, Str& str)
{
	// Look for a string with the right key in the cache
	std::map<TranslatedString*, CacheData>::iterator it = 
		TranslationCache.find(sb->String);

	// See if it actually exists
	if (it == TranslationCache.end())
		return false;

	// Check quickly whether the hashes match
	if (sb->Hash() != (*it).second.hash)
		return false;

	// Check every variable to see whether they're identical
	debug_assert(sb->Variables.size() == (*it).second.vars.size()); // this should always be true
	size_t count = sb->Variables.size();
	for (size_t i = 0; i < count; ++i)
		if (! ( *sb->Variables[i] == *(*it).second.vars[i] ) )
			return false;

	str = (*it).second.output;
	return true;
}

void CLocale::ClearCache()
{
	// Deallocate cached data
	for (std::map<TranslatedString*, CacheData>::iterator it = TranslationCache.begin(); it != TranslationCache.end(); ++it)
		std::for_each((*it).second.vars.begin(), (*it).second.vars.end(), delete_fn<BufferVariable>);

	TranslationCache.clear();
}


bool is_valid_variable_char(wchar_t c)
{
	// c =~ /[a-zA-Z0-9_]/
	// (Hurrah for internationalisation.)
	return
		(c >= 'a' && c <= 'z')
	||	(c >= 'A' && c <= 'Z')
	||	(c >= '0' && c <= '9')
	||	(c == '_');
}

TranslatedString& CLocale::AddDefaultString(const wchar_t* id)
{
	// Parse a string involving $variables and $$ (=$)

	enum ParseState {
		st_default,
		st_afterdollar,
		st_variable
	};
	ParseState state = st_default;

	TranslatedString* str = new TranslatedString;
	str->VarCount = 0;

	std::wstring tempstr;

	for (const wchar_t* ch = id; *ch != '\0'; ++ch)
	{
		switch (state)
		{
		case st_default:
			if (*ch == '$')
			{
				state = st_afterdollar;
			}
			else
			{
				tempstr += *ch;
			}

			break;
		case st_afterdollar:
			if (*ch == '$')
			{
				tempstr += '$';
				state = st_default;
			}
			else
			{
				// Start of a variable name.
				// Push the old string onto the component stack
				if (tempstr.length())
				{
					str->Parts.push_back(new TSComponentString(tempstr.c_str()));
					tempstr.clear();
				}

				// Set the ID (starting at 0) and increment the count
				str->Parts.push_back(new TSComponentVariable(str->VarCount++));
				
				state = st_variable;
			}
			break;
		case st_variable:
			if (*ch == '$')
			{
				state = st_afterdollar;
			}
			else if (! is_valid_variable_char(*ch))
			{
				state = st_default;
				tempstr = *ch;
			}
			// We don't care about the actual name of the variable, so just ignore it.

			break;
		}
	}
	// Make sure the last string is added to the parts list
	if (tempstr.length())
		str->Parts.push_back(new TSComponentString(tempstr.c_str()));

	Strings[id] = str;

	return *str;
}


CLocale::~CLocale()
{
	// Clean up the list of strings
	for (StringsType::iterator it = Strings.begin(); it != Strings.end(); ++it)
		delete (*it).second;

	ClearCache();
}
