/* Copyright (C) 2013 Wildfire Games.
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

bool JSI_ConfigDB::GetConfigNamespace(std::wstring cfgNsString, EConfigNamespace& cfgNs)
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
		LOGERROR(L"Invalid namespace name passed to the ConfigDB!");
		cfgNs = CFG_DEFAULT;
		return false;		
	}
	return true;
}

std::string JSI_ConfigDB::GetValue(void* UNUSED(cbdata), std::wstring cfgNsString, std::string name)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return std::string();

	CConfigValue *val = g_ConfigDB.GetValue(cfgNs, name);
	if (val)
	{
		return val->m_String;
	}
	else
	{
		LOGMESSAGE(L"Config setting %hs does not exist!", name.c_str());
		return std::string();
	}
}

bool JSI_ConfigDB::CreateValue(void* UNUSED(cbdata), std::wstring cfgNsString, std::string name, std::string value)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	CConfigValue *val = g_ConfigDB.CreateValue(cfgNs, name);
	val->m_String = value;
	return true;
}

bool JSI_ConfigDB::WriteFile(void* UNUSED(cbdata), std::wstring cfgNsString, Path path)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	bool ret = g_ConfigDB.WriteFile(cfgNs, path);
	return ret;
}

bool JSI_ConfigDB::Reload(void* UNUSED(cbdata), std::wstring cfgNsString)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	bool ret = g_ConfigDB.Reload(cfgNs);
	return ret;
}

bool JSI_ConfigDB::SetFile(void* UNUSED(cbdata), std::wstring cfgNsString, Path path)
{
	EConfigNamespace cfgNs;
	if (!GetConfigNamespace(cfgNsString, cfgNs))
		return false;

	g_ConfigDB.SetConfigFile(cfgNs, path);
	return true;
}

void JSI_ConfigDB::RegisterScriptFunctions(ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<std::string, std::wstring, std::string, &JSI_ConfigDB::GetValue>("ConfigDB_GetValue");
	scriptInterface.RegisterFunction<bool, std::wstring, std::string, std::string, &JSI_ConfigDB::CreateValue>("ConfigDB_CreateValue");
	scriptInterface.RegisterFunction<bool, std::wstring, Path, &JSI_ConfigDB::WriteFile>("ConfigDB_WriteFile");
	scriptInterface.RegisterFunction<bool, std::wstring, Path, &JSI_ConfigDB::SetFile>("ConfigDB_SetFile");
	scriptInterface.RegisterFunction<bool, std::wstring, &JSI_ConfigDB::Reload>("ConfigDB_Reload");
	
}
