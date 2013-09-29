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

#ifndef INCLUDED_JSI_CONFIGDB
#define INCLUDED_JSI_CONFIGDB

#include "ps/ConfigDB.h"

class ScriptInterface;

namespace JSI_ConfigDB
{
	bool GetConfigNamespace(std::wstring cfgNsString, EConfigNamespace& cfgNs);
	std::string GetValue(void* cbdata, std::wstring cfgNsString, std::string name);
	bool CreateValue(void* cbdata, std::wstring cfgNsString, std::string name, std::string value);
	bool WriteFile(void* cbdata, std::wstring cfgNsString, Path path);
	bool Reload(void* cbdata, std::wstring cfgNsString);
	bool SetFile(void* cbdata, std::wstring cfgNsString, Path path);
	void RegisterScriptFunctions(ScriptInterface& scriptInterface);
}

#endif
