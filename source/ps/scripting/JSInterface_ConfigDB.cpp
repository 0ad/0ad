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

#include "JSInterface_ConfigDB.h"

#include "ps/ConfigDB.h"
#include "ps/CLogger.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptRequest.h"

#include <string>
#include <unordered_set>

namespace JSI_ConfigDB
{
// These entries will not be readable nor writable for JS, so that e.g. malicious mods can't leak personal or sensitive data
static const std::unordered_set<std::string> g_ProtectedConfigNames = {
	"modio.public_key", // See ModIO.cpp
	"modio.v1.baseurl",
	"modio.v1.api_key",
	"modio.v1.name_id",
	"userreport.id" // Acts as authentication token for GDPR personal data requests.
};

bool IsProtectedConfigName(const std::string& name)
{
	if (g_ProtectedConfigNames.find(name) != g_ProtectedConfigNames.end())
	{
		LOGERROR("Access denied (%s)", name.c_str());
		return true;
	}
	return false;
}

bool GetConfigNamespace(const std::wstring& cfgNsString, EConfigNamespace& cfgNs)
{
	if (cfgNsString == L"default")
		cfgNs = CFG_DEFAULT;
	else if (cfgNsString == L"mod")
		cfgNs = CFG_MOD;
	else if (cfgNsString == L"system")
		cfgNs = CFG_SYSTEM;
	else if (cfgNsString == L"user")
		cfgNs = CFG_USER;
	else if (cfgNsString == L"hwdetect")
		cfgNs = CFG_HWDETECT;
	else
	{
		LOGERROR("Invalid namespace name passed to the ConfigDB!");
		cfgNs = CFG_DEFAULT;
		return false;
	}
	return true;
}

bool HasChanges(const std::wstring& cfgNsString)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	return g_ConfigDB.HasChanges(cfgNs);
}

bool SetChanges(const std::wstring& cfgNsString, bool value)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	g_ConfigDB.SetChanges(cfgNs, value);
	return true;
}

std::string GetValue(const std::wstring& cfgNsString, const std::string& name)
{
	if (IsProtectedConfigName(name))
		return "";

	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return std::string();

	std::string value;
	g_ConfigDB.GetValue(cfgNs, name, value);
	return value;
}

bool CreateValue(const std::wstring& cfgNsString, const std::string& name, const std::string& value)
{
	if (IsProtectedConfigName(name))
		return false;

	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	g_ConfigDB.SetValueString(cfgNs, name, value);
	return true;
}

bool CreateValues(const std::wstring& cfgNsString, const std::string& name, const std::vector<CStr>& values)
{
	if (IsProtectedConfigName(name))
		return false;

	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	g_ConfigDB.SetValueList(cfgNs, name, values);
	return true;
}


bool RemoveValue(const std::wstring& cfgNsString, const std::string& name)
{
	if (IsProtectedConfigName(name))
		return false;

	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	g_ConfigDB.RemoveValue(cfgNs, name);
	return true;
}

bool WriteFile(const std::wstring& cfgNsString, const Path& path)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	return g_ConfigDB.WriteFile(cfgNs, path);
}

bool WriteValueToFile(const std::wstring& cfgNsString,  const std::string& name, const std::string& value, const Path& path)
{
	if (IsProtectedConfigName(name))
		return false;

	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	return g_ConfigDB.WriteValueToFile(cfgNs, name, value, path);
}

void CreateAndWriteValueToFile(const std::wstring& cfgNsString,  const std::string& name, const std::string& value, const Path& path)
{
	CreateValue(cfgNsString, name, value);
	WriteValueToFile(cfgNsString, name, value, path);
}

bool Reload(const std::wstring& cfgNsString)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	return g_ConfigDB.Reload(cfgNs);
}

bool SetFile(const std::wstring& cfgNsString, const Path& path)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	g_ConfigDB.SetConfigFile(cfgNs, path);
	return true;
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&HasChanges>(rq, "ConfigDB_HasChanges");
	ScriptFunction::Register<&SetChanges>(rq, "ConfigDB_SetChanges");
	ScriptFunction::Register<&GetValue>(rq, "ConfigDB_GetValue");
	ScriptFunction::Register<&CreateValue>(rq, "ConfigDB_CreateValue");
	ScriptFunction::Register<&CreateValues>(rq, "ConfigDB_CreateValues");
	ScriptFunction::Register<&RemoveValue>(rq, "ConfigDB_RemoveValue");
	ScriptFunction::Register<&WriteFile>(rq, "ConfigDB_WriteFile");
	ScriptFunction::Register<&WriteValueToFile>(rq, "ConfigDB_WriteValueToFile");
	ScriptFunction::Register<&CreateAndWriteValueToFile>(rq, "ConfigDB_CreateAndWriteValueToFile");
	ScriptFunction::Register<&SetFile>(rq, "ConfigDB_SetFile");
	ScriptFunction::Register<&Reload>(rq, "ConfigDB_Reload");
}
}
