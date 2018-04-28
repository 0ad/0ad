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

#include "precompiled.h"

#include "JSInterface_GUIManager.h"

#include "gui/GUIManager.h"
#include "gui/IGUIObject.h"
#include "ps/GameSetup/Config.h"
#include "scriptinterface/ScriptInterface.h"

// Note that the initData argument may only contain clonable data.
// Functions aren't supported for example!
void JSI_GUIManager::PushGuiPage(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& name, JS::HandleValue initData)
{
	g_GUI->PushPage(name, pCxPrivate->pScriptInterface->WriteStructuredClone(initData));
}

void JSI_GUIManager::SwitchGuiPage(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& name, JS::HandleValue initData)
{
	g_GUI->SwitchPage(name, pCxPrivate->pScriptInterface, initData);
}

void JSI_GUIManager::PopGuiPage(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	g_GUI->PopPage();
}

void JSI_GUIManager::PopGuiPageCB(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue args)
{
	g_GUI->PopPageCB(pCxPrivate->pScriptInterface->WriteStructuredClone(args));
}

JS::Value JSI_GUIManager::GetGUIObjectByName(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& name)
{
	IGUIObject* guiObj = g_GUI->FindObjectByName(name);
	if (!guiObj)
		return JS::UndefinedValue();

	return JS::ObjectValue(*guiObj->GetJSObject());
}

std::wstring JSI_GUIManager::SetCursor(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& name)
{
	std::wstring old = g_CursorName;
	g_CursorName = name;
	return old;
}

void JSI_GUIManager::ResetCursor(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	g_GUI->ResetCursor();
}

bool JSI_GUIManager::TemplateExists(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& templateName)
{
	return g_GUI->TemplateExists(templateName);
}

CParamNode JSI_GUIManager::GetTemplate(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& templateName)
{
	return g_GUI->GetTemplate(templateName);
}

void JSI_GUIManager::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<void, std::wstring, JS::HandleValue, &PushGuiPage>("PushGuiPage");
	scriptInterface.RegisterFunction<void, std::wstring, JS::HandleValue, &SwitchGuiPage>("SwitchGuiPage");
	scriptInterface.RegisterFunction<void, &PopGuiPage>("PopGuiPage");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &PopGuiPageCB>("PopGuiPageCB");
	scriptInterface.RegisterFunction<JS::Value, std::string, &GetGUIObjectByName>("GetGUIObjectByName");
	scriptInterface.RegisterFunction<std::wstring, std::wstring, &SetCursor>("SetCursor");
	scriptInterface.RegisterFunction<void, &ResetCursor>("ResetCursor");
	scriptInterface.RegisterFunction<bool, std::string, &TemplateExists>("TemplateExists");
	scriptInterface.RegisterFunction<CParamNode, std::string, &GetTemplate>("GetTemplate");
}
