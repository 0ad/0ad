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
#include "ps/Pyrogenesis.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/ScriptConversions.h"

extern void RestartEngine();

// To avoid copying data needlessly in GetEngineInfo, implement a ToJSVal for pointer types.
using ModDataCPtr = const Mod::ModData*;

template<>
void Script::ToJSVal(const ScriptRequest& rq, JS::MutableHandleValue ret, const ModDataCPtr& data)
{
	ret.set(Script::CreateObject(rq));
	Script::SetProperty(rq, ret, "mod", data->m_Pathname);
	Script::SetProperty(rq, ret, "name", data->m_Name);
	Script::SetProperty(rq, ret, "version", data->m_Version);
	Script::SetProperty(rq, ret, "ignoreInCompatibilityChecks", data->m_IgnoreInCompatibilityChecks);
}

// Required by JSVAL_VECTOR, but can't be implemented.
template<>
bool Script::FromJSVal(const ScriptRequest &, const JS::HandleValue, ModDataCPtr&)
{
	LOGERROR("Not implemented");
	return false;
}

JSVAL_VECTOR(const Mod::ModData*);

// Implement FromJSVal as a non-pointer type.
template<>
void Script::ToJSVal(const ScriptRequest& rq, JS::MutableHandleValue ret, const Mod::ModData& data)
{
	ret.set(Script::CreateObject(rq));
	Script::SetProperty(rq, ret, "mod", data.m_Pathname);
	Script::SetProperty(rq, ret, "name", data.m_Name);
	Script::SetProperty(rq, ret, "version", data.m_Version);
	Script::SetProperty(rq, ret, "ignoreInCompatibilityChecks", data.m_IgnoreInCompatibilityChecks);
}

template<>
bool Script::FromJSVal(const ScriptRequest& rq, const JS::HandleValue val, Mod::ModData& data)
{
	// To avoid errors & for convenience, some retro-compatibility when reading
	// TODO: remove this once we hit A26.
	JS::RootedObject obj(rq.cx, val.toObjectOrNull());
	bool isArray = false;
	if (JS::IsArray(rq.cx, obj, &isArray) && isArray)
	{
		if (!Script::GetPropertyInt(rq, val, 0, data.m_Pathname))
			return false;
		if (!Script::GetPropertyInt(rq, val, 1, data.m_Version))
			return false;
		// Set a sane default for this.
		data.m_Name = data.m_Pathname;
		return true;
	}

	// This property is not set in mod.json files, so don't fail if it's not there.
	if (Script::HasProperty(rq, val, "mod") && !Script::GetProperty(rq, val, "mod", data.m_Pathname))
		return false;

	if (!Script::GetProperty(rq, val, "version", data.m_Version))
		return false;
	if (!Script::GetProperty(rq, val, "name", data.m_Name))
		return false;

	// Optional - this makes the mod 'GUI-only'.
	if (Script::HasProperty(rq, val, "ignoreInCompatibilityChecks"))
	{
		if (!Script::GetProperty(rq, val, "ignoreInCompatibilityChecks", data.m_IgnoreInCompatibilityChecks))
			return false;
	}
	else
		data.m_IgnoreInCompatibilityChecks = false;

	return true;
}

JSVAL_VECTOR(Mod::ModData);

namespace JSI_Mod
{
Mod* ModGetter(const ScriptRequest&, JS::CallArgs&)
{
	return &g_Mods;
}

JS::Value GetEngineInfo(const ScriptInterface& scriptInterface)
{
	ScriptRequest rq(scriptInterface);

	JS::RootedValue mods(rq.cx);
	Script::ToJSVal(rq, &mods, g_Mods.GetEnabledModsData());
	JS::RootedValue metainfo(rq.cx);

	Script::CreateObject(
		 rq,
		 &metainfo,
		 "engine_version", engine_version,
		 "mods", mods);

	Script::FreezeObject(rq, metainfo, true);

	return metainfo;
}

JS::Value GetAvailableMods(const ScriptRequest& rq)
{
	JS::RootedValue ret(rq.cx, Script::CreateObject(rq));
	for (const Mod::ModData& data : g_Mods.GetAvailableMods())
	{
		JS::RootedValue json(rq.cx);
		if (!Script::ParseJSON(rq, data.m_Text, &json))
		{
			ScriptException::Raise(rq, "Error parsing mod.json of '%s'", data.m_Pathname.c_str());
			continue;
		}
		Script::SetProperty(rq, ret, data.m_Pathname.c_str(), json);
	}
	return ret.get();
}

bool AreModsPlayCompatible(const std::vector<Mod::ModData>& a, const std::vector<Mod::ModData>& b)
{
	std::vector<const Mod::ModData*> modsA, modsB;
	modsA.reserve(a.size());
	for (const Mod::ModData& mod : a)
		modsA.push_back(&mod);
	modsB.reserve(b.size());
	for (const Mod::ModData& mod : b)
		modsB.push_back(&mod);
	return Mod::AreModsPlayCompatible(modsA, modsB);
}

bool SetModsAndRestartEngine(const std::vector<CStr>& mods)
{
	if (!g_Mods.EnableMods(mods, false))
		return false;

	RestartEngine();
	return true;
}

bool HasIncompatibleMods()
{
	return g_Mods.GetIncompatibleMods().size() > 0;
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<GetEngineInfo>(rq, "GetEngineInfo");
	ScriptFunction::Register<GetAvailableMods>(rq, "GetAvailableMods");
	ScriptFunction::Register<&Mod::GetEnabledMods, ModGetter>(rq, "GetEnabledMods");
	ScriptFunction::Register<AreModsPlayCompatible>(rq, "AreModsPlayCompatible");
	ScriptFunction::Register<HasIncompatibleMods> (rq, "HasIncompatibleMods");
	ScriptFunction::Register<&Mod::GetIncompatibleMods, ModGetter>(rq, "GetIncompatibleMods");
	ScriptFunction::Register<&SetModsAndRestartEngine>(rq, "SetModsAndRestartEngine");
}
}
