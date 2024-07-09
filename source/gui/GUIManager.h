/* Copyright (C) 2024 Wildfire Games.
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

#include "lib/file/vfs/vfs_path.h"
#include "lib/input.h"
#include "ps/CStr.h"
#include "ps/TemplateLoader.h"
#include "scriptinterface/StructuredClone.h"

#include <deque>
#include <string>
#include <unordered_set>

class CCanvas2D;
class CGUI;

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
	CGUIManager(ScriptContext& scriptContext, ScriptInterface& scriptInterface);
	~CGUIManager();

	ScriptInterface& GetScriptInterface()
	{
		return m_ScriptInterface;
	}
	ScriptContext& GetContext() { return m_ScriptContext; }
	std::shared_ptr<CGUI> GetActiveGUI() { return top(); }

	/**
	 * Returns the number of currently open GUI pages.
	 */
	size_t GetPageCount() const;

	/**
	 * Load a new GUI page and make it active. All current pages will be destroyed.
	 */
	void SwitchPage(const CStrW& name, const ScriptInterface* srcScriptInterface, JS::HandleValue initData);

	/**
	 * Load a new GUI page and make it active. All current pages will be retained,
	 * and will still be drawn and receive tick events, but will not receive
	 * user inputs.
	 * The returned promise will be fulfilled once the pushed page is closed.
	 */
	JS::Value PushPage(const CStrW& pageName, Script::StructuredClone initData);

	/**
	 * Unload the currently active GUI page, and make the previous page active.
	 * (There must be at least two pages when you call this.)
	 */
	void PopPage(Script::StructuredClone args);

	/**
	 * Called when a file has been modified, to hotload changes.
	 */
	Status ReloadChangedFile(const VfsPath& path);

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
	void Draw(CCanvas2D& canvas) const;

	/**
	 * See CGUI::UpdateResolution; applies to @em all loaded pages.
	 */
	void UpdateResolution();

	/**
	 * Check if a template with this name exists
	 */
	bool TemplateExists(const std::string& templateName) const;

	/**
	 * Retrieve the requested template, used for displaying faction specificities.
	 */
	const CParamNode& GetTemplate(const std::string& templateName);

	/**
	 * Display progress / description in loading screen.
	 */
	void DisplayLoadProgress(int percent, const wchar_t* pending_task);

private:
	struct SGUIPage
	{
		// COPYABLE, because event handlers may invalidate page stack iterators by open or close pages,
		// and event handlers need to be called for the entire stack.

		/**
		 * Initializes the data that will be used to create the CGUI page one or multiple times (hotloading).
		 */
		SGUIPage(const CStrW& pageName, const Script::StructuredClone initData);

		/**
		 * Create the CGUI with it's own ScriptInterface. Deletes the previous CGUI if it existed.
		 */
		void LoadPage(ScriptContext& scriptContext);

		/**
		 * A new promise gets set. A reference to that promise is returned. The promise will settle when
		 * the page is closed.
		 */
		JS::Value ReplacePromise(ScriptInterface& scriptInterface);

		/**
		 * Execute the stored callback function with the given arguments.
		 */
		void ResolvePromise(Script::StructuredClone args);

		std::wstring m_Name;
		std::unordered_set<VfsPath> inputs; // for hotloading
		Script::StructuredClone initData; // data to be passed to the init() function
		std::shared_ptr<CGUI> gui; // the actual GUI page

		/**
		 * Function executed by this parent GUI page when the child GUI page it pushed is popped.
		 * Notice that storing it in the SGUIPage instead of CGUI means that it will survive the hotloading CGUI reset.
		 */
		std::shared_ptr<JS::PersistentRootedObject> callbackFunction;
	};

	std::shared_ptr<CGUI> top() const;

	ScriptContext& m_ScriptContext;
	ScriptInterface& m_ScriptInterface;

	/**
	 * The page stack must not move pointers on push/pop, or pushing a page in a page's init method
	 * may crash (as the pusher page will suddenly have moved, and the stack will be confused).
	 * Therefore use std::deque over std::vector.
	 * Also the elements have to be destructed back to front.
	 */
	class PageStackType : public std::deque<SGUIPage>
	{
	public:
		~PageStackType()
		{
			clear();
		}

		void clear()
		{
			while (!std::deque<SGUIPage>::empty())
				std::deque<SGUIPage>::pop_back();
		}
	};
	PageStackType m_PageStack;

	CTemplateLoader m_TemplateLoader;
};

extern CGUIManager* g_GUI;

extern InReaction gui_handler(const SDL_Event_* ev);

#endif // INCLUDED_GUIMANAGER
