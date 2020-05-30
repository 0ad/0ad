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

#ifndef INCLUDED_JSI_MAIN
#define INCLUDED_JSI_MAIN

#include "scriptinterface/ScriptInterface.h"

namespace JSI_Main
{
	void QuitEngine(ScriptInterface::RealmPrivate* pRealmPrivate);
	void StartAtlas(ScriptInterface::RealmPrivate* pRealmPrivate);
	bool AtlasIsAvailable(ScriptInterface::RealmPrivate* pRealmPrivate);
	bool IsAtlasRunning(ScriptInterface::RealmPrivate* pRealmPrivate);
	void OpenURL(ScriptInterface::RealmPrivate* pRealmPrivate, const std::string& url);
	std::wstring GetSystemUsername(ScriptInterface::RealmPrivate* pRealmPrivate);
	std::wstring GetMatchID(ScriptInterface::RealmPrivate* pRealmPrivate);
	JS::Value LoadMapSettings(ScriptInterface::RealmPrivate* pRealmPrivate, const VfsPath& pathname);
	bool HotkeyIsPressed_(ScriptInterface::RealmPrivate* pRealmPrivate, const std::string& hotkeyName);
	int GetFps(ScriptInterface::RealmPrivate* pRealmPrivate);
	int GetTextWidth(ScriptInterface::RealmPrivate* pRealmPrivate, const std::string& fontName, const std::wstring& text);
	std::string CalculateMD5(ScriptInterface::RealmPrivate* pRealmPrivate, const std::string& input);

	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
}

#endif // INCLUDED_JSI_MAIN
