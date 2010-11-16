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

#ifndef INCLUDED_GUIMANAGER
#define INCLUDED_GUIMANAGER

#include "lib/input.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"
#include "scriptinterface/ScriptVal.h"

#include <set>

class CGUI;
struct JSObject;
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
	CGUIManager(ScriptInterface& scriptInterface);
	~CGUIManager();

	ScriptInterface& GetScriptInterface() { return m_ScriptInterface; }

	/**
	 * Returns whether there are any current pages.
	 */
	bool HasPages();

	/**
	 * Load a new GUI page and make it active. All current pages will be destroyed.
	 */
	void SwitchPage(const CStrW& name, CScriptVal initData);

	/**
	 * Load a new GUI page and make it active. All current pages will be retained,
	 * and will still be drawn and receive tick events, but will not receive
	 * user inputs.
	 */
	void PushPage(const CStrW& name, CScriptVal initData);

	/**
	 * Unload the currently active GUI page, and make the previous page active.
	 * (There must be at least two pages when you call this.)
	 */
	void PopPage();

	/**
	 * Display a modal message box with an "OK" button.
	 */
	void DisplayMessageBox(int width, int height, const CStrW& title, const CStrW& message);

	/**
	 * Call when a file has bee modified, to hotload pages if their .xml files changed.
	 */
	LibError ReloadChangedFiles(const VfsPath& path);

	/**
	 * Pass input events to the currently active GUI page.
	 */
	InReaction HandleEvent(const SDL_Event_* ev);

	/**
	 * See CGUI::GetPreDefinedColor; applies to the currently active page.
	 */
	bool GetPreDefinedColor(const CStr& name, CColor& output);

	/**
	 * See CGUI::IconExists; applies to the currently active page.
	 */
	bool IconExists(const CStr& str) const;

	/**
	 * See CGUI::GetIcon; applies to the currently active page.
	 */
	SGUIIcon GetIcon(const CStr& str) const;

	/**
	 * See CGUI::FindObjectByName; applies to the currently active page.
	 */
	IGUIObject* FindObjectByName(const CStr& name) const;

	/**
	 * See CGUI::SendEventToAll; applies to the currently active page.
	 */
	void SendEventToAll(const CStr& eventName);

	/**
	 * See CGUI::GetScriptObject; applies to the currently active page.
	 */
	JSObject* GetScriptObject();

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

private:
	struct SGUIPage
	{
		CStrW name;
		std::set<VfsPath> inputs; // for hotloading

		JSContext* cx;
		CScriptValRooted initData; // data to be passed to the init() function

		shared_ptr<CGUI> gui; // the actual GUI page
	};

	void LoadPage(SGUIPage& page);

	shared_ptr<CGUI> top() const;

	typedef std::vector<SGUIPage> PageStackType;
	PageStackType m_PageStack;

	shared_ptr<CGUI> m_CurrentGUI; // used to latch state during TickObjects/LoadPage (this is kind of ugly)

	ScriptInterface& m_ScriptInterface;
};

extern CGUIManager* g_GUI;

extern InReaction gui_handler(const SDL_Event_* ev);

#endif // INCLUDED_GUIMANAGER
