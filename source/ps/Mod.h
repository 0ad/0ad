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

	/**
	 * Parsed mod.json data for C++ usage.
	 * Note that converting to/from JS is lossy.
	 */
	struct ModData
	{
		// 'Folder name' of the mod, e.g. 'public' for the main 0 A.D. mod.
		CStr m_Pathname;
		// "name" property in the mod.json
		CStr m_Name;

		CStr m_Version;
		std::vector<CStr> m_Dependencies;
		// If true, the mod is assumed to be 'GUI-only', i.e. ignored for MP or replay compatibility checks.
		bool m_IgnoreInCompatibilityChecks;

		// For convenience when exporting to JS, keep a record of the full file.
		CStr m_Text;
	};

	const std::vector<CStr>& GetEnabledMods() const;
	const std::vector<CStr>& GetIncompatibleMods() const;
	const std::vector<ModData>& GetAvailableMods() const;

	/**
	 * Enables specified mods (& mods required by the engine).
	 * @param addPublic - if true, enable the public mod.
	 * @return whether the mods were enabled successfully. This can fail if e.g. mods are incompatible.
	 * If true, GetEnabledMods() should be non-empty, GetIncompatibleMods() empty. Otherwise, GetIncompatibleMods() is non-empty.
	 */
	bool EnableMods(const ScriptInterface& scriptInterface, const std::vector<CStr>& mods, const bool addPublic);

	/**
	 * Get data for the given mod.
	 * @param the mod path name (e.g. 'public')
	 * @return the mod data or nullptr if unavailable.
	 * TODO: switch to std::optional or something related.
	 */
	const ModData* GetModData(const CStr& mod) const;

	/**
	 * Get a list of the enabled mod's data (intended for compatibility checks).
	 * "user" mod and "mod" mod are ignored as they are irrelevant for compatibility checks.
	 */
	const std::vector<const Mod::ModData*> GetEnabledModsData() const;

	/**
	 * @return whether the two lists are compatible for replaying / MP play.
	 */
	static bool AreModsPlayCompatible(const std::vector<const Mod::ModData*>& modsA, const std::vector<const Mod::ModData*>& modsB);
private:
	/**
	 * Fetches available mods and stores some metadata about them.
	 * This may open the zipped mod archives, depending on the situation,
	 * and/or try to write files to the user mod folder,
	 * which can be quite slow, so should be run rarely.
	 * TODO: if this did not need the scriptInterface to parse JSON,
	 * we could run it in different contexts and possibly cleaner.
	 */
	void UpdateAvailableMods(const ScriptInterface& scriptInterface);

	/**
	 * Checks a list of @a mods and returns the incompatible mods, if any.
	 */
	std::vector<CStr> CheckForIncompatibleMods(const std::vector<CStr>& mods) const;
	bool CompareVersionStrings(const CStr& required, const CStr& op, const CStr& version) const;

	std::vector<CStr> m_EnabledMods;
	// Of the currently loaded mods, these are the incompatible with the engine and cannot be loaded.
	std::vector<CStr> m_IncompatibleMods;

	std::vector<ModData> m_AvailableMods;
};

#endif // INCLUDED_MOD
