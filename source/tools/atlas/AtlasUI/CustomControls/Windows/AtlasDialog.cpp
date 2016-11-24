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

#include "precompiled.h"

#include "AtlasDialog.h"

#include "wx/statline.h"

IMPLEMENT_CLASS(AtlasDialog, wxDialog);

BEGIN_EVENT_TABLE(AtlasDialog, wxDialog)
	EVT_MENU(wxID_UNDO, AtlasDialog::OnUndo)
	EVT_MENU(wxID_REDO, AtlasDialog::OnRedo)
END_EVENT_TABLE()


AtlasDialog::AtlasDialog(wxWindow* parent, const wxString& title, const wxSize& size)
	: wxDialog(parent, -1, title, wxDefaultPosition, size,
			   wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCLOSE_BOX)
{
	// Create generic dialog box, with OK/Cancel buttons, some horizontal
	// dividing lines, and a wxPanel in the middle:

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(mainSizer);

	// -------------------------------------------------------------------------
	mainSizer->Add(new wxStaticLine(this, -1), wxSizerFlags().Expand().Border(wxALL, 5));


	m_MainPanel = new wxPanel(this);
	mainSizer->Add(m_MainPanel, wxSizerFlags().Proportion(1).Expand().Border(wxLEFT|wxRIGHT, 5));

	// -------------------------------------------------------------------------
	mainSizer->Add(new wxStaticLine(this, -1), wxSizerFlags().Expand().Border(wxALL, 5));


	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(buttonSizer, wxSizerFlags().Expand().Align(wxALIGN_RIGHT).Border(wxALL, 5));

	buttonSizer->Add(new wxButton(this, wxID_OK, _("OK")), wxSizerFlags().Border(wxRIGHT, 25));
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, _("Cancel")), wxSizerFlags().Border(wxRIGHT, 5));

	//////////////////////////////////////////////////////////////////////////

	// Set up handlers for Ctrl+Z, Ctrl+Y (undo/redo), since dialogs don't
	// have any menu entries for them (since they don't have menus at all).
	wxAcceleratorEntry entries[2];
	entries[0].Set(wxACCEL_CTRL, 'Z', wxID_UNDO);
	entries[1].Set(wxACCEL_CTRL, 'Y', wxID_REDO);
	wxAcceleratorTable accel(2, entries);
	SetAcceleratorTable(accel);

	m_CommandProc.Initialize();
}

void AtlasDialog::OnUndo(wxCommandEvent& WXUNUSED(event))
{
	m_CommandProc.Undo();
}

void AtlasDialog::OnRedo(wxCommandEvent& WXUNUSED(event))
{
	m_CommandProc.Redo();
}
