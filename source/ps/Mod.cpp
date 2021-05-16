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

#include "ps/Mod.h"

#include "i18n/L10n.h"
#include "lib/file/file_system.h"
#include "lib/file/vfs/vfs.h"
#include "lib/utf8.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/Paths.h"
#include "ps/Profiler2.h"
#include "ps/Pyrogenesis.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/JSON.h"

#include <algorithm>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace Mod
{
std::vector<CStr> g_ModsLoaded;
std::vector<CStr> g_IncompatibleMods;
std::vector<CStr> g_FailedMods;

std::vector<std::vector<CStr>> g_LoadedModVersions;

bool ParseModJSON(const ScriptRequest& rq, const PIVFS& vfs, OsPath modsPath, OsPath mod, JS::MutableHandleValue json)
{
	// Attempt to open mod.json first.
	std::ifstream modjson;
	modjson.open((modsPath / mod / L"mod.json").string8());

	if (!modjson.is_open())
	{
		modjson.close();

		// Fallback: open the archive and read mod.json there.
		// This can take in the hundreds of milliseconds with large mods.
		vfs->Clear();
		if (vfs->Mount(L"", modsPath / mod / "", VFS_MOUNT_MUST_EXIST, VFS_MIN_PRIORITY) < 0)
			return false;

		CVFSFile modinfo;
		if (modinfo.Load(vfs, L"mod.json", false) != PSRETURN_OK)
			return false;

		if (!Script::ParseJSON(rq, modinfo.GetAsString(), json))
			return false;

		// Attempt to write the mod.json file so we'll take the fast path next time.
		std::ofstream out_mod_json((modsPath / mod / L"mod.json").string8());
		if (out_mod_json.good())
		{
			out_mod_json << modinfo.GetAsString();
			out_mod_json.close();
		}
		else
		{
			// Print a warning - we'll keep trying, which could have adverse effects.
			if (L10n::IsInitialised())
				LOGWARNING(g_L10n.Translate("Could not write external mod.json for zipped mod '%s'. The mod should be reinstalled."), mod.string8());
			else
				LOGWARNING("Could not write external mod.json for zipped mod '%s'. The mod should be reinstalled.", mod.string8());
		}
		return true;
	}
	else
	{
		std::stringstream buffer;
		buffer << modjson.rdbuf();
		return Script::ParseJSON(rq, buffer.str(), json);
	}
}

JS::Value GetAvailableMods(const ScriptInterface& scriptInterface)
{
	PROFILE2("GetAvailableMods");

	const Paths paths(g_CmdLineArgs);

	// loop over all possible paths
	OsPath modPath = paths.RData()/"mods";
	OsPath modUserPath = paths.UserData()/"mods";

	DirectoryNames modDirs;
	DirectoryNames modDirsUser;

	GetDirectoryEntries(modPath, NULL, &modDirs);
	// Sort modDirs so that we can do a fast lookup below
	std::sort(modDirs.begin(), modDirs.end());

	PIVFS vfs = CreateVfs();

	ScriptRequest rq(scriptInterface);
	JS::RootedValue value(rq.cx, Script::CreateObject(rq));

	for (DirectoryNames::iterator iter = modDirs.begin(); iter != modDirs.end(); ++iter)
	{
		JS::RootedValue json(rq.cx);
		if (!ParseModJSON(rq, vfs, modPath, *iter, &json))
			continue;
		// Valid mod data, add it to our structure
		Script::SetProperty(rq, value, utf8_from_wstring(iter->string()).c_str(), json);
	}

	GetDirectoryEntries(modUserPath, NULL, &modDirsUser);

	for (DirectoryNames::iterator iter = modDirsUser.begin(); iter != modDirsUser.end(); ++iter)
	{
		// Ignore mods in the user folder if we have already found them in modDirs.
		if (std::binary_search(modDirs.begin(), modDirs.end(), *iter))
			continue;

		JS::RootedValue json(rq.cx);
		if (!ParseModJSON(rq, vfs, modUserPath, *iter, &json))
			continue;
		// Valid mod data, add it to our structure
		Script::SetProperty(rq, value, utf8_from_wstring(iter->string()).c_str(), json);
	}

	return value.get();
}

const std::vector<CStr>& GetEnabledMods()
{
	return g_ModsLoaded;
}

const std::vector<CStr>& GetIncompatibleMods()
{
	return g_IncompatibleMods;
}

const std::vector<CStr>& GetFailedMods()
{
	return g_FailedMods;
}

const std::vector<CStr>& GetModsFromArguments(const CmdLineArgs& args, int flags)
{
	const bool initMods = (flags & INIT_MODS) == INIT_MODS;
	const bool addPublic = (flags & INIT_MODS_PUBLIC) == INIT_MODS_PUBLIC;

	if (!initMods)
		return g_ModsLoaded;

	g_ModsLoaded = args.GetMultiple("mod");

	if (addPublic)
		g_ModsLoaded.insert(g_ModsLoaded.begin(), "public");

	g_ModsLoaded.insert(g_ModsLoaded.begin(), "mod");

	return g_ModsLoaded;
}

void SetDefaultMods()
{
	g_ModsLoaded.clear();
	g_ModsLoaded.insert(g_ModsLoaded.begin(), "mod");
}

void ClearIncompatibleMods()
{
	g_IncompatibleMods.clear();
	g_FailedMods.clear();
}

bool CheckAndEnableMods(const ScriptInterface& scriptInterface, const std::vector<CStr>& mods)
{
	ScriptRequest rq(scriptInterface);

	JS::RootedValue availableMods(rq.cx, GetAvailableMods(scriptInterface));
	if (!AreModsCompatible(scriptInterface, mods, availableMods))
	{
		g_FailedMods = mods;
		return false;
	}
	g_ModsLoaded = mods;
	return true;
}

bool AreModsCompatible(const ScriptInterface& scriptInterface, const std::vector<CStr>& mods, const JS::RootedValue& availableMods)
{
	ScriptRequest rq(scriptInterface);
	std::unordered_map<CStr, std::vector<CStr>> modDependencies;
	std::unordered_map<CStr, CStr> modNameVersions;
	for (const CStr& mod : mods)
	{
		if (mod == "mod")
			continue;

		JS::RootedValue modData(rq.cx);

		// Requested mod is not available, fail
		if (!Script::HasProperty(rq, availableMods, mod.c_str()))
		{
			g_IncompatibleMods.push_back(mod);
			continue;
		}
		if (!Script::GetProperty(rq, availableMods, mod.c_str(), &modData))
		{
			g_IncompatibleMods.push_back(mod);
			continue;
		}

		std::vector<CStr> dependencies;
		CStr version;
		CStr name;
		Script::GetProperty(rq, modData, "dependencies", dependencies);
		Script::GetProperty(rq, modData, "version", version);
		Script::GetProperty(rq, modData, "name", name);

		modNameVersions.emplace(name, version);
		modDependencies.emplace(mod, dependencies);
	}

	static const std::vector<CStr> toCheck = { "<=", ">=", "=", "<", ">" };
	for (const CStr& mod : mods)
	{
		if (mod == "mod")
			continue;

		const std::unordered_map<CStr, std::vector<CStr>>::iterator res = modDependencies.find(mod);
		if (res == modDependencies.end())
			continue;
		const std::vector<CStr> deps = res->second;
		if (deps.empty())
			continue;

		for (const CStr& dep : deps)
		{
			if (dep.empty())
				continue;
			// 0ad<=0.0.24
			for (const CStr& op : toCheck)
			{
				const int pos = dep.Find(op.c_str());
				if (pos == -1)
					continue;
				//0ad
				const CStr modToCheck = dep.substr(0, pos);
				//0.0.24
				const CStr versionToCheck = dep.substr(pos + op.size());
				const std::unordered_map<CStr, CStr>::iterator it = modNameVersions.find(modToCheck);
				if (it == modNameVersions.end())
				{
					g_IncompatibleMods.push_back(mod);
					continue;
				}
				// 0.0.25(0ad) , <=, 0.0.24(required version)
				if (!CompareVersionStrings(it->second, op, versionToCheck))
				{
					g_IncompatibleMods.push_back(mod);
					continue;
				}
				break;
			}
		}

	}

	return g_IncompatibleMods.empty();
}

bool CompareVersionStrings(const CStr& version, const CStr& op, const CStr& required)
{
	std::vector<CStr> versionSplit;
	std::vector<CStr> requiredSplit;
	static const std::string toIgnore = "-,_";
	boost::split(versionSplit, version, boost::is_any_of(toIgnore), boost::token_compress_on);
	boost::split(requiredSplit, required, boost::is_any_of(toIgnore), boost::token_compress_on);
	boost::split(versionSplit, versionSplit[0], boost::is_any_of("."), boost::token_compress_on);
	boost::split(requiredSplit, requiredSplit[0], boost::is_any_of("."), boost::token_compress_on);

	const bool eq = op.Find("=") != -1;
	const bool lt = op.Find("<") != -1;
	const bool gt = op.Find(">") != -1;

	const size_t min = std::min(versionSplit.size(), requiredSplit.size());

	for (size_t i = 0; i < min; ++i)
	{
		const int diff = versionSplit[i].ToInt() - requiredSplit[i].ToInt();
		if ((gt && diff > 0) || (lt && diff < 0))
			return true;

		if ((gt && diff < 0) || (lt && diff > 0) || (eq && diff))
			return false;
	}

	const size_t versionSize = versionSplit.size();
	const size_t requiredSize = requiredSplit.size();
	if (versionSize == requiredSize)
		return eq;
	return versionSize < requiredSize ? lt : gt;
}


void CacheEnabledModVersions(const shared_ptr<ScriptContext>& scriptContext)
{
	ScriptInterface scriptInterface("Engine", "CacheEnabledModVersions", scriptContext);
	ScriptRequest rq(scriptInterface);

	JS::RootedValue availableMods(rq.cx, GetAvailableMods(scriptInterface));

	g_LoadedModVersions.clear();

	for (const CStr& mod : g_ModsLoaded)
	{
		// Ignore mod mod as it is irrelevant for compatibility checks
		if (mod == "mod")
			continue;

		CStr version;
		JS::RootedValue modData(rq.cx);
		if (Script::GetProperty(rq, availableMods, mod.c_str(), &modData))
			Script::GetProperty(rq, modData, "version", version);

		g_LoadedModVersions.push_back({mod, version});
	}
}

JS::Value GetLoadedModsWithVersions(const ScriptInterface& scriptInterface)
{
	ScriptRequest rq(scriptInterface);
	JS::RootedValue returnValue(rq.cx);
	Script::ToJSVal(rq, &returnValue, g_LoadedModVersions);
	return returnValue;
}

JS::Value GetEngineInfo(const ScriptInterface& scriptInterface)
{
	ScriptRequest rq(scriptInterface);

	JS::RootedValue mods(rq.cx, GetLoadedModsWithVersions(scriptInterface));
	JS::RootedValue metainfo(rq.cx);

	Script::CreateObject(
		rq,
		&metainfo,
		"engine_version", engine_version,
		"mods", mods);

	Script::FreezeObject(rq, metainfo, true);

	return metainfo;
}
}
