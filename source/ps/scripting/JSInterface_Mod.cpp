/* Copyright (C) 2018 Wildfire Games.
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

#include "JSInterface_Mod.h"

#include "ps/Mod.h"
#include "scriptinterface/ScriptInterface.h"

extern void RestartEngine();

JS::Value JSI_Mod::GetEngineInfo(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	return Mod::GetEngineInfo(*(pCmptPrivate->pScriptInterface));
}

/**
 * Returns a JS object containing a listing of available mods that
 * have a modname.json file in their modname folder. The returned
 * object looks like { modname1: json1, modname2: json2, ... } where
 * jsonN is the content of the modnameN/modnameN.json file as a JS
 * object.
 *
 * @return JS object with available mods as the keys of the modname.json
 *         properties.
 */
JS::Value JSI_Mod::GetAvailableMods(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	return Mod::GetAvailableMods(*(pCmptPrivate->pScriptInterface));
}

void JSI_Mod::RestartEngine(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	::RestartEngine();
}

void JSI_Mod::SetMods(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::vector<CStr>& mods)
{
	g_modsLoaded = mods;
}

void JSI_Mod::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<JS::Value, &GetEngineInfo>("GetEngineInfo");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Mod::GetAvailableMods>("GetAvailableMods");
	scriptInterface.RegisterFunction<void, &JSI_Mod::RestartEngine>("RestartEngine");
	scriptInterface.RegisterFunction<void, std::vector<CStr>, &JSI_Mod::SetMods>("SetMods");
}
