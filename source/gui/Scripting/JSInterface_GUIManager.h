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

#ifndef INCLUDED_JSI_GUIMANAGER
#define INCLUDED_JSI_GUIMANAGER

#include "scriptinterface/ScriptInterface.h"
#include "simulation2/system/ParamNode.h"

namespace JSI_GUIManager
{
	void PushGuiPage(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& name, JS::HandleValue initData, JS::HandleValue callbackFunction);
	void SwitchGuiPage(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& name, JS::HandleValue initData);
	void PopGuiPage(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue args);
	JS::Value GetGUIObjectByName(ScriptInterface::CmptPrivate* pCmptPrivate, const std::string& name);
	void SetGlobalHotkey(ScriptInterface::CmptPrivate* pCmptPrivate, const std::string& hotkeyTag, const std::string& eventName, JS::HandleValue function);
	void UnsetGlobalHotkey(ScriptInterface::CmptPrivate* pCmptPrivate, const std::string& hotkeyTag, const std::string& eventName);
	std::wstring SetCursor(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& name);
	void ResetCursor(ScriptInterface::CmptPrivate* pCmptPrivate);
	bool TemplateExists(ScriptInterface::CmptPrivate* pCmptPrivate, const std::string& templateName);
	CParamNode GetTemplate(ScriptInterface::CmptPrivate* pCmptPrivate, const std::string& templateName);

	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
}

#endif // INCLUDED_JSI_GUIMANAGER
