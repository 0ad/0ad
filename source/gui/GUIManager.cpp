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

void CGUIManager::SwitchPage(const CStrW& pageName, ScriptInterface* srcScriptInterface, CScriptVal initData)
{
	// The page stack is cleared (including the script context where initData came from),
	// therefore we have to clone initData.
	shared_ptr<ScriptInterface::StructuredClone> initDataClone;
	if (initData.get() != JSVAL_VOID)
	{
		initDataClone = srcScriptInterface->WriteStructuredClone(initData.get());
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
		initDataVal.set(scriptInterface->ReadStructuredClone(initDataClone));
	else
	{
		LOGERROR(L"Called PopPageCB when initData (which should contain the callback function name) isn't set!");
		return;
	}
	
	if (!scriptInterface->HasProperty(initDataVal, "callback"))
	{
		LOGERROR(L"Called PopPageCB when the callback function name isn't set!");
		return;
	}
	
	std::string callback;
	if (!scriptInterface->GetProperty(initDataVal, "callback", callback))
	{
		LOGERROR(L"Failed to get the callback property as a string from initData in PopPageCB!");
		return;
	}
	
	JS::RootedValue global(cx, scriptInterface->GetGlobalObject());
	if (!scriptInterface->HasProperty(global, callback.c_str()))
	{
		LOGERROR(L"The specified callback function %hs does not exist in the page %ls", callback.c_str(), m_PageStack.back().name.c_str());
		return;
	}

	JS::RootedValue argVal(cx);
	if (args)
		argVal.set(scriptInterface->ReadStructuredClone(args));
	if (!scriptInterface->CallFunctionVoid(global, callback.c_str(), argVal))
	{
		LOGERROR(L"Failed to call the callback function %hs in the page %ls", callback.c_str(), m_PageStack.back().name.c_str());
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
		LOGERROR(L"GUI page '%ls' must have root element <page>", page.name.c_str());
		return;
	}

	XERO_ITER_EL(root, node)
	{
		if (node.GetNodeName() != elmt_include)
		{
			LOGERROR(L"GUI page '%ls' must only have <include> elements inside <page>", page.name.c_str());
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
		initDataVal.set(scriptInterface->ReadStructuredClone(page.initData));
	if (hotloadData)
		hotloadDataVal.set(scriptInterface->ReadStructuredClone(hotloadData));
	
	// Call the init() function
	if (!scriptInterface->CallFunctionVoid(
			global, 
			"init", 
			initDataVal, 
			hotloadDataVal)
		)
	{
		LOGERROR(L"GUI page '%ls': Failed to call init() function", page.name.c_str());
	}

	m_CurrentGUI = oldGUI;
}

Status CGUIManager::ReloadChangedFile(const VfsPath& path)
{
	for (PageStackType::iterator it = m_PageStack.begin(); it != m_PageStack.end(); ++it)
	{
		if (it->inputs.count(path))
		{
			LOGMESSAGE(L"GUI file '%ls' changed - reloading page '%ls'", path.string().c_str(), it->name.c_str());
			LoadPage(*it);
			// TODO: this can crash if LoadPage runs an init script which modifies the page stack and breaks our iterators
		}
	}

	return INFO::OK;
}

CScriptVal CGUIManager::GetSavedGameData(ScriptInterface*& pPageScriptInterface)
{
	JSContext* cx = top()->GetScriptInterface()->GetContext();
	JSAutoRequest rq(cx);
	
	JS::RootedValue global(cx, top()->GetGlobalObject());
	JS::RootedValue data(cx);
	if (!top()->GetScriptInterface()->CallFunction(global, "getSavedGameData", &data))
		LOGERROR(L"Failed to call getSavedGameData() on the current GUI page");
	pPageScriptInterface = GetScriptInterface().get();
	return CScriptVal(data);
}

std::string CGUIManager::GetSavedGameData()
{
	shared_ptr<ScriptInterface> scriptInterface = top()->GetScriptInterface();
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);
	
	JS::RootedValue data(cx);
	JS::RootedValue global(cx, top()->GetGlobalObject());
	scriptInterface->CallFunction(global, "getSavedGameData", &data);
	return scriptInterface->StringifyJSON(data, false);
}

void CGUIManager::RestoreSavedGameData(std::string jsonData)
{
	shared_ptr<ScriptInterface> scriptInterface = top()->GetScriptInterface();
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);
	
	JS::RootedValue global(cx, top()->GetGlobalObject());
	scriptInterface->CallFunctionVoid(global, "restoreSavedGameData", scriptInterface->ParseJSON(jsonData));
}

InReaction CGUIManager::HandleEvent(const SDL_Event_* ev)
{
	// We want scripts to have access to the raw input events, so they can do complex
	// processing when necessary (e.g. for unit selection and camera movement).
	// Sometimes they'll want to be layered behind the GUI widgets (e.g. to detect mousedowns on the
	// visible game area), sometimes they'll want to intercepts events before the GUI (e.g.
	// to capture all mouse events until a mouseup after dragging).
	// So we call two separate handler functions:
	
	shared_ptr<ScriptInterface> scriptInterface = top()->GetScriptInterface();
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue global(cx, top()->GetGlobalObject());
	bool handled;

	{
		PROFILE("handleInputBeforeGui");
		if (scriptInterface->CallFunction(global, "handleInputBeforeGui", *ev, top()->FindObjectUnderMouse(), handled))
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
		PROFILE("handleInputAfterGui");
		if (scriptInterface->CallFunction(global, "handleInputAfterGui", *ev, handled))
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

// This returns a shared_ptr to make sure the CGUI doesn't get deallocated
// while we're in the middle of calling a function on it (e.g. if a GUI script
// calls SwitchPage)
shared_ptr<CGUI> CGUIManager::top() const
{
	ENSURE(m_PageStack.size());
	return m_PageStack.back().gui;
}
