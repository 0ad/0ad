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

#include "ps/i18n.h"

#include "lib/wchar.h"
#include "ps/Filesystem.h"
#include "scripting/ScriptingHost.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY L"i18n"

// Yay, global variables. (The user only ever wants to be using one language
// at a time, so this is sufficient)
I18n::CLocale_interface* g_CurrentLocale = NULL;
std::string g_CurrentLocaleName;

bool I18n::LoadLanguage(const char* name)
{
	// Special case: If name==NULL, use an 'empty' locale which should have
	// no external dependencies other than CLogger, so it can be called
	// before SpiderMonkey/etc has been initialised (useful for localised
	// error messages that should fall back to English if the language hasn't
	// been loaded yet)
	if (name == NULL)
	{
		CLocale_interface* locale_ptr = I18n::NewLocale(NULL, NULL);
		debug_assert(locale_ptr);
		delete g_CurrentLocale;
		g_CurrentLocale = locale_ptr;
		g_CurrentLocaleName = "";
		return true;
	}

	CLocale_interface* locale_ptr = I18n::NewLocale(g_ScriptingHost.getContext(), JS_GetGlobalObject(g_ScriptingHost.getContext()));

	if (! locale_ptr)
	{
		debug_warn(L"Failed to create locale");
		return false;
	}

	// Automatically delete the pointer when returning early
	std::auto_ptr<CLocale_interface> locale (locale_ptr);

	VfsPath dirname = AddSlash(VfsPath(L"language")/wstring_from_string(name));

	// Open *.lng with LoadStrings
	VfsPaths pathnames;
	if(fs_util::GetPathnames(g_VFS, dirname, L"*.lng", pathnames) < 0)
		return false;
	for (size_t i = 0; i < pathnames.size(); i++)
	{
		CVFSFile strings;
		if (! (strings.Load(pathnames[i]) == PSRETURN_OK && locale->LoadStrings((const char*)strings.GetBuffer())))
		{
			LOG(CLogger::Error, LOG_CATEGORY, L"Error opening language string file '%ls'", pathnames[i].string().c_str());
			return false;
		}
	}

	// Open *.wrd with LoadDictionary
	if(fs_util::GetPathnames(g_VFS, dirname, L"*.wrd", pathnames) < 0)
		return false;
	for (size_t i = 0; i < pathnames.size(); i++)
	{
		CVFSFile strings;
		if (! (strings.Load(pathnames[i]) == PSRETURN_OK && locale->LoadDictionary((const char*)strings.GetBuffer())))
		{
			LOG(CLogger::Error, LOG_CATEGORY, L"Error opening language string file '%ls'", pathnames[i].string().c_str());
			return false;
		}
	}

	// Open *.js with LoadFunctions
	if(fs_util::GetPathnames(g_VFS, dirname, L"*.js", pathnames) < 0)
		return false;
	for (size_t i = 0; i < pathnames.size(); i++)
	{
		const wchar_t* pathname = pathnames[i].string().c_str();
		CStr8 pathname8(pathname);
		CVFSFile strings;
		if (! (strings.Load(pathname) == PSRETURN_OK
			&& 
			locale->LoadFunctions(
				(const char*)strings.GetBuffer(),
				strings.GetBufferSize(),
				pathname8.c_str()
			)))
		{
			LOG(CLogger::Error, LOG_CATEGORY, L"Error opening language function file '%ls'", pathname);
			return false;
		}
	}

	// Free any previously loaded data
	delete g_CurrentLocale;

	// Store the new CLocale*, and stop the auto_ptr from deleting it
	g_CurrentLocale = locale.release();

	// Remember the name
	g_CurrentLocaleName = name;

	return true;
}

const char* I18n::CurrentLanguageName()
{
	return g_CurrentLocaleName.c_str();
}

void I18n::Shutdown()
{
	delete g_CurrentLocale;
	g_CurrentLocale = NULL;
}
