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
