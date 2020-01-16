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

#include "GUIManager.h"

#include "gui/CGUI.h"
#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/Config.h"
#include "ps/Profile.h"
#include "ps/XML/Xeromyces.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRuntime.h"

CGUIManager* g_GUI = nullptr;

const CStr CGUIManager::EventNameWindowResized = "WindowResized";


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
	if (!g_GUI)
		return IN_PASS;

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

	if (!CXeromyces::AddValidator(g_VFS, "gui_page", "gui/gui_page.rng"))
		LOGERROR("CGUIManager: failed to load GUI page grammar file 'gui/gui_page.rng'");
	if (!CXeromyces::AddValidator(g_VFS, "gui", "gui/gui.rng"))
		LOGERROR("CGUIManager: failed to load GUI XML grammar file 'gui/gui.rng'");

	RegisterFileReloadFunc(ReloadChangedFileCB, this);
}

CGUIManager::~CGUIManager()
{
	UnregisterFileReloadFunc(ReloadChangedFileCB, this);
}

size_t CGUIManager::GetPageCount() const
{
	return m_PageStack.size();
}

void CGUIManager::SwitchPage(const CStrW& pageName, ScriptInterface* srcScriptInterface, JS::HandleValue initData)
{
	// The page stack is cleared (including the script context where initData came from),
	// therefore we have to clone initData.

	shared_ptr<ScriptInterface::StructuredClone> initDataClone;
	if (!initData.isUndefined())
		initDataClone = srcScriptInterface->WriteStructuredClone(initData);

	m_PageStack.clear();

	PushPage(pageName, initDataClone, JS::UndefinedHandleValue);
}

void CGUIManager::PushPage(const CStrW& pageName, shared_ptr<ScriptInterface::StructuredClone> initData, JS::HandleValue callbackFunction)
{
	// Store the callback handler in the current GUI page before opening the new one
	if (!m_PageStack.empty() && !callbackFunction.isUndefined())
		m_PageStack.back().SetCallbackFunction(*m_ScriptInterface, callbackFunction);

	// Push the page prior to loading its contents, because that may push
	// another GUI page on init which should be pushed on top of this new page.
	m_PageStack.emplace_back(pageName, initData);
	m_PageStack.back().LoadPage(m_ScriptRuntime);
}

void CGUIManager::PopPage(shared_ptr<ScriptInterface::StructuredClone> args)
{
	if (m_PageStack.size() < 2)
	{
		debug_warn(L"Tried to pop GUI page when there's < 2 in the stack");
		return;
	}

	m_PageStack.pop_back();
	m_PageStack.back().PerformCallbackFunction(args);
}

CGUIManager::SGUIPage::SGUIPage(const CStrW& pageName, const shared_ptr<ScriptInterface::StructuredClone> initData)
	: name(pageName), initData(initData), inputs(), gui(), callbackFunction()
{
}

void CGUIManager::SGUIPage::LoadPage(shared_ptr<ScriptRuntime> scriptRuntime)
{
	// If we're hotloading then try to grab some data from the previous page
	shared_ptr<ScriptInterface::StructuredClone> hotloadData;
	if (gui)
	{
		shared_ptr<ScriptInterface> scriptInterface = gui->GetScriptInterface();
		JSContext* cx = scriptInterface->GetContext();
		JSAutoRequest rq(cx);

		JS::RootedValue global(cx, scriptInterface->GetGlobalObject());
		JS::RootedValue hotloadDataVal(cx);
		scriptInterface->CallFunction(global, "getHotloadData", &hotloadDataVal);
		hotloadData = scriptInterface->WriteStructuredClone(hotloadDataVal);
	}

	g_CursorName = g_DefaultCursor;
	inputs.clear();
	gui.reset(new CGUI(scriptRuntime));

	gui->AddObjectTypes();

	VfsPath path = VfsPath("gui") / name;
	inputs.insert(path);

	CXeromyces xero;
	if (xero.Load(g_VFS, path, "gui_page") != PSRETURN_OK)
		// Fail silently (Xeromyces reported the error)
		return;

	int elmt_page = xero.GetElementID("page");
	int elmt_include = xero.GetElementID("include");

	XMBElement root = xero.GetRoot();

	if (root.GetNodeName() != elmt_page)
	{
		LOGERROR("GUI page '%s' must have root element <page>", utf8_from_wstring(name));
		return;
	}

	XERO_ITER_EL(root, node)
	{
		if (node.GetNodeName() != elmt_include)
		{
			LOGERROR("GUI page '%s' must only have <include> elements inside <page>", utf8_from_wstring(name));
			continue;
		}

		std::string name = node.GetText();
		CStrW nameW (node.GetText().FromUTF8());

		PROFILE2("load gui xml");
		PROFILE2_ATTR("name: %s", name.c_str());

		TIMER(nameW.c_str());
		if (name.back() == '/')
		{
			VfsPath directory = VfsPath("gui") / nameW;
			VfsPaths pathnames;
			vfs::GetPathnames(g_VFS, directory, L"*.xml", pathnames);
			for (const VfsPath& path : pathnames)
				gui->LoadXmlFile(path, inputs);
		}
		else
		{
			VfsPath path = VfsPath("gui") / nameW;
			gui->LoadXmlFile(path, inputs);
		}
	}

	gui->LoadedXmlFiles();

	shared_ptr<ScriptInterface> scriptInterface = gui->GetScriptInterface();
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue initDataVal(cx);
	JS::RootedValue hotloadDataVal(cx);
	JS::RootedValue global(cx, scriptInterface->GetGlobalObject());

	if (initData)
		scriptInterface->ReadStructuredClone(initData, &initDataVal);

	if (hotloadData)
		scriptInterface->ReadStructuredClone(hotloadData, &hotloadDataVal);

	if (scriptInterface->HasProperty(global, "init") &&
	    !scriptInterface->CallFunctionVoid(global, "init", initDataVal, hotloadDataVal))
		LOGERROR("GUI page '%s': Failed to call init() function", utf8_from_wstring(name));
}

