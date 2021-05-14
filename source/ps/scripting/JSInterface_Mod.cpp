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

namespace
{
bool SetModsAndRestartEngine(ScriptInterface::CmptPrivate* pCmptPrivate, const std::vector<CStr>& mods)
{
	Mod::ClearIncompatibleMods();
	if (!Mod::CheckAndEnableMods(*(pCmptPrivate->pScriptInterface), mods))
		return false;

	RestartEngine();
	return true;
}
}

bool HasFailedMods(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return Mod::GetFailedMods().size() > 0;
}

void JSI_Mod::RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&Mod::GetEngineInfo>(rq, "GetEngineInfo");
	ScriptFunction::Register<&Mod::GetAvailableMods>(rq, "GetAvailableMods");
	ScriptFunction::Register<&Mod::GetEnabledMods>(rq, "GetEnabledMods");
	ScriptFunction::Register<HasFailedMods> (rq, "HasFailedMods");
	ScriptFunction::Register<&Mod::GetFailedMods>(rq, "GetFailedMods");
	ScriptFunction::Register<&SetModsAndRestartEngine>(rq, "SetModsAndRestartEngine");
}
