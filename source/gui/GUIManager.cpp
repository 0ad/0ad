/* Copyright (C) 2010 Wildfire Games.
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
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "ps/XML/Xeromyces.h"
#include "scripting/ScriptingHost.h"
#include "scriptinterface/ScriptInterface.h"

CGUIManager* g_GUI = NULL;


// General TODOs:
//
// A lot of the CGUI data could (and should) be shared between
// multiple pages, instead of treating them as completely independent, to save
// memory and loading time.
//
// Hotkeys are not unregistered when a page is unloaded.


// called from main loop when (input) events are received.
// event is passed to other handlers if false is returned.
// trampoline: we don't want to make the HandleEvent implementation static
InReaction gui_handler(const SDL_Event_* ev)
{
	PROFILE("GUI event handler");
	return g_GUI->HandleEvent(ev);
}

CGUIManager::SGUIPage::SGUIPage()
{
	JS_AddNamedRoot(g_ScriptingHost.GetContext(), &initData, "SGUIPage initData");
}

CGUIManager::SGUIPage::SGUIPage(const SGUIPage& page)
{
	*this = page;
	JS_AddNamedRoot(g_ScriptingHost.GetContext(), &initData, "SGUIPage initData copy");
}

CGUIManager::SGUIPage::~SGUIPage()
{
	JS_RemoveRoot(g_ScriptingHost.GetContext(), &initData);
}

CGUIManager::CGUIManager(ScriptInterface& scriptInterface) :
	m_ScriptInterface(scriptInterface)
{
	debug_assert(ScriptInterface::GetCallbackData(scriptInterface.GetContext()) == NULL);
	scriptInterface.SetCallbackData(this);
}

CGUIManager::~CGUIManager()
{
}

void CGUIManager::SwitchPage(const CStrW& pageName, CScriptVal initData)
{
	m_PageStack.clear();
	PushPage(pageName, initData);
}

void CGUIManager::PushPage(const CStrW& pageName, CScriptVal initData)
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

void CGUIManager::DisplayMessageBox(int width, int height, const CStrW& title, const CStrW& message)
{
	// Set up scripted init data for the standard message box window
	CScriptValRooted data;
	m_ScriptInterface.Eval("({})", data);
	m_ScriptInterface.SetProperty(data.get(), "width", width, false);
	m_ScriptInterface.SetProperty(data.get(), "height", height, false);
	m_ScriptInterface.SetProperty(data.get(), "mode", 2, false);
	m_ScriptInterface.SetProperty(data.get(), "font", std::string("verdana16"), false);
	m_ScriptInterface.SetProperty(data.get(), "title", std::wstring(title), false);
	m_ScriptInterface.SetProperty(data.get(), "message", std::wstring(message), false);

	// Display the message box
	PushPage(L"page_msgbox.xml", data.get());
}

void CGUIManager::LoadPage(SGUIPage& page)
{
	// If we're hotloading then try to grab some data from the previous page
	CScriptValRooted hotloadData;
	if (page.gui)
		m_ScriptInterface.CallFunction(OBJECT_TO_JSVAL(page.gui->GetScriptObject()), "getHotloadData", hotloadData);

	page.inputs.clear();
	page.gui.reset(new CGUI());
	page.gui->Initialize();

	VfsPath path = VfsPath(L"gui")/page.name.c_str();
	page.inputs.insert(path);

	CXeromyces xero;
	if (xero.Load(path) != PSRETURN_OK)
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

		CStrW name (node.GetText());
		TIMER(name.c_str());
		VfsPath path (VfsPath(L"gui")/name.c_str());
		page.gui->LoadXmlFile(path, page.inputs);
	}

	page.gui->SendEventToAll("load");

	// Call the init() function
	if (!m_ScriptInterface.CallFunctionVoid(OBJECT_TO_JSVAL(page.gui->GetScriptObject()), "init", page.initData, hotloadData))
	{
		LOGERROR(L"GUI page '%ls': Failed to call init() function", page.name.c_str());
	}
}

LibError CGUIManager::ReloadChangedFiles(const VfsPath& path)
{
	for (PageStackType::iterator it = m_PageStack.begin(); it != m_PageStack.end(); ++it)
	{
		if (it->inputs.count(path))
		{
			LOGMESSAGE(L"GUI file '%ls' changed - reloading page '%ls'", path.string().c_str(), it->name.c_str());
			LoadPage(*it);
		}
	}

	return INFO::OK;
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
		if (m_ScriptInterface.CallFunction(OBJECT_TO_JSVAL(top()->GetScriptObject()), "handleInputBeforeGui", *ev, handled))
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
		if (m_ScriptInterface.CallFunction(OBJECT_TO_JSVAL(top()->GetScriptObject()), "handleInputAfterGui", *ev, handled))
			if (handled)
				return IN_HANDLED;
	}

	return IN_PASS;
}


bool CGUIManager::GetPreDefinedColor(const CStr& name, CColor& output)
{
	return top()->GetPreDefinedColor(name, output);
}

bool CGUIManager::IconExists(const CStr& str) const
{
	return top()->IconExists(str);
}

SGUIIcon CGUIManager::GetIcon(const CStr& str) const
{
	return top()->GetIcon(str);
}

IGUIObject* CGUIManager::FindObjectByName(const CStr& name) const
{
	return top()->FindObjectByName(name);
}

void CGUIManager::SendEventToAll(const CStr& eventName)
{
	top()->SendEventToAll(eventName);
}

void CGUIManager::TickObjects()
{
	for (PageStackType::iterator it = m_PageStack.begin(); it != m_PageStack.end(); ++it)
		it->gui->TickObjects();
}

void CGUIManager::Draw()
{
	for (PageStackType::iterator it = m_PageStack.begin(); it != m_PageStack.end(); ++it)
		it->gui->Draw();
}

void CGUIManager::UpdateResolution()
{
	for (PageStackType::iterator it = m_PageStack.begin(); it != m_PageStack.end(); ++it)
		it->gui->UpdateResolution();
}

JSObject* CGUIManager::GetScriptObject()
{
	return top()->GetScriptObject();
}

// This returns a shared_ptr to make sure the CGUI doesn't get deallocated
// while we're in the middle of calling a function on it (e.g. if a GUI script
// calls SwitchPage)
shared_ptr<CGUI> CGUIManager::top() const
{
	debug_assert(m_PageStack.size());
	return m_PageStack.back().gui;
}
