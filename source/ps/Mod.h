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

#ifndef INCLUDED_MOD
#define INCLUDED_MOD

#include "ps/CStr.h"
#include "scriptinterface/ScriptForward.h"

#include <vector>

#define g_Mods (Mod::Instance())

class Mod
{
	friend class TestMod;
public:
	// Singleton-like interface.
	static Mod& Instance();

	JS::Value GetAvailableMods(const ScriptInterface& scriptInterface) const;
	const std::vector<CStr>& GetEnabledMods() const;
	const std::vector<CStr>& GetIncompatibleMods() const;

	/**
	 * Enables specified mods (& mods required by the engine).
	 * @param addPublic - if true, enable the public mod.
	 * @return whether the mods were enabled successfully. This can fail if e.g. mods are incompatible.
	 * If true, GetEnabledMods() should be non-empty, GetIncompatibleMods() empty. Otherwise, GetIncompatibleMods() is non-empty.
	 */
	bool EnableMods(const ScriptInterface& scriptInterface, const std::vector<CStr>& mods, const bool addPublic);

	/**
	 * Get the loaded mods and their version.
	 * "user" mod and "mod" mod are ignored as they are irrelevant for compatibility checks.
	 *
	 * @param scriptInterface the ScriptInterface in which to create the return data.
	 * @return list of loaded mods with the format [[modA, versionA], [modB, versionB], ...]
	 */
	JS::Value GetLoadedModsWithVersions(const ScriptInterface& scriptInterface) const;

	/**
	 * Gets info (version and mods loaded) on the running engine
	 *
	 * @param scriptInterface the ScriptInterface in which to create the return data.
	 * @return list of objects containing data
	 */
	JS::Value GetEngineInfo(const ScriptInterface& scriptInterface) const;

private:
	/**
	 * This reads the version numbers from the launched mods.
	 * It caches the result, since the reading of zip files is slow and
	 * JS pages can request the version numbers too often easily.
	 * Make sure this is called after each MountMods call.
	 */
	void CacheEnabledModVersions(const ScriptInterface& scriptInterface);

	/**
	 * Checks a list of @a mods and returns the incompatible mods, if any.
	 */
	std::vector<CStr> CheckForIncompatibleMods(const ScriptInterface& scriptInterface, const std::vector<CStr>& mods, const JS::RootedValue& availableMods) const;
	bool CompareVersionStrings(const CStr& required, const CStr& op, const CStr& version) const;

	std::vector<CStr> m_ModsLoaded;
	// Of the currently loaded mods, these are the incompatible with the engine and cannot be loaded.
	std::vector<CStr> m_IncompatibleMods;

	std::vector<std::vector<CStr>> m_LoadedModVersions;
};

#endif // INCLUDED_MOD
