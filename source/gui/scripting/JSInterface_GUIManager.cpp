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

#include "precompiled.h"

#include "JSInterface_GUIManager.h"

#include "gui/CGUI.h"
#include "gui/GUIManager.h"
#include "gui/IGUIObject.h"
#include "ps/GameSetup/Config.h"
#include "scriptinterface/ScriptInterface.h"

// Note that the initData argument may only contain clonable data.
// Functions aren't supported for example!
void JSI_GUIManager::PushGuiPage(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& name, JS::HandleValue initData, JS::HandleValue callbackFunction)
{
	g_GUI->PushPage(name, pCxPrivate->pScriptInterface->WriteStructuredClone(initData), callbackFunction);
}

void JSI_GUIManager::SwitchGuiPage(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& name, JS::HandleValue initData)
{
	g_GUI->SwitchPage(name, pCxPrivate->pScriptInterface, initData);
}

void JSI_GUIManager::PopGuiPage(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue args)
{
	if (g_GUI->GetPageCount() < 2)
	{
		JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
		JSAutoRequest rq(cx);
		JS_ReportError(cx, "Can't pop GUI pages when less than two pages are opened!");
		return;
	}

	g_GUI->PopPage(pCxPrivate->pScriptInterface->WriteStructuredClone(args));
}

JS::Value JSI_GUIManager::GetGUIObjectByName(ScriptInterface::CxPrivate* pCxPrivate, const std::string& name)
{
	CGUI* guiPage = static_cast<CGUI*>(pCxPrivate->pCBData);

	IGUIObject* guiObj = guiPage->FindObjectByName(name);
	if (!guiObj)
		return JS::UndefinedValue();

	return JS::ObjectValue(*guiObj->GetJSObject());
}

void JSI_GUIManager::SetGlobalHotkey(ScriptInterface::CxPrivate* pCxPrivate, const std::string& hotkeyTag, JS::HandleValue function)
{
	CGUI* guiPage = static_cast<CGUI*>(pCxPrivate->pCBData);
	guiPage->SetGlobalHotkey(hotkeyTag, function);
}

void JSI_GUIManager::UnsetGlobalHotkey(ScriptInterface::CxPrivate* pCxPrivate, const std::string& hotkeyTag)
{
	CGUI* guiPage = static_cast<CGUI*>(pCxPrivate->pCBData);
	guiPage->UnsetGlobalHotkey(hotkeyTag);
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
	scriptInterface.RegisterFunction<void, std::wstring, JS::HandleValue, JS::HandleValue, &PushGuiPage>("PushGuiPage");
	scriptInterface.RegisterFunction<void, std::wstring, JS::HandleValue, &SwitchGuiPage>("SwitchGuiPage");
	scriptInterface.RegisterFunction<void, std::string, JS::HandleValue, &SetGlobalHotkey>("SetGlobalHotkey");
	scriptInterface.RegisterFunction<void, std::string, &UnsetGlobalHotkey>("UnsetGlobalHotkey");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &PopGuiPage>("PopGuiPage");
	scriptInterface.RegisterFunction<JS::Value, std::string, &GetGUIObjectByName>("GetGUIObjectByName");
	scriptInterface.RegisterFunction<std::wstring, std::wstring, &SetCursor>("SetCursor");
	scriptInterface.RegisterFunction<void, &ResetCursor>("ResetCursor");
	scriptInterface.RegisterFunction<bool, std::string, &TemplateExists>("TemplateExists");
	scriptInterface.RegisterFunction<CParamNode, std::string, &GetTemplate>("GetTemplate");
}
