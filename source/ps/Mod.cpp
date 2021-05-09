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

#include "lib/file/file_system.h"
#include "lib/file/vfs/vfs.h"
#include "lib/utf8.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/GameSetup/Paths.h"
#include "ps/Pyrogenesis.h"
#include "scriptinterface/ScriptInterface.h"

#include <algorithm>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <unordered_map>

std::vector<CStr> g_modsLoaded;
std::vector<CStr> g_incompatibleMods;
std::vector<CStr> g_failedMods;

std::vector<std::vector<CStr>> g_LoadedModVersions;

CmdLineArgs g_args;

JS::Value Mod::GetAvailableMods(const ScriptInterface& scriptInterface)
{
	ScriptRequest rq(scriptInterface);
	JS::RootedObject obj(rq.cx, JS_NewPlainObject(rq.cx));

	const Paths paths(g_args);

	// loop over all possible paths
	OsPath modPath = paths.RData()/"mods";
	OsPath modUserPath = paths.UserData()/"mods";

	DirectoryNames modDirs;
	DirectoryNames modDirsUser;

	GetDirectoryEntries(modPath, NULL, &modDirs);
	// Sort modDirs so that we can do a fast lookup below
	std::sort(modDirs.begin(), modDirs.end());

	PIVFS vfs = CreateVfs();

	for (DirectoryNames::iterator iter = modDirs.begin(); iter != modDirs.end(); ++iter)
	{
		vfs->Clear();
		// Mount with lowest priority, we don't want to overwrite anything
		if (vfs->Mount(L"", modPath / *iter / "", VFS_MOUNT_MUST_EXIST, VFS_MIN_PRIORITY) < 0)
			continue;

		CVFSFile modinfo;
		if (modinfo.Load(vfs, L"mod.json", false) != PSRETURN_OK)
			continue;

		JS::RootedValue json(rq.cx);
		if (!scriptInterface.ParseJSON(modinfo.GetAsString(), &json))
			continue;

		// Valid mod, add it to our structure
		JS_SetProperty(rq.cx, obj, utf8_from_wstring(iter->string()).c_str(), json);
	}

	GetDirectoryEntries(modUserPath, NULL, &modDirsUser);
	bool dev = InDevelopmentCopy();

	for (DirectoryNames::iterator iter = modDirsUser.begin(); iter != modDirsUser.end(); ++iter)
	{
		// If we are in a dev copy we do not mount mods in the user mod folder that
		// are already present in the mod folder, thus we skip those here.
		if (dev && std::binary_search(modDirs.begin(), modDirs.end(), *iter))
			continue;

		vfs->Clear();
		// Mount with lowest priority, we don't want to overwrite anything
		if (vfs->Mount(L"", modUserPath / *iter / "", VFS_MOUNT_MUST_EXIST, VFS_MIN_PRIORITY) < 0)
			continue;

		CVFSFile modinfo;
		if (modinfo.Load(vfs, L"mod.json", false) != PSRETURN_OK)
			continue;

		JS::RootedValue json(rq.cx);
		if (!scriptInterface.ParseJSON(modinfo.GetAsString(), &json))
			continue;

		// Valid mod, add it to our structure
		JS_SetProperty(rq.cx, obj, utf8_from_wstring(iter->string()).c_str(), json);
	}

	return JS::ObjectValue(*obj);
}

const std::vector<CStr>& Mod::GetEnabledMods()
{
	return g_modsLoaded;
}

const std::vector<CStr>& Mod::GetIncompatibleMods()
{
	return g_incompatibleMods;
}

const std::vector<CStr>& Mod::GetFailedMods()
{
	return g_failedMods;
}

const std::vector<CStr>& Mod::GetModsFromArguments(const CmdLineArgs& args, int flags)
{
	const bool initMods = (flags & INIT_MODS) == INIT_MODS;
	const bool addPublic = (flags & INIT_MODS_PUBLIC) == INIT_MODS_PUBLIC;

	if (!initMods)
		return g_modsLoaded;

	g_modsLoaded = args.GetMultiple("mod");

	if (addPublic)
		g_modsLoaded.insert(g_modsLoaded.begin(), "public");

	g_modsLoaded.insert(g_modsLoaded.begin(), "mod");

	return g_modsLoaded;
}

