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

#include "precompiled.h"

#include "JSInterface_ConfigDB.h"

#include "ps/ConfigDB.h"
#include "ps/CLogger.h"
#include "scriptinterface/ScriptInterface.h"

#include <string>
#include <unordered_set>

// These entries will not be readable nor writable for JS, so that malicious mods can't leak personal or sensitive data
static const std::unordered_set<std::string> g_ProtectedConfigNames = {
	"userreport.id" // authentication token for GDPR personal data requests
};

bool JSI_ConfigDB::IsProtectedConfigName(const std::string& name)
{
	if (g_ProtectedConfigNames.find(name) != g_ProtectedConfigNames.end())
	{
		LOGERROR("Access denied (%s)", name.c_str());
		return true;
	}
	return false;
}

bool JSI_ConfigDB::GetConfigNamespace(const std::wstring& cfgNsString, EConfigNamespace& cfgNs)
{
	if (cfgNsString == L"default")
		cfgNs = CFG_DEFAULT;
	else if (cfgNsString == L"system")
		cfgNs = CFG_SYSTEM;
	else if (cfgNsString == L"user")
		cfgNs = CFG_USER;
	else if (cfgNsString == L"mod")
		cfgNs = CFG_MOD;
	else
	{
		LOGERROR("Invalid namespace name passed to the ConfigDB!");
		cfgNs = CFG_DEFAULT;
		return false;
	}
	return true;
}

bool JSI_ConfigDB::HasChanges(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& cfgNsString)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	return g_ConfigDB.HasChanges(cfgNs);
}

bool JSI_ConfigDB::SetChanges(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& cfgNsString, bool value)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	g_ConfigDB.SetChanges(cfgNs, value);
	return true;
}

std::string JSI_ConfigDB::GetValue(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& cfgNsString, const std::string& name)
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

bool JSI_ConfigDB::CreateValue(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& cfgNsString, const std::string& name, const std::string& value)
{
	if (IsProtectedConfigName(name))
		return false;

	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	g_ConfigDB.SetValueString(cfgNs, name, value);
	return true;
}

bool JSI_ConfigDB::RemoveValue(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& cfgNsString, const std::string& name)
{
	if (IsProtectedConfigName(name))
		return false;

	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	g_ConfigDB.RemoveValue(cfgNs, name);
	return true;
}

bool JSI_ConfigDB::WriteFile(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& cfgNsString, const Path& path)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	return g_ConfigDB.WriteFile(cfgNs, path);
}

bool JSI_ConfigDB::WriteValueToFile(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& cfgNsString,  const std::string& name, const std::string& value, const Path& path)
{
	if (IsProtectedConfigName(name))
		return false;

	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	return g_ConfigDB.WriteValueToFile(cfgNs, name, value, path);
}

void JSI_ConfigDB::CreateAndWriteValueToFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& cfgNsString,  const std::string& name, const std::string& value, const Path& path)
{
	CreateValue(pCxPrivate, cfgNsString, name, value);
	WriteValueToFile(pCxPrivate, cfgNsString, name, value, path);
}

bool JSI_ConfigDB::Reload(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& cfgNsString)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	return g_ConfigDB.Reload(cfgNs);
}

bool JSI_ConfigDB::SetFile(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& cfgNsString, const Path& path)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	g_ConfigDB.SetConfigFile(cfgNs, path);
	return true;
}

void JSI_ConfigDB::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<bool, std::wstring, &JSI_ConfigDB::HasChanges>("ConfigDB_HasChanges");
	scriptInterface.RegisterFunction<bool, std::wstring, bool, &JSI_ConfigDB::SetChanges>("ConfigDB_SetChanges");
	scriptInterface.RegisterFunction<std::string, std::wstring, std::string, &JSI_ConfigDB::GetValue>("ConfigDB_GetValue");
	scriptInterface.RegisterFunction<bool, std::wstring, std::string, std::string, &JSI_ConfigDB::CreateValue>("ConfigDB_CreateValue");
	scriptInterface.RegisterFunction<bool, std::wstring, std::string, &JSI_ConfigDB::RemoveValue>("ConfigDB_RemoveValue");
	scriptInterface.RegisterFunction<bool, std::wstring, Path, &JSI_ConfigDB::WriteFile>("ConfigDB_WriteFile");
	scriptInterface.RegisterFunction<bool, std::wstring, std::string, std::string, Path, &JSI_ConfigDB::WriteValueToFile>("ConfigDB_WriteValueToFile");
	scriptInterface.RegisterFunction<void, std::wstring, std::string, std::string, Path, &JSI_ConfigDB::CreateAndWriteValueToFile>("ConfigDB_CreateAndWriteValueToFile");
	scriptInterface.RegisterFunction<bool, std::wstring, Path, &JSI_ConfigDB::SetFile>("ConfigDB_SetFile");
	scriptInterface.RegisterFunction<bool, std::wstring, &JSI_ConfigDB::Reload>("ConfigDB_Reload");
}
