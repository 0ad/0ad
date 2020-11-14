/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_JSI_SAVEDGAME
#define INCLUDED_JSI_SAVEDGAME

#include "scriptinterface/ScriptInterface.h"

namespace JSI_SavedGame
{
	JS::Value GetSavedGames(ScriptInterface::CmptPrivate* pCmptPrivate);
	bool DeleteSavedGame(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& name);
	void SaveGame(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& filename, const std::wstring& description, JS::HandleValue GUIMetadata);
	void SaveGamePrefix(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& prefix, const std::wstring& description, JS::HandleValue GUIMetadata);
	void QuickSave(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue GUIMetadata);
	void QuickLoad(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value StartSavedGame(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& name);

	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
}

#endif // INCLUDED_JSI_SAVEDGAME
