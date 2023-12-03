/* Copyright (C) 2009 Wildfire Games.
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

#ifndef INCLUDED_ATLASDIALOG
#define INCLUDED_ATLASDIALOG

#include "wx/dialog.h"

#include "General/AtlasWindowCommandProc.h"
#include "General/IAtlasSerialiser.h"

class FieldEditCtrl_Dialog;

class AtlasDialog : public wxDialog, public IAtlasSerialiser
{
	friend class FieldEditCtrl_Dialog;
	friend class AtlasWindowCommandProc;

	DECLARE_CLASS(AtlasDialog);

public:
	AtlasDialog(wxWindow* parent, const wxString& title, const wxSize& size);
	virtual ~AtlasDialog() {}

private:
	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);

protected:
	wxPanel* m_MainPanel;

private:
	AtlasWindowCommandProc m_CommandProc;

	DECLARE_EVENT_TABLE()
};

#endif // INCLUDED_ATLASDIALOG
