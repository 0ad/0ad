/* Copyright (C) 2018 Wildfire Games.
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

#ifndef INCLUDED_MAPDIALOG
#define INCLUDED_MAPDIALOG

#include <wx/dialog.h>
#include <wx/version.h>

#if !wxCHECK_VERSION(2, 9, 0)
#define wxBookCtrlEvent wxNotebookEvent
#endif
class wxBookCtrlEvent;

enum MapDialogType { MAPDIALOG_OPEN, MAPDIALOG_SAVE };

class MapDialog : public wxDialog
{
public:
	MapDialog(wxWindow* parent, MapDialogType type, const wxIcon& icon);

	/**
	 * Returns VFS path of the selected map with .xml extension, else empty string
	 */
	wxString GetSelectedFilePath() const;

private:

	void OnCancel(wxCommandEvent& evt);
	void OnOpen(wxCommandEvent& evt);
	void OnSave(wxCommandEvent& evt);
	void OnListBox(wxCommandEvent& evt);
	void OnFilename(wxCommandEvent& evt);
	void OnNotebookChanged(wxBookCtrlEvent& evt);

	void OpenFile();
	void SaveFile();

	wxString m_FileName;
	MapDialogType m_Type;

	DECLARE_EVENT_TABLE();
};

#endif // INCLUDED_MAPDIALOG
