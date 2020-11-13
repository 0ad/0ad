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

#ifndef INCLUDED_JSI_MOD
#define INCLUDED_JSI_MOD

#include "ps/CStr.h"
#include "scriptinterface/ScriptInterface.h"

namespace JSI_Mod
{
	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);

	JS::Value GetEngineInfo(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value GetAvailableMods(ScriptInterface::CmptPrivate* pCmptPrivate);
	void RestartEngine(ScriptInterface::CmptPrivate* pCmptPrivate);
	void SetMods(ScriptInterface::CmptPrivate* pCmptPrivate, const std::vector<CStr>& mods);
}

#endif // INCLUDED_JSI_MOD
