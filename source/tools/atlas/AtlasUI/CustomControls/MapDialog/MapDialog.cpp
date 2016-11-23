/* Copyright (C) 2015 Wildfire Games.
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

#include "MapDialog.h"

#include "GameInterface/MessagePasser.h"
#include "GameInterface/Messages.h"

#include <wx/filename.h>
#include <wx/notebook.h>

enum {
	ID_MapDialogFilename = 1,
	ID_MapDialogNotebook,
	ID_ScenarioPage,
	ID_SkirmishPage
};

static const wxString scenarioPath(L"maps/scenarios/");
static const wxString skirmishPath(L"maps/skirmishes/");

MapDialog::MapDialog(wxWindow* parent, MapDialogType type, const wxIcon& icon)
	: wxDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(600,400), wxCAPTION|wxRESIZE_BORDER|wxCLOSE_BOX|wxSYSTEM_MENU),
	m_Type(type), m_SelectedPage(0)
{
	Freeze();

	SetIcon(icon);

	if (m_Type == MAPDIALOG_OPEN)
		SetTitle(_("Choose map to open"));
	else // MAPDIALOG_SAVE
		SetTitle(_("Choose map to save"));

	AtlasMessage::qGetMapList qry;
	qry.Post();

	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	wxNotebook* notebook = new wxNotebook(this, ID_MapDialogNotebook);
	{
		wxPanel* page = new wxPanel(notebook, ID_ScenarioPage);
		wxSizer* pageSizer = new wxBoxSizer(wxVERTICAL);
		// TODO: Should display something nicer than raw VFS paths
		wxListBox* listBox = new wxListBox(page, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_HSCROLL);
		const std::vector<std::wstring>& scenarioFilenames = *qry.scenarioFilenames;
		for (size_t i = 0; i < scenarioFilenames.size(); ++i)
		{
			wxString name = scenarioFilenames[i].substr(scenarioPath.Length());
			listBox->Append(name, new wxStringClientData(scenarioFilenames[i]));
		}

		pageSizer->Add(listBox,  wxSizerFlags().Proportion(1).Expand().Align(wxBOTTOM));
		page->SetSizer(pageSizer);
		notebook->AddPage(page, _("Scenarios"));
	}
	{
		wxPanel* page = new wxPanel(notebook, ID_SkirmishPage);
		wxSizer* pageSizer = new wxBoxSizer(wxVERTICAL);
		// TODO: Should display something nicer than raw VFS paths
		wxListBox* listBox = new wxListBox(page, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_HSCROLL);
		const std::vector<std::wstring>& skirmishFilenames = *qry.skirmishFilenames;
		for (size_t i = 0; i < skirmishFilenames.size(); ++i)
		{
			wxString name = skirmishFilenames[i].substr(skirmishPath.Length());
			listBox->Append(name, new wxStringClientData(skirmishFilenames[i]));
		}

		pageSizer->Add(listBox,  wxSizerFlags().Proportion(1).Expand());
		page->SetSizer(pageSizer);
		notebook->AddPage(page, _("Skirmishes"));
	}

	notebook->SetSelection(0);

	sizer->Add(notebook, wxSizerFlags().Proportion(1).Expand());

	sizer->AddSpacer(5);

	wxSizer* filenameSizer = new wxBoxSizer(wxHORIZONTAL);
	filenameSizer->AddSpacer(10);
	filenameSizer->Add(new wxStaticText(this, wxID_ANY, _("Map name: ")), wxSizerFlags().Align(wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL));
	wxTextCtrl* filename = new wxTextCtrl(this, ID_MapDialogFilename, wxEmptyString);
	if (m_Type == MAPDIALOG_OPEN)
		filename->Disable();
	filenameSizer->Add(filename, wxSizerFlags().Proportion(1).Expand());
	sizer->Add(filenameSizer, wxSizerFlags().Expand());

	sizer->AddSpacer(20);

	wxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	if (m_Type == MAPDIALOG_OPEN)
		buttonSizer->Add(new wxButton(this, wxID_OPEN, _("Open")));
	else // MAPDIALOG_SAVE
		buttonSizer->Add(new wxButton(this, wxID_SAVE, _("Save")));
	buttonSizer->AddSpacer(5);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, _("Cancel")));

	sizer->Add(buttonSizer, wxSizerFlags().Align(wxALIGN_RIGHT).Border(wxRIGHT|wxBOTTOM, 10));

	SetSizer(sizer);

	Layout();
	Thaw();
}

wxString MapDialog::GetFilename() const
{
	wxFileName filename(m_Filename, wxPATH_UNIX);
	filename.SetExt(L"xml");
	if (m_SelectedPage == 0)
		return scenarioPath + filename.GetFullPath(wxPATH_UNIX);
	else if (m_SelectedPage == 1)
		return skirmishPath + filename.GetFullPath(wxPATH_UNIX);
	else
		return wxEmptyString;
}

void MapDialog::OnListBox(wxCommandEvent& evt)
{
	if (evt.GetInt() < 0)
		m_Filename = wxEmptyString;
	else
		m_Filename = evt.GetString();

	wxDynamicCast(FindWindow(ID_MapDialogFilename), wxTextCtrl)->ChangeValue(m_Filename);

	if (evt.GetEventType() == wxEVT_COMMAND_LISTBOX_DOUBLECLICKED)
	{
		if (m_Type == MAPDIALOG_OPEN)
			OpenFile();
		else
			SaveFile();
	}
}

void MapDialog::OnCancel(wxCommandEvent& WXUNUSED(evt))
{
	EndModal(wxID_CANCEL);
}

void MapDialog::OnOpen(wxCommandEvent& WXUNUSED(evt))
{
	OpenFile();
}

void MapDialog::OnSave(wxCommandEvent& WXUNUSED(evt))
{
	SaveFile();
}

void MapDialog::OnFilename(wxCommandEvent& evt)
{
	m_Filename = evt.GetString();
}

void MapDialog::OnNotebookChanged(wxBookCtrlEvent& evt)
{
	m_SelectedPage = evt.GetSelection();
}

void MapDialog::OpenFile()
{
	wxString filename = GetFilename();
	if (filename.empty())
		return;

	AtlasMessage::qVFSFileExists qry(filename.wc_str());
	qry.Post();
	if (!qry.exists)
		return;

	EndModal(wxID_OK);
}

void MapDialog::SaveFile()
{
	wxString filename = GetFilename();
	if (filename.empty())
		return;

	// TODO: this test would work better outside the VFS
	AtlasMessage::qVFSFileExists qry(filename.wc_str());
	qry.Post();
	if (qry.exists)
	{
		if (wxMessageBox(_("WARNING: '") + filename + _("' already exists, it may be overwritten. Continue?"), _("Overwrite map confirmation"), wxICON_EXCLAMATION | wxYES_NO) != wxYES)
			return;
	}

	EndModal(wxID_OK);
}

BEGIN_EVENT_TABLE(MapDialog, wxDialog)
	EVT_BUTTON			 (wxID_CANCEL, MapDialog::OnCancel)
	EVT_BUTTON			 (wxID_OPEN, MapDialog::OnOpen)
	EVT_BUTTON			 (wxID_SAVE, MapDialog::OnSave)
	EVT_LISTBOX          (wxID_ANY, MapDialog::OnListBox)
	EVT_LISTBOX_DCLICK   (wxID_ANY, MapDialog::OnListBox)
	EVT_TEXT			 (ID_MapDialogFilename, MapDialog::OnFilename)
	EVT_NOTEBOOK_PAGE_CHANGED (ID_MapDialogNotebook, MapDialog::OnNotebookChanged)
END_EVENT_TABLE()
