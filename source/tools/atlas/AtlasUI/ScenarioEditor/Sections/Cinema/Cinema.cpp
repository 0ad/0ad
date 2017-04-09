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
}

void CinemaSidebar::OnFirstDisplay()
{
	m_DrawPath->SetValue(false);
}

void CinemaSidebar::OnMapReload()
{
	m_DrawPath->SetValue(false);
}

void CinemaSidebar::OnTogglePathsDrawing(wxCommandEvent& evt)
{
	POST_COMMAND(SetCinemaPathsDrawing, (evt.IsChecked()));
}

BEGIN_EVENT_TABLE(CinemaSidebar, Sidebar)
EVT_CHECKBOX(ID_PathsDrawing, CinemaSidebar::OnTogglePathsDrawing)
END_EVENT_TABLE();
