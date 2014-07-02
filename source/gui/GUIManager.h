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

#ifndef INCLUDED_GUIMANAGER
#define INCLUDED_GUIMANAGER

#include <boost/unordered_set.hpp>

#include "lib/input.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "scriptinterface/ScriptVal.h"
#include "scriptinterface/ScriptInterface.h"

#include <set>

class CGUI;
class JSObject;
class IGUIObject;
struct CColor;
struct SGUIIcon;

/**
 * External interface to the GUI system.
 *
 * The GUI consists of a set of pages. Each page is constructed from a
 * series of XML files, and is independent from any other page.
 * Only one page is active at a time. All events and render requests etc
 * will go to the active page. This lets the GUI switch between pre-game menu
 * and in-game UI.
 */
class CGUIManager
{
	NONCOPYABLE(CGUIManager);
public:
	CGUIManager();
	~CGUIManager();

	shared_ptr<ScriptInterface> GetScriptInterface() 
	{ 
		return m_ScriptInterface; 
	}
	shared_ptr<ScriptRuntime> GetRuntime() { return m_ScriptRuntime; }
	shared_ptr<CGUI> GetActiveGUI() { return top(); }

	/**
	 * Returns whether there are any current pages.
	 */
	bool HasPages();

	/**
	 * Load a new GUI page and make it active. All current pages will be destroyed.
	 */
	void SwitchPage(const CStrW& name, ScriptInterface* srcScriptInterface, CScriptVal initData);

	/**
	 * Load a new GUI page and make it active. All current pages will be retained,
	 * and will still be drawn and receive tick events, but will not receive
	 * user inputs.
	 */
	void PushPage(const CStrW& pageName, shared_ptr<ScriptInterface::StructuredClone> initData);

	/**
	 * Unload the currently active GUI page, and make the previous page active.
	 * (There must be at least two pages when you call this.)
	 */
	void PopPage();
	void PopPageCB(shared_ptr<ScriptInterface::StructuredClone> args);

	/**
	 * Display a modal message box with an "OK" button.
	 */
	void DisplayMessageBox(int width, int height, const CStrW& title, const CStrW& message);

	/**
	 * Called when a file has been modified, to hotload changes.
	 */
	Status ReloadChangedFile(const VfsPath& path);

	/**
	 * Pass input events to the currently active GUI page.
	 */
	InReaction HandleEvent(const SDL_Event_* ev);

	/**
	 * See CGUI::GetPreDefinedColor; applies to the currently active page.
	 */
	bool GetPreDefinedColor(const CStr& name, CColor& output);

	/**
	 * See CGUI::FindObjectByName; applies to the currently active page.
	 */
	IGUIObject* FindObjectByName(const CStr& name) const;

	/**
	 * See CGUI::SendEventToAll; applies to the currently active page.
	 */
	void SendEventToAll(const CStr& eventName);

	/**
	 * See CGUI::TickObjects; applies to @em all loaded pages.
	 */
	void TickObjects();

	/**
	 * See CGUI::Draw; applies to @em all loaded pages.
	 */
	void Draw();

	/**
	 * See CGUI::UpdateResolution; applies to @em all loaded pages.
	 */
	void UpdateResolution();

 	/**
 	 * Calls the current page's script function getSavedGameData() and returns the result.
	 * The first overload also returns a pointer to the ScriptInterface the returned CScriptVal belongs to.
 	 */
	CScriptVal GetSavedGameData(ScriptInterface*& pPageScriptInterface);
	std::string GetSavedGameData();
 
	void RestoreSavedGameData(std::string jsonData);

private:
	struct SGUIPage
	{
		CStrW name;
		boost::unordered_set<VfsPath> inputs; // for hotloading

		JSContext* cx;
		shared_ptr<ScriptInterface::StructuredClone> initData; // data to be passed to the init() function
		CStrW callbackPageName;

		shared_ptr<CGUI> gui; // the actual GUI page
	};

	void LoadPage(SGUIPage& page);

	shared_ptr<CGUI> top() const;
	
	shared_ptr<CGUI> m_CurrentGUI; // used to latch state during TickObjects/LoadPage (this is kind of ugly)
	shared_ptr<ScriptRuntime> m_ScriptRuntime;
	shared_ptr<ScriptInterface> m_ScriptInterface;

	typedef std::vector<SGUIPage> PageStackType;
	PageStackType m_PageStack;
};

extern CGUIManager* g_GUI;

extern InReaction gui_handler(const SDL_Event_* ev);

#endif // INCLUDED_GUIMANAGER
