/* Copyright (C) 2014 Wildfire Games.
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

#include "GUIManager.h"

#include "CGUI.h"

#include "lib/timer.h"
#include "ps/Filesystem.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "ps/XML/Xeromyces.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRuntime.h"

CGUIManager* g_GUI = NULL;


// General TODOs:
//
// A lot of the CGUI data could (and should) be shared between
// multiple pages, instead of treating them as completely independent, to save
// memory and loading time.


// called from main loop when (input) events are received.
// event is passed to other handlers if false is returned.
// trampoline: we don't want to make the HandleEvent implementation static
InReaction gui_handler(const SDL_Event_* ev)
{
	PROFILE("GUI event handler");
	return g_GUI->HandleEvent(ev);
}

static Status ReloadChangedFileCB(void* param, const VfsPath& path)
{
	return static_cast<CGUIManager*>(param)->ReloadChangedFile(path);
}

CGUIManager::CGUIManager()
{
	m_ScriptRuntime = g_ScriptRuntime;
	m_ScriptInterface.reset(new ScriptInterface("Engine", "GUIManager", m_ScriptRuntime));
	m_ScriptInterface->SetCallbackData(this);
	m_ScriptInterface->LoadGlobalScripts();
	RegisterFileReloadFunc(ReloadChangedFileCB, this);
}

CGUIManager::~CGUIManager()
{
	UnregisterFileReloadFunc(ReloadChangedFileCB, this);
}

bool CGUIManager::HasPages()
{
	return !m_PageStack.empty();
}

void CGUIManager::SwitchPage(const CStrW& pageName, ScriptInterface* srcScriptInterface, JS::HandleValue initData)
{
	// The page stack is cleared (including the script context where initData came from),
	// therefore we have to clone initData.
	shared_ptr<ScriptInterface::StructuredClone> initDataClone;
	if (!initData.isUndefined())
	{
		initDataClone = srcScriptInterface->WriteStructuredClone(initData);
	}
	m_PageStack.clear();
	PushPage(pageName, initDataClone);
}

void CGUIManager::PushPage(const CStrW& pageName, shared_ptr<ScriptInterface::StructuredClone> initData)
{
	m_PageStack.push_back(SGUIPage());
	m_PageStack.back().name = pageName;
	m_PageStack.back().initData = initData;
	LoadPage(m_PageStack.back());
}

void CGUIManager::PopPage()
{
	if (m_PageStack.size() < 2)
	{
		debug_warn(L"Tried to pop GUI page when there's < 2 in the stack");
		return;
	}

	m_PageStack.pop_back();
}

void CGUIManager::PopPageCB(shared_ptr<ScriptInterface::StructuredClone> args)
{
	shared_ptr<ScriptInterface::StructuredClone> initDataClone = m_PageStack.back().initData;
	PopPage();
	
	shared_ptr<ScriptInterface> scriptInterface = m_PageStack.back().gui->GetScriptInterface();
	JSContext* cx = scriptInterface->GetContext();
	JS::RootedValue initDataVal(cx);
	if (initDataClone)
		scriptInterface->ReadStructuredClone(initDataClone, &initDataVal);
	else
	{
		LOGERROR("Called PopPageCB when initData (which should contain the callback function name) isn't set!");
		return;
	}
	
	if (!scriptInterface->HasProperty(initDataVal, "callback"))
	{
		LOGERROR("Called PopPageCB when the callback function name isn't set!");
		return;
	}
	
	std::string callback;
	if (!scriptInterface->GetProperty(initDataVal, "callback", callback))
	{
		LOGERROR("Failed to get the callback property as a string from initData in PopPageCB!");
		return;
	}
	
	JS::RootedValue global(cx, scriptInterface->GetGlobalObject());
	if (!scriptInterface->HasProperty(global, callback.c_str()))
	{
		LOGERROR("The specified callback function %s does not exist in the page %s", callback, utf8_from_wstring(m_PageStack.back().name));
		return;
	}

	JS::RootedValue argVal(cx);
	if (args)
		scriptInterface->ReadStructuredClone(args, &argVal);
	if (!scriptInterface->CallFunctionVoid(global, callback.c_str(), argVal))
	{
		LOGERROR("Failed to call the callback function %s in the page %s", callback, utf8_from_wstring(m_PageStack.back().name));
		return;
	}
}

void CGUIManager::DisplayMessageBox(int width, int height, const CStrW& title, const CStrW& message)
{
	JSContext* cx = m_ScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	// Set up scripted init data for the standard message box window
	JS::RootedValue data(cx);
	m_ScriptInterface->Eval("({})", &data);
	m_ScriptInterface->SetProperty(data, "width", width, false);
	m_ScriptInterface->SetProperty(data, "height", height, false);
	m_ScriptInterface->SetProperty(data, "mode", 2, false);
	m_ScriptInterface->SetProperty(data, "title", std::wstring(title), false);
	m_ScriptInterface->SetProperty(data, "message", std::wstring(message), false);

	// Display the message box
	PushPage(L"page_msgbox.xml", m_ScriptInterface->WriteStructuredClone(data));
}

void CGUIManager::LoadPage(SGUIPage& page)
{
	// If we're hotloading then try to grab some data from the previous page
	shared_ptr<ScriptInterface::StructuredClone> hotloadData;
	if (page.gui)
	{
		shared_ptr<ScriptInterface> scriptInterface = page.gui->GetScriptInterface();
		JSContext* cx = scriptInterface->GetContext();
		JSAutoRequest rq(cx);
		
		JS::RootedValue global(cx, scriptInterface->GetGlobalObject());
		JS::RootedValue hotloadDataVal(cx);
		scriptInterface->CallFunction(global, "getHotloadData", &hotloadDataVal); 
		hotloadData = scriptInterface->WriteStructuredClone(hotloadDataVal);
	}
		
	page.inputs.clear();
	page.gui.reset(new CGUI(m_ScriptRuntime));

	page.gui->Initialize();

	VfsPath path = VfsPath("gui") / page.name;
	page.inputs.insert(path);

	CXeromyces xero;
	if (xero.Load(g_VFS, path) != PSRETURN_OK)
		// Fail silently (Xeromyces reported the error)
		return;

	int elmt_page = xero.GetElementID("page");
	int elmt_include = xero.GetElementID("include");

	XMBElement root = xero.GetRoot();

	if (root.GetNodeName() != elmt_page)
	{
		LOGERROR("GUI page '%s' must have root element <page>", utf8_from_wstring(page.name));
		return;
	}

	XERO_ITER_EL(root, node)
	{
		if (node.GetNodeName() != elmt_include)
		{
			LOGERROR("GUI page '%s' must only have <include> elements inside <page>", utf8_from_wstring(page.name));
			continue;
		}

		CStrW name (node.GetText().FromUTF8());

		PROFILE2("load gui xml");
		PROFILE2_ATTR("name: %ls", name.c_str());

		TIMER(name.c_str());
		VfsPath path = VfsPath("gui") / name;
		page.gui->LoadXmlFile(path, page.inputs);
	}

	// Remember this GUI page, in case the scripts call FindObjectByName
	shared_ptr<CGUI> oldGUI = m_CurrentGUI;
	m_CurrentGUI = page.gui;

	page.gui->SendEventToAll("load");

	shared_ptr<ScriptInterface> scriptInterface = page.gui->GetScriptInterface();
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);
	
	JS::RootedValue initDataVal(cx);
	JS::RootedValue hotloadDataVal(cx);
	JS::RootedValue global(cx, scriptInterface->GetGlobalObject());
	if (page.initData) 
		scriptInterface->ReadStructuredClone(page.initData, &initDataVal);
	if (hotloadData)
		scriptInterface->ReadStructuredClone(hotloadData, &hotloadDataVal);
	
	// Call the init() function
	if (!scriptInterface->CallFunctionVoid(
			global, 
			"init", 
			initDataVal, 
			hotloadDataVal)
		)
	{
		LOGERROR("GUI page '%s': Failed to call init() function", utf8_from_wstring(page.name));
	}

	m_CurrentGUI = oldGUI;
}

Status CGUIManager::ReloadChangedFile(const VfsPath& path)
{
	for (PageStackType::iterator it = m_PageStack.begin(); it != m_PageStack.end(); ++it)
	{
		if (it->inputs.count(path))
		{
			LOGMESSAGE("GUI file '%s' changed - reloading page '%s'", path.string8(), utf8_from_wstring(it->name));
			LoadPage(*it);
			// TODO: this can crash if LoadPage runs an init script which modifies the page stack and breaks our iterators
		}
	}

	return INFO::OK;
}

Status CGUIManager::ReloadAllPages()
{
	// TODO: this can crash if LoadPage runs an init script which modifies the page stack and breaks our iterators
	for (PageStackType::iterator it = m_PageStack.begin(); it != m_PageStack.end(); ++it)
		LoadPage(*it);

	return INFO::OK;
}

std::string CGUIManager::GetSavedGameData()
{
	shared_ptr<ScriptInterface> scriptInterface = top()->GetScriptInterface();
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);
	
	JS::RootedValue data(cx);
	JS::RootedValue global(cx, top()->GetGlobalObject());
	scriptInterface->CallFunction(global, "getSavedGameData", &data);
	return scriptInterface->StringifyJSON(&data, false);
}

void CGUIManager::RestoreSavedGameData(std::string jsonData)
{
	shared_ptr<ScriptInterface> scriptInterface = top()->GetScriptInterface();
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);
	
	JS::RootedValue global(cx, top()->GetGlobalObject());
	JS::RootedValue dataVal(cx);
	scriptInterface->ParseJSON(jsonData, &dataVal);
	scriptInterface->CallFunctionVoid(global, "restoreSavedGameData", dataVal);
}

InReaction CGUIManager::HandleEvent(const SDL_Event_* ev)
{
	// We want scripts to have access to the raw input events, so they can do complex
	// processing when necessary (e.g. for unit selection and camera movement).
	// Sometimes they'll want to be layered behind the GUI widgets (e.g. to detect mousedowns on the
	// visible game area), sometimes they'll want to intercepts events before the GUI (e.g.
	// to capture all mouse events until a mouseup after dragging).
	// So we call two separate handler functions:
	
	bool handled;

	{
		PROFILE("handleInputBeforeGui");
		JSContext* cx = top()->GetScriptInterface()->GetContext();
		JSAutoRequest rq(cx);
		JS::RootedValue global(cx, top()->GetGlobalObject());
		if (top()->GetScriptInterface()->CallFunction(global, "handleInputBeforeGui", *ev, top()->FindObjectUnderMouse(), handled))
			if (handled)
				return IN_HANDLED;
	}

	{
		PROFILE("handle event in native GUI");
		InReaction r = top()->HandleEvent(ev);
		if (r != IN_PASS)
			return r;
	}

	{
		// We can't take the following lines out of this scope because top() may be another gui page than it was when calling handleInputBeforeGui!
		JSContext* cx = top()->GetScriptInterface()->GetContext();
		JSAutoRequest rq(cx);
		JS::RootedValue global(cx, top()->GetGlobalObject());

		PROFILE("handleInputAfterGui");
		if (top()->GetScriptInterface()->CallFunction(global, "handleInputAfterGui", *ev, handled))
			if (handled)
				return IN_HANDLED;
	}

	return IN_PASS;
}


bool CGUIManager::GetPreDefinedColor(const CStr& name, CColor& output)
{
	return top()->GetPreDefinedColor(name, output);
}

IGUIObject* CGUIManager::FindObjectByName(const CStr& name) const
{
	// This can be called from scripts run by TickObjects,
	// and we want to return it the same GUI page as is being ticked
	if (m_CurrentGUI)
		return m_CurrentGUI->FindObjectByName(name);
	else
		return top()->FindObjectByName(name);
}

void CGUIManager::SendEventToAll(const CStr& eventName)
{
	top()->SendEventToAll(eventName);
}

void CGUIManager::TickObjects()
{
	PROFILE3("gui tick");

	// We share the script runtime with everything else that runs in the same thread.
	// This call makes sure we trigger GC regularly even if the simulation is not running.
	m_ScriptInterface->GetRuntime()->MaybeIncrementalGC(1.0f);
	
	// Save an immutable copy so iterators aren't invalidated by tick handlers
	PageStackType pageStack = m_PageStack;

	for (PageStackType::iterator it = pageStack.begin(); it != pageStack.end(); ++it)
	{
		m_CurrentGUI = it->gui;
		it->gui->TickObjects();
	}
	m_CurrentGUI.reset();
}

void CGUIManager::Draw()
{
	PROFILE3_GPU("gui");

	for (PageStackType::iterator it = m_PageStack.begin(); it != m_PageStack.end(); ++it)
		it->gui->Draw();
}

void CGUIManager::UpdateResolution()
{
	for (PageStackType::iterator it = m_PageStack.begin(); it != m_PageStack.end(); ++it)
		it->gui->UpdateResolution();
}

bool CGUIManager::TemplateExists(const std::string& templateName)
{
	return m_TemplateLoader.TemplateExists(templateName);
}

const CParamNode& CGUIManager::GetTemplate(const std::string& templateName)
{
	const CParamNode& templateRoot = m_TemplateLoader.GetTemplateFileData(templateName).GetChild("Entity");
	if (!templateRoot.IsOk())
		LOGERROR("Invalid template found for '%s'", templateName.c_str());

	return templateRoot;
}

// This returns a shared_ptr to make sure the CGUI doesn't get deallocated
// while we're in the middle of calling a function on it (e.g. if a GUI script
// calls SwitchPage)
shared_ptr<CGUI> CGUIManager::top() const
{
	ENSURE(m_PageStack.size());
	return m_PageStack.back().gui;
}
