/* Copyright (C) 2020 Wildfire Games.
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

#include <unordered_map>
#include <vector>

#include "lib/external_libraries/libsdl.h"
#include "ps/Hotkey.h"
#include "ps/KeyName.h"
#include "scriptinterface/ScriptConversions.h"

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
	for (const std::pair<T, U>& item : val)
	{
		JS::RootedValue el(rq.cx);
		ScriptInterface::ToJSVal<U>(rq, &el, item.second);
		JS_SetProperty(rq.cx, obj, item.first.c_str(), el);
	}
	ret.setObject(*obj);
}

template<>
void ScriptInterface::ToJSVal<std::unordered_map<std::string, std::vector<std::vector<std::string>>>>(const ScriptRequest& rq, JS::MutableHandleValue ret, const std::unordered_map<std::string, std::vector<std::vector<std::string>>>& val)
{
	ToJSVal_unordered_map(rq, ret, val);
}

template<>
void ScriptInterface::ToJSVal<std::unordered_map<std::string, std::string>>(const ScriptRequest& rq, JS::MutableHandleValue ret, const std::unordered_map<std::string, std::string>& val)
{
	ToJSVal_unordered_map(rq, ret, val);
}

/**
 * @return a (js) object mapping hotkey name (from cfg files) to a list ofscancode names
 */
JS::Value GetHotkeyMap(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	ScriptRequest rq(*pCmptPrivate->pScriptInterface);

	JS::RootedValue hotkeyMap(rq.cx);

	std::unordered_map<std::string, std::vector<std::vector<std::string>>> hotkeys;
	for (const std::pair<SDL_Scancode_, KeyMapping>& key : g_HotkeyMap)
		for (const SHotkeyMapping& mapping : key.second)
		{
			std::vector<std::string> keymap;
			keymap.push_back(FindScancodeName(static_cast<SDL_Scancode>(key.first)));
			for (const SKey& secondary_key : mapping.requires)
				keymap.push_back(FindScancodeName(static_cast<SDL_Scancode>(secondary_key.code)));
			// All hotkey permutations are present so only push one (arbitrarily).
			if (keymap.size() == 1 || keymap[0] < keymap[1])
				hotkeys[mapping.name].emplace_back(keymap);
		}
	pCmptPrivate->pScriptInterface->ToJSVal(rq, &hotkeyMap, hotkeys);

	return hotkeyMap;
}

/**
 * @return a (js) object mapping scancode names to their locale-dependent name.
 */
JS::Value GetScancodeKeyNames(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	ScriptRequest rq(*pCmptPrivate->pScriptInterface);

	JS::RootedValue obj(rq.cx);
	std::unordered_map<std::string, std::string> map;

	// Get the name of all scancodes.
	// This is slightly wasteful but should be fine overall, they are dense.
	for (int i = 0; i < MOUSE_LAST; ++i)
		map[FindScancodeName(static_cast<SDL_Scancode>(i))] = FindKeyName(static_cast<SDL_Scancode>(i));
	pCmptPrivate->pScriptInterface->ToJSVal(rq, &obj, map);

	return obj;
}

void ReloadHotkeys(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	UnloadHotkeys();
	LoadHotkeys();
}

JS::Value GetConflicts(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue combination)
{
	ScriptInterface* scriptInterface = pCmptPrivate->pScriptInterface;
	ScriptRequest rq(*scriptInterface);

	std::vector<std::string> keys;
	if (!scriptInterface->FromJSVal(rq, combination, keys))
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
		codes.insert(SKey{ FindScancode(key), false });

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
	scriptInterface->ToJSVal(rq, &ret, conflicts);
	return ret;
}

void JSI_Hotkey::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<JS::Value, &GetHotkeyMap>("GetHotkeyMap");
	scriptInterface.RegisterFunction<JS::Value, &GetScancodeKeyNames>("GetScancodeKeyNames");
	scriptInterface.RegisterFunction<void, &ReloadHotkeys>("ReloadHotkeys");
	scriptInterface.RegisterFunction<JS::Value, JS::HandleValue, &GetConflicts>("GetConflicts");
}
