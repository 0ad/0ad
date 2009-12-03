#include "precompiled.h"

#include "GUIManager.h"

#include "CGUI.h"

#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "ps/XML/Xeromyces.h"

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

CGUIManager::CGUIManager()
{
}

CGUIManager::~CGUIManager()
{
}

void CGUIManager::SwitchPage(const CStrW& pageName, jsval initData)
{
	m_PageStack.clear();
	PushPage(pageName, initData);
}

void CGUIManager::PushPage(const CStrW& pageName, jsval initData)
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


void CGUIManager::LoadPage(SGUIPage& page)
{
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
	jsval rval;
	if (! JS_CallFunctionName(g_ScriptingHost.GetContext(), page.gui->GetScriptObject(), "init", 1, &page.initData, &rval))
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

InReaction CGUIManager::HandleEvent(const SDL_Event_* ev)
{
	return top()->HandleEvent(ev);
}

// This returns a shared_ptr to make sure the CGUI doesn't get deallocated
// while we're in the middle of calling a function on it (e.g. if a GUI script
// calls SwitchPage)
shared_ptr<CGUI> CGUIManager::top() const
{
	debug_assert(m_PageStack.size());
	return m_PageStack.back().gui;
}
