/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "JSInterface_GUIManager.h"

#include "gui/CGUI.h"
#include "gui/GUIManager.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "ps/GameSetup/Config.h"
#include "ps/VideoMode.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/StructuredClone.h"

namespace JSI_GUIManager
{
// Note that the initData argument may only contain clonable data.
// Functions aren't supported for example!
void PushGuiPage(const ScriptRequest& rq, const std::wstring& name, JS::HandleValue initData, JS::HandleValue callbackFunction)
{
	g_GUI->PushPage(name, Script::WriteStructuredClone(rq, initData), callbackFunction);
}

void SwitchGuiPage(const ScriptInterface& scriptInterface, const std::wstring& name, JS::HandleValue initData)
{
	g_GUI->SwitchPage(name, &scriptInterface, initData);
}

void PopGuiPage(const ScriptRequest& rq, JS::HandleValue args)
{
	if (g_GUI->GetPageCount() < 2)
	{
		ScriptException::Raise(rq, "Can't pop GUI pages when less than two pages are opened!");
		return;
	}

	g_GUI->PopPage(Script::WriteStructuredClone(rq, args));
}

void SetCursor(const std::wstring& name)
{
	g_VideoMode.SetCursor(name);
}

void ResetCursor()
{
	g_VideoMode.ResetCursor();
}

bool TemplateExists(const std::string& templateName)
{
	return g_GUI->TemplateExists(templateName);
}

CParamNode GetTemplate(const std::string& templateName)
{
	return g_GUI->GetTemplate(templateName);
}


void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&PushGuiPage>(rq, "PushGuiPage");
	ScriptFunction::Register<&SwitchGuiPage>(rq, "SwitchGuiPage");
	ScriptFunction::Register<&PopGuiPage>(rq, "PopGuiPage");
	ScriptFunction::Register<&SetCursor>(rq, "SetCursor");
	ScriptFunction::Register<&ResetCursor>(rq, "ResetCursor");
	ScriptFunction::Register<&TemplateExists>(rq, "TemplateExists");
	ScriptFunction::Register<&GetTemplate>(rq, "GetTemplate");

	ScriptFunction::Register<&CGUI::FindObjectByName, &ScriptInterface::ObjectFromCBData<CGUI>>(rq, "GetGUIObjectByName");
	ScriptFunction::Register<&CGUI::SetGlobalHotkey, &ScriptInterface::ObjectFromCBData<CGUI>>(rq, "SetGlobalHotkey");
	ScriptFunction::Register<&CGUI::UnsetGlobalHotkey, &ScriptInterface::ObjectFromCBData<CGUI>>(rq, "UnsetGlobalHotkey");
}
}
