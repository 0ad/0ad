/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_JSI_CONFIGDB
#define INCLUDED_JSI_CONFIGDB

#include "ps/ConfigDB.h"
#include "scriptinterface/ScriptInterface.h"

#include <string>

namespace JSI_ConfigDB
{
	bool IsProtectedConfigName(const std::string& name);
	bool GetConfigNamespace(const std::wstring& cfgNsString, EConfigNamespace& cfgNs);
	bool HasChanges(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString);
	bool SetChanges(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString, bool value);
	std::string GetValue(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString, const std::string& name);
	bool CreateValue(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString, const std::string& name, const std::string& value);
 	bool CreateValues(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString, const std::string& name, const std::vector<CStr>& values);
	bool RemoveValue(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString, const std::string& name);
	bool WriteFile(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString, const Path& path);
	bool WriteValueToFile(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString, const std::string& name, const std::string& value, const Path& path);
	void CreateAndWriteValueToFile(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString, const std::string& name, const std::string& value, const Path& path);
	bool Reload(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString);
	bool SetFile(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& cfgNsString, const Path& path);
	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
}

#endif // INCLUDED_JSI_CONFIGDB