void Mod::SetDefaultMods(const CmdLineArgs& args, int flags)
{
	g_modsLoaded.clear();
	g_modsLoaded.insert(g_modsLoaded.begin(), "mod");
}

void Mod::ClearIncompatibleMods()
{
	g_incompatibleMods.clear();
	g_failedMods.clear();
}

bool Mod::CheckAndEnableMods(const ScriptInterface& scriptInterface, const std::vector<CStr>& mods)
{
	ScriptRequest rq(scriptInterface);

	JS::RootedValue availableMods(rq.cx, GetAvailableMods(scriptInterface));
	if (!AreModsCompatible(scriptInterface, mods, availableMods))
	{
		g_failedMods = mods;
		return false;
	}
	g_modsLoaded = mods;
	return true;
}

bool Mod::AreModsCompatible(const ScriptInterface& scriptInterface, const std::vector<CStr>& mods, const JS::RootedValue& availableMods)
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
		if (!scriptInterface.HasProperty(availableMods, mod.c_str()))
		{
			g_incompatibleMods.push_back(mod);
			continue;
		}
		if (!scriptInterface.GetProperty(availableMods, mod.c_str(), &modData))
		{
			g_incompatibleMods.push_back(mod);
			continue;
		}

		std::vector<CStr> dependencies;
		CStr version;
		CStr name;
		scriptInterface.GetProperty(modData, "dependencies", dependencies);
		scriptInterface.GetProperty(modData, "version", version);
		scriptInterface.GetProperty(modData, "name", name);

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
					g_incompatibleMods.push_back(mod);
					continue;
				}
				// 0.0.25(0ad) , <=, 0.0.24(required version)
				if (!CompareVersionStrings(it->second, op, versionToCheck))
				{
					g_incompatibleMods.push_back(mod);
					continue;
				}
			}
		}

	}

	return g_incompatibleMods.empty();
}

bool Mod::CompareVersionStrings(const CStr& version, const CStr& op, const CStr& required)
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
		if (gt && diff > 0 || lt && diff < 0)
			return true;

		if (gt && diff < 0 || lt && diff > 0 || eq && diff)
			return false;
	}

	const size_t versionSize = versionSplit.size();
	const size_t requiredSize = requiredSplit.size();
	if (versionSize == requiredSize)
		return eq;
	return versionSize < requiredSize ? lt : gt;
}


void Mod::CacheEnabledModVersions(const shared_ptr<ScriptContext>& scriptContext)
{
	ScriptInterface scriptInterface("Engine", "CacheEnabledModVersions", scriptContext);
	ScriptRequest rq(scriptInterface);

	JS::RootedValue availableMods(rq.cx, GetAvailableMods(scriptInterface));

	g_LoadedModVersions.clear();

	for (const CStr& mod : g_modsLoaded)
	{
		// Ignore mod mod as it is irrelevant for compatibility checks
		if (mod == "mod")
			continue;

		CStr version;
		JS::RootedValue modData(rq.cx);
		if (scriptInterface.GetProperty(availableMods, mod.c_str(), &modData))
			scriptInterface.GetProperty(modData, "version", version);

		g_LoadedModVersions.push_back({mod, version});
	}
}

JS::Value Mod::GetLoadedModsWithVersions(const ScriptInterface& scriptInterface)
{
	ScriptRequest rq(scriptInterface);
	JS::RootedValue returnValue(rq.cx);
	scriptInterface.ToJSVal(rq, &returnValue, g_LoadedModVersions);
	return returnValue;
}

JS::Value Mod::GetEngineInfo(const ScriptInterface& scriptInterface)
{
	ScriptRequest rq(scriptInterface);

	JS::RootedValue mods(rq.cx, Mod::GetLoadedModsWithVersions(scriptInterface));
	JS::RootedValue metainfo(rq.cx);

	ScriptInterface::CreateObject(
		rq,
		&metainfo,
		"engine_version", engine_version,
		"mods", mods);

	scriptInterface.FreezeObject(metainfo, true);

	return metainfo;
}
