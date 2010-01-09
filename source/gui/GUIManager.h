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

	// Load a new GUI page and make it active. All current pages will be destroyed.
	void SwitchPage(const CStrW& name, CScriptVal initData);

	// Load a new GUI page and make it active. All current pages will be retained,
	// and will still be drawn and receive tick events, but will not receive
	// user inputs.
	void PushPage(const CStrW& name, CScriptVal initData);

	// Unload the currently active GUI page, and make the previous page active.
	// (There must be at least two pages when you call this.)
	void PopPage();

	// Hotload pages when their .xml files have changed
	LibError ReloadChangedFiles(const VfsPath& path);

	// Handle input events
	InReaction HandleEvent(const SDL_Event_* ev);

	// These functions are all equivalent to the CGUI functions of the same
	// name, applied to the currently active GUI page:

	bool GetPreDefinedColor(const CStr& name, CColor& output);
	bool IconExists(const CStr& str) const;
	SGUIIcon GetIcon(const CStr& str) const;

	IGUIObject* FindObjectByName(const CStr& name) const;

	void SendEventToAll(const CStr& eventName);
	void TickObjects();
	void Draw();
	void UpdateResolution();

	JSObject* GetScriptObject();

private:
	struct SGUIPage
	{
		SGUIPage();
		SGUIPage(const SGUIPage&);
		~SGUIPage();

		CStrW name;
		std::set<VfsPath> inputs; // for hotloading

		JSContext* cx;
		CScriptVal initData; // data to be passed to the init() function

		shared_ptr<CGUI> gui; // the actual GUI page
	};

	void LoadPage(SGUIPage& page);

	shared_ptr<CGUI> top() const;

	typedef std::vector<SGUIPage> PageStackType;
	PageStackType m_PageStack;

	ScriptInterface& m_ScriptInterface;
};

extern CGUIManager* g_GUI;

extern InReaction gui_handler(const SDL_Event_* ev);

#endif // INCLUDED_GUIMANAGER
