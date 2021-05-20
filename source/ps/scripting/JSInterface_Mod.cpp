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

#include "JSInterface_Mod.h"

#include "ps/Mod.h"
#include "scriptinterface/FunctionWrapper.h"

extern void RestartEngine();

namespace JSI_Mod
{
Mod* ModGetter(const ScriptRequest&, JS::CallArgs&)
{
	return &g_Mods;
}

bool SetModsAndRestartEngine(const ScriptInterface& scriptInterface, const std::vector<CStr>& mods)
{
	g_Mods.ClearIncompatibleMods();
	if (!g_Mods.CheckAndEnableMods(scriptInterface, mods))
		return false;

	RestartEngine();
	return true;
}

bool HasFailedMods()
{
	return g_Mods.GetFailedMods().size() > 0;
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&Mod::GetEngineInfo, ModGetter>(rq, "GetEngineInfo");
	ScriptFunction::Register<&Mod::GetAvailableMods, ModGetter>(rq, "GetAvailableMods");
	ScriptFunction::Register<&Mod::GetEnabledMods, ModGetter>(rq, "GetEnabledMods");
	ScriptFunction::Register<HasFailedMods> (rq, "HasFailedMods");
	ScriptFunction::Register<&Mod::GetFailedMods, ModGetter>(rq, "GetFailedMods");
	ScriptFunction::Register<&SetModsAndRestartEngine>(rq, "SetModsAndRestartEngine");
}
}
