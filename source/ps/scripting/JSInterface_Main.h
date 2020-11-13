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
	void QuitEngine(ScriptInterface::CmptPrivate* pCmptPrivate);
	void StartAtlas(ScriptInterface::CmptPrivate* pCmptPrivate);
	bool AtlasIsAvailable(ScriptInterface::CmptPrivate* pCmptPrivate);
	bool IsAtlasRunning(ScriptInterface::CmptPrivate* pCmptPrivate);
	void OpenURL(ScriptInterface::CmptPrivate* pCmptPrivate, const std::string& url);
	std::wstring GetSystemUsername(ScriptInterface::CmptPrivate* pCmptPrivate);
	std::wstring GetMatchID(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value LoadMapSettings(ScriptInterface::CmptPrivate* pCmptPrivate, const VfsPath& pathname);
	bool HotkeyIsPressed_(ScriptInterface::CmptPrivate* pCmptPrivate, const std::string& hotkeyName);
	int GetFps(ScriptInterface::CmptPrivate* pCmptPrivate);
	int GetTextWidth(ScriptInterface::CmptPrivate* pCmptPrivate, const std::string& fontName, const std::wstring& text);
	std::string CalculateMD5(ScriptInterface::CmptPrivate* pCmptPrivate, const std::string& input);

	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
}

#endif // INCLUDED_JSI_MAIN
