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

#ifndef INCLUDED_GUIMANAGER
#define INCLUDED_GUIMANAGER

#include <boost/unordered_set.hpp>
#include <set>

#include "lib/input.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/TemplateLoader.h"
#include "scriptinterface/ScriptVal.h"
#include "scriptinterface/ScriptInterface.h"

class CGUI;
class JSObject;
class IGUIObject;
struct CGUIColor;
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
	void SwitchPage(const CStrW& name, ScriptInterface* srcScriptInterface, JS::HandleValue initData);

	/**
	 * Load a new GUI page and make it active. All current pages will be retained,
	 * and will still be drawn and receive tick events, but will not receive
	 * user inputs.
	 * If given, the callbackHandler function will be executed once this page is closed.
	 */
	void PushPage(const CStrW& pageName, shared_ptr<ScriptInterface::StructuredClone> initData, JS::HandleValue callbackFunc);

	/**
	 * Unload the currently active GUI page, and make the previous page active.
	 * (There must be at least two pages when you call this.)
	 */
	void PopPage(shared_ptr<ScriptInterface::StructuredClone> args);

	/**
	 * Called when a file has been modified, to hotload changes.
	 */
	Status ReloadChangedFile(const VfsPath& path);

	/**
	 * Sets the default mouse pointer.
	 */
	void ResetCursor();

	/**
	 * Called when we should reload all pages (e.g. translation hotloading update).
	 */
	Status ReloadAllPages();

	/**
	 * Pass input events to the currently active GUI page.
	 */
	InReaction HandleEvent(const SDL_Event_* ev);

	/**
	 * See CGUI::SendEventToAll; applies to the currently active page.
	 */
	void SendEventToAll(const CStr& eventName) const;
	void SendEventToAll(const CStr& eventName, JS::HandleValueArray paramData) const;

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
 	 */
	std::string GetSavedGameData();

	void RestoreSavedGameData(const std::string& jsonData);

	/**
	 * Check if a template with this name exists
	 */
	bool TemplateExists(const std::string& templateName) const;

	/**
	 * Retrieve the requested template, used for displaying faction specificities.
	 */
	const CParamNode& GetTemplate(const std::string& templateName);

private:
	struct SGUIPage
	{
		// COPYABLE, because event handlers may invalidate page stack iterators by open or close pages,
		// and event handlers need to be called for the entire stack.

		/**
		 * Initializes the data that will be used to create the CGUI page one or multiple times (hotloading).
		 */
		SGUIPage(const CStrW& pageName, const shared_ptr<ScriptInterface::StructuredClone> initData);

		/**
		 * Create the CGUI with it's own ScriptInterface. Deletes the previous CGUI if it existed.
		 */
		void LoadPage(shared_ptr<ScriptRuntime> scriptRuntime);

		/**
		 * Sets the callback handler when a new page is opened that will be performed when the page is closed.
		 */
		void SetCallbackFunction(ScriptInterface& scriptInterface, JS::HandleValue callbackFunc);

		/**
		 * Execute the stored callback function with the given arguments.
		 */
		void PerformCallbackFunction(shared_ptr<ScriptInterface::StructuredClone> args);

		CStrW name;
		boost::unordered_set<VfsPath> inputs; // for hotloading
		shared_ptr<ScriptInterface::StructuredClone> initData; // data to be passed to the init() function
		shared_ptr<CGUI> gui; // the actual GUI page

		/**
		 * Function executed by this parent GUI page when the child GUI page it pushed is popped.
		 * Notice that storing it in the SGUIPage instead of CGUI means that it will survive the hotloading CGUI reset.
		 */
		shared_ptr<JS::PersistentRootedValue> callbackFunction;
	};

	shared_ptr<CGUI> top() const;

	shared_ptr<ScriptRuntime> m_ScriptRuntime;
	shared_ptr<ScriptInterface> m_ScriptInterface;

	typedef std::vector<SGUIPage> PageStackType;
	PageStackType m_PageStack;

	CTemplateLoader m_TemplateLoader;
};

extern CGUIManager* g_GUI;

extern InReaction gui_handler(const SDL_Event_* ev);

#endif // INCLUDED_GUIMANAGER
