/* Copyright (C) 2017 Wildfire Games.
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
	void ExitProgram(ScriptInterface::CxPrivate* pCxPrivate);
	void RestartInAtlas(ScriptInterface::CxPrivate* pCxPrivate);
	bool AtlasIsAvailable(ScriptInterface::CxPrivate* pCxPrivate);
	bool IsAtlasRunning(ScriptInterface::CxPrivate* pCxPrivate);
	void OpenURL(ScriptInterface::CxPrivate* pCxPrivate, const std::string& url);
	std::wstring GetSystemUsername(ScriptInterface::CxPrivate* pCxPrivate);
	std::wstring GetMatchID(ScriptInterface::CxPrivate* pCxPrivate);
	JS::Value LoadMapSettings(ScriptInterface::CxPrivate* pCxPrivate, const VfsPath& pathname);
	bool HotkeyIsPressed_(ScriptInterface::CxPrivate* pCxPrivate, const std::string& hotkeyName);
	int GetFps(ScriptInterface::CxPrivate* pCxPrivate);
	int GetTextWidth(ScriptInterface::CxPrivate* pCxPrivate, const std::string& fontName, const std::wstring& text);

	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
}

#endif