void CGUIManager::SGUIPage::SetCallbackFunction(ScriptInterface& scriptInterface, JS::HandleValue callbackFunc)
{
	if (!callbackFunc.isObject())
	{
		LOGERROR("Given callback handler is not an object!");
		return;
	}

	// Does not require JSAutoRequest
	if (!JS_ObjectIsFunction(scriptInterface.GetContext(), &callbackFunc.toObject()))
	{
		LOGERROR("Given callback handler is not a function!");
		return;
	}

	callbackFunction = std::make_shared<JS::PersistentRootedValue>(scriptInterface.GetJSRuntime(), callbackFunc);
}

void CGUIManager::SGUIPage::PerformCallbackFunction(shared_ptr<ScriptInterface::StructuredClone> args)
{
	if (!callbackFunction)
		return;

	shared_ptr<ScriptInterface> scriptInterface = gui->GetScriptInterface();
	JSContext* cx = scriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedObject globalObj(cx, &scriptInterface->GetGlobalObject().toObject());

	JS::RootedValue funcVal(cx, *callbackFunction);

	// Delete the callback function, so that it is not called again
	callbackFunction.reset();

	JS::RootedValue argVal(cx);
	if (args)
		scriptInterface->ReadStructuredClone(args, &argVal);

	JS::AutoValueVector paramData(cx);
	paramData.append(argVal);

	JS::RootedValue result(cx);

	JS_CallFunctionValue(cx, globalObj, funcVal, paramData, &result);
}

Status CGUIManager::ReloadChangedFile(const VfsPath& path)
{
	for (SGUIPage& p : m_PageStack)
		if (p.inputs.find(path) != p.inputs.end())
		{
			LOGMESSAGE("GUI file '%s' changed - reloading page '%s'", path.string8(), utf8_from_wstring(p.name));
			p.LoadPage(m_ScriptRuntime);
			// TODO: this can crash if LoadPage runs an init script which modifies the page stack and breaks our iterators
		}

	return INFO::OK;
}

Status CGUIManager::ReloadAllPages()
{
	// TODO: this can crash if LoadPage runs an init script which modifies the page stack and breaks our iterators
	for (SGUIPage& p : m_PageStack)
		p.LoadPage(m_ScriptRuntime);

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

	bool handled = false;

	{
		PROFILE("handleInputBeforeGui");
		JSContext* cx = top()->GetScriptInterface()->GetContext();
		JSAutoRequest rq(cx);
		JS::RootedValue global(cx, top()->GetGlobalObject());
		if (top()->GetScriptInterface()->CallFunction(global, "handleInputBeforeGui", handled, *ev, top()->FindObjectUnderMouse()))
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
		if (top()->GetScriptInterface()->CallFunction(global, "handleInputAfterGui", handled, *ev))
			if (handled)
				return IN_HANDLED;
	}

	return IN_PASS;
}

void CGUIManager::SendEventToAll(const CStr& eventName) const
{
	// Save an immutable copy so iterators aren't invalidated by handlers
	PageStackType pageStack = m_PageStack;

	for (const SGUIPage& p : pageStack)
		p.gui->SendEventToAll(eventName);

}

void CGUIManager::SendEventToAll(const CStr& eventName, JS::HandleValueArray paramData) const
{
	// Save an immutable copy so iterators aren't invalidated by handlers
	PageStackType pageStack = m_PageStack;

	for (const SGUIPage& p : pageStack)
		p.gui->SendEventToAll(eventName, paramData);
}

void CGUIManager::TickObjects()
{
	PROFILE3("gui tick");

	// We share the script runtime with everything else that runs in the same thread.
	// This call makes sure we trigger GC regularly even if the simulation is not running.
	m_ScriptInterface->GetRuntime()->MaybeIncrementalGC(1.0f);

	// Save an immutable copy so iterators aren't invalidated by tick handlers
	PageStackType pageStack = m_PageStack;

	for (const SGUIPage& p : pageStack)
		p.gui->TickObjects();
}

void CGUIManager::Draw() const
{
	PROFILE3_GPU("gui");

	for (const SGUIPage& p : m_PageStack)
		p.gui->Draw();
}

void CGUIManager::UpdateResolution()
{
	// Save an immutable copy so iterators aren't invalidated by event handlers
	PageStackType pageStack = m_PageStack;

	for (const SGUIPage& p : pageStack)
	{
		p.gui->UpdateResolution();
		p.gui->SendEventToAll(EventNameWindowResized);
	}
}

bool CGUIManager::TemplateExists(const std::string& templateName) const
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
