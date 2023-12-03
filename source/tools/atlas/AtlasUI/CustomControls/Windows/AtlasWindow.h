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

#ifndef INCLUDED_ATLASWINDOW
#define INCLUDED_ATLASWINDOW

#include "General/AtlasWindowCommandProc.h"

#include "General/IAtlasSerialiser.h"
#include "FileHistory/FileHistory.h"

#include "wx/filename.h"

#include <boost/version.hpp>
#if BOOST_VERSION >= 104000
# include <boost/signals2/signal.hpp>
#else
# error Atlas requires Boost 1.40 or later
#endif

class AtObj;

class AtlasWindow : public wxFrame, public IAtlasSerialiser
{
	friend class AtlasWindowCommandProc;

	DECLARE_CLASS(AtlasWindow);

public:

	enum
	{
		//	ID_Import,
		//	ID_Export,

		// First available ID for custom window-specific menu items
		ID_Custom = 1
	};

	AtlasWindow(wxWindow* parent, const wxString& title, const wxSize& size);
	boost::signals2::signal<void ()> sig_FileSaved;

private:

	void OnNew(wxCommandEvent& event);
//	void OnImport(wxCommandEvent& event);
//	void OnExport(wxCommandEvent& event);
	// TODO: import/export vs open/save/saveas - how should it decide which to do?
	void OnOpen(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnSaveAs(wxCommandEvent& event);

	void OnQuit(wxCommandEvent& event);

	void OnMRUFile(wxCommandEvent& event);

	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);

	void OnClose(wxCloseEvent& event);

protected:
	void SetCurrentFilename(wxFileName filename = wxString());
	wxFileName GetCurrentFilename() { return m_CurrentFilename; }

	void AddCustomMenu(wxMenu* menu, const wxString& title);

	virtual wxString GetDefaultOpenDirectory() = 0;

	bool SaveChanges(bool forceSaveAs);
public:
	bool OpenFile(const wxString& filename);

private:
	AtlasWindowCommandProc m_CommandProc;

	wxMenuItem* m_menuItem_Save;
	wxMenuBar* m_MenuBar;

	wxFileName m_CurrentFilename;
	wxString m_WindowTitle;

	FileHistory m_FileHistory;

	DECLARE_EVENT_TABLE();
};

#endif // INCLUDED_ATLASWINDOW
