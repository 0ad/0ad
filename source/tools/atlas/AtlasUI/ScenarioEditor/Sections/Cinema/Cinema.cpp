/* Copyright (C) 2017 Wildfire Games.
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

#include "Cinema.h"

#include "GameInterface/Messages.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "CustomControls/ColorDialog/ColorDialog.h"

using AtlasMessage::Shareable;

enum {
	ID_PathsDrawing,
	ID_PathsList,
	ID_AddPath,
	ID_DeletePath
};

// Helper function for adding tooltips
static wxWindow* Tooltipped(wxWindow* window, const wxString& tip)
{
	window->SetToolTip(tip);
	return window;
}

CinemaSidebar::CinemaSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
	: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer)
{
	wxSizer* scrollSizer = new wxBoxSizer(wxVERTICAL);
	scrolledWindow = new wxScrolledWindow(this);
	scrolledWindow->SetScrollRate(10, 10);
	scrolledWindow->SetSizer(scrollSizer);
	m_MainSizer->Add(scrolledWindow, wxSizerFlags().Proportion(1).Expand());

	wxSizer* commonSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _T("Common settings"));
	scrollSizer->Add(commonSizer, wxSizerFlags().Expand());

	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 5, 5);
	gridSizer->AddGrowableCol(1);

	gridSizer->Add(Tooltipped(m_DrawPath = new wxCheckBox(scrolledWindow, ID_PathsDrawing, _("Draw all paths")),
		_("Display every cinematic path added to the map")));

	commonSizer->Add(gridSizer, wxSizerFlags().Expand());

	// Paths list panel
	wxSizer* pathsSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _T("Paths"));
	scrollSizer->Add(pathsSizer, wxSizerFlags().Proportion(1).Expand());

	pathsSizer->Add(m_PathList = new wxListBox(scrolledWindow, ID_PathsList, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE | wxLB_SORT), wxSizerFlags().Proportion(1).Expand());
	scrollSizer->AddSpacer(3);
	pathsSizer->Add(Tooltipped(new wxButton(scrolledWindow, ID_DeletePath, _("Delete")), _T("Delete selected path")), wxSizerFlags().Expand());

	pathsSizer->Add(m_NewPathName = new wxTextCtrl(scrolledWindow, wxID_ANY), wxSizerFlags().Expand());
	pathsSizer->Add(new wxButton(scrolledWindow, ID_AddPath, _("Add")), wxSizerFlags().Expand());
}

void CinemaSidebar::OnFirstDisplay()
{
	m_DrawPath->SetValue(false);

	ReloadPathList();
}

void CinemaSidebar::OnMapReload()
{
	m_DrawPath->SetValue(false);

	ReloadPathList();
}

void CinemaSidebar::OnTogglePathsDrawing(wxCommandEvent& evt)
{
	POST_COMMAND(SetCinemaPathsDrawing, (evt.IsChecked()));
}

void CinemaSidebar::OnAddPath(wxCommandEvent&)
{
	if (m_NewPathName->GetValue().empty())
		return;

	POST_COMMAND(AddCinemaPath, (m_NewPathName->GetValue().ToStdWstring()));
	m_NewPathName->Clear();
	ReloadPathList();
}

void CinemaSidebar::OnDeletePath(wxCommandEvent&)
{
	int index = m_PathList->GetSelection();
	if (index < 0)
		return;

	wxString pathName = m_PathList->GetString(index);
	if (pathName.empty())
		return;

	POST_COMMAND(DeleteCinemaPath, (pathName.ToStdWstring()));
	ReloadPathList();
}

void CinemaSidebar::ReloadPathList()
{
	int index = m_PathList->GetSelection();
	wxString pathName;
	if (index >= 0)
		pathName = m_PathList->GetString(index);

	AtlasMessage::qGetCinemaPaths query_paths;
	query_paths.Post();

	m_PathList->Clear();
	for (const AtlasMessage::sCinemaPath& path : *query_paths.paths)
		m_PathList->Append(*path.name);
}

BEGIN_EVENT_TABLE(CinemaSidebar, Sidebar)
EVT_CHECKBOX(ID_PathsDrawing, CinemaSidebar::OnTogglePathsDrawing)
EVT_BUTTON(ID_AddPath, CinemaSidebar::OnAddPath)
EVT_BUTTON(ID_DeletePath, CinemaSidebar::OnDeletePath)
END_EVENT_TABLE();
