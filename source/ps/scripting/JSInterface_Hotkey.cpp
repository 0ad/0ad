/* Copyright (C) 2022 Wildfire Games.
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

#include "JSInterface_Hotkey.h"

#include "lib/external_libraries/libsdl.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Hotkey.h"
#include "ps/KeyName.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptConversions.h"

#include <unordered_map>
#include <vector>
#include <set>

/**
 * Convert an unordered map to a JS object, mapping keys to values.
 * Assumes T to have a c_str() method that returns a const char*
 * NB: this is unordered since no particular effort is made to preserve order.
 * TODO: this could be moved to ScriptConversions.cpp if the need arises.
 */
template<typename T, typename U>
static void ToJSVal_unordered_map(const ScriptRequest& rq, JS::MutableHandleValue ret, const std::unordered_map<T, U>& val)
{
	JS::RootedObject obj(rq.cx, JS_NewPlainObject(rq.cx));
	if (!obj)
	{
		ret.setUndefined();
		return;
	}
	for (const std::pair<const T, U>& item : val)
	{
		JS::RootedValue el(rq.cx);
		Script::ToJSVal<U>(rq, &el, item.second);
		JS_SetProperty(rq.cx, obj, item.first.c_str(), el);
	}
	ret.setObject(*obj);
}

template<>
void Script::ToJSVal<std::unordered_map<std::string, std::vector<std::vector<std::string>>>>(const ScriptRequest& rq, JS::MutableHandleValue ret, const std::unordered_map<std::string, std::vector<std::vector<std::string>>>& val)
{
	ToJSVal_unordered_map(rq, ret, val);
}

template<>
void Script::ToJSVal<std::unordered_map<std::string, std::string>>(const ScriptRequest& rq, JS::MutableHandleValue ret, const std::unordered_map<std::string, std::string>& val)
{
	ToJSVal_unordered_map(rq, ret, val);
}

namespace
{
/**
 * @return a (js) object mapping hotkey name (from cfg files) to a list ofscancode names
 */
JS::Value GetHotkeyMap(const ScriptRequest& rq)
{
	JS::RootedValue hotkeyMap(rq.cx);

	std::unordered_map<std::string, std::vector<std::vector<std::string>>> hotkeys;
	for (const std::pair<const SDL_Scancode_, KeyMapping>& key : g_HotkeyMap)
		for (const SHotkeyMapping& mapping : key.second)
		{
			std::vector<std::string> keymap;
			if (key.first != UNUSED_HOTKEY_CODE)
				keymap.push_back(FindScancodeName(static_cast<SDL_Scancode>(key.first)));
			for (const SKey& secondary_key : mapping.requires)
				keymap.push_back(FindScancodeName(static_cast<SDL_Scancode>(secondary_key.code)));
			// If keymap is empty (== unused) or size 1, push the combination.
			// Otherwise, all permutations of the combination will exist, so pick one using an arbitrary order.
			if (keymap.size() < 2 || keymap[0] < keymap[1])
				hotkeys[mapping.name].emplace_back(keymap);
		}
	Script::ToJSVal(rq, &hotkeyMap, hotkeys);

	return hotkeyMap;
}

/**
 * @return a (js) object mapping scancode names to their locale-dependent name.
 */
JS::Value GetScancodeKeyNames(const ScriptRequest& rq)
{
	JS::RootedValue obj(rq.cx);
	std::unordered_map<std::string, std::string> map;

	// Get the name of all scancodes.
	// This is slightly wasteful but should be fine overall, they are dense.
	for (int i = 0; i < MOUSE_LAST; ++i)
		map[FindScancodeName(static_cast<SDL_Scancode>(i))] = FindKeyName(static_cast<SDL_Scancode>(i));
	Script::ToJSVal(rq, &obj, map);

	return obj;
}

void ReloadHotkeys()
{
	UnloadHotkeys();
	LoadHotkeys(g_ConfigDB);
}

JS::Value GetConflicts(const ScriptRequest& rq, JS::HandleValue combination)
{
	std::vector<std::string> keys;
	if (!Script::FromJSVal(rq, combination, keys))
	{
		LOGERROR("Invalid hotkey combination");
		return JS::NullValue();
	}

	if (keys.empty())
		return JS::NullValue();

	// Pick a random code as a starting point of the hotkeys (they are all equivalent).
	SDL_Scancode_ startCode = FindScancode(keys.back());

	std::unordered_map<SDL_Scancode_, KeyMapping>::const_iterator it = g_HotkeyMap.find(startCode);
	if (it == g_HotkeyMap.end())
		return JS::NullValue();

	// Create a sorted vector with the remaining keys.
	keys.pop_back();

	std::set<SKey> codes;
	for (const std::string& key : keys)
		codes.insert(SKey{ FindScancode(key) });

	std::vector<CStr> conflicts;
	// This isn't very efficient, but we shouldn't iterate too many hotkeys
	// since we at least have one matching key.
	for (const SHotkeyMapping& keymap : it->second)
	{
		std::set<SKey> match(keymap.requires.begin(), keymap.requires.end());
		if (codes == match)
			conflicts.emplace_back(keymap.name);
	}
	if (conflicts.empty())
		return JS::NullValue();

	JS::RootedValue ret(rq.cx);
	Script::ToJSVal(rq, &ret, conflicts);
	return ret;
}
}

void JSI_Hotkey::RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&HotkeyIsPressed>(rq, "HotkeyIsPressed");
	ScriptFunction::Register<&GetHotkeyMap>(rq, "GetHotkeyMap");
	ScriptFunction::Register<&GetScancodeKeyNames>(rq, "GetScancodeKeyNames");
	ScriptFunction::Register<&ReloadHotkeys>(rq, "ReloadHotkeys");
	ScriptFunction::Register<&GetConflicts>(rq, "GetConflicts");
}
