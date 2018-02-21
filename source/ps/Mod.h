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

#ifndef INCLUDED_MOD
#define INCLUDED_MOD

#include "ps/CStr.h"
#include "ps/GameSetup/CmdLineArgs.h"
#include "scriptinterface/ScriptInterface.h"

extern std::vector<CStr> g_modsLoaded;
extern CmdLineArgs g_args;

namespace Mod
{
	JS::Value GetAvailableMods(const ScriptInterface& scriptInterface);

	/**
	 * Get the loaded mods and their version.
	 * "user" mod and "mod" mod are ignored as they are irrelevant for compatibility checks.
	 *
	 * @param scriptInterface the ScriptInterface in which to create the return data.
	 * @return list of loaded mods with the format [[modA, versionA], [modB, versionB], ...]
	 */
	JS::Value GetLoadedModsWithVersions(const ScriptInterface& scriptInterface);

	/**
	 * Gets info (version and mods loaded) on the running engine
	 *
	 * @param scriptInterface the ScriptInterface in which to create the return data.
	 * @return list of objects containing data
	 */
	JS::Value GetEngineInfo(const ScriptInterface& scriptInterface);
}
#endif // INCLUDED_MOD
