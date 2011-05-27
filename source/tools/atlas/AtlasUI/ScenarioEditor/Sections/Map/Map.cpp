/* Copyright (C) 2011 Wildfire Games.
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

#include "Map.h"

#include "General/Datafile.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

#include "GameInterface/Messages.h"

#include "wx/filename.h"

enum
{
	ID_GenerateMap,
	ID_GenerateRMS,
	ID_SimPlay,
	ID_SimFast,
	ID_SimSlow,
	ID_SimPause,
	ID_SimReset
};

enum
{
	SimInactive,
	SimPlaying,
	SimPlayingFast,
	SimPlayingSlow,
	SimPaused
};
bool IsPlaying(int s) { return (s == SimPlaying || s == SimPlayingFast || s == SimPlayingSlow); }

MapSidebar::MapSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
	: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer), m_SimState(SimInactive)
{
	// TODO: Less ugliness
	// TODO: Intercept arrow keys and send them to the GL window

	m_MainSizer->Add(new wxButton(this, ID_GenerateMap, _("Generate empty map")));

	{
		wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
		m_RMSText = new wxTextCtrl(this, wxID_ANY, _T("cantabrian_highlands"));
		sizer->Add(m_RMSText);
		sizer->Add(new wxButton(this, ID_GenerateRMS, _("Generate RMS")));
		m_MainSizer->Add(sizer);
	}

	{
		wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Simulation test"));
		sizer->Add(new wxButton(this, ID_SimPlay, _("Play")), wxSizerFlags().Proportion(1));
		sizer->Add(new wxButton(this, ID_SimFast, _("Fast")), wxSizerFlags().Proportion(1));
		sizer->Add(new wxButton(this, ID_SimSlow, _("Slow")), wxSizerFlags().Proportion(1));
		sizer->Add(new wxButton(this, ID_SimPause, _("Pause")), wxSizerFlags().Proportion(1));
		sizer->Add(new wxButton(this, ID_SimReset, _("Reset")), wxSizerFlags().Proportion(1));
		UpdateSimButtons();
		m_MainSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 16));
	}
}

void MapSidebar::GenerateMap(wxCommandEvent& WXUNUSED(event))
{
//	qGenerateMap qry();
//	qry.Post();
//	POST_MESSAGE(GenerateMap, (9));
}

void MapSidebar::GenerateRMS(wxCommandEvent& WXUNUSED(event))
{
	wxChar* argv[] = { _T("rmgen.exe"), 0, _T("_atlasrm"), 0 };
	wxString scriptName = m_RMSText->GetValue();
	argv[1] = const_cast<wxChar*>(scriptName.c_str());

	wxString cwd = wxFileName::GetCwd();
	wxFileName::SetCwd(Datafile::GetDataDirectory());
	wxExecute(argv, wxEXEC_SYNC);
	wxFileName::SetCwd(cwd);

	POST_MESSAGE(LoadMap, (L"_atlasrm.pmp"));
}

void MapSidebar::UpdateSimButtons()
{
	wxButton* button;

	button = wxDynamicCast(FindWindow(ID_SimPlay), wxButton);
	wxCHECK(button, );
	button->Enable(m_SimState != SimPlaying);

	button = wxDynamicCast(FindWindow(ID_SimFast), wxButton);
	wxCHECK(button, );
	button->Enable(m_SimState != SimPlayingFast);

	button = wxDynamicCast(FindWindow(ID_SimSlow), wxButton);
	wxCHECK(button, );
	button->Enable(m_SimState != SimPlayingSlow);

	button = wxDynamicCast(FindWindow(ID_SimPause), wxButton);
	wxCHECK(button, );
	button->Enable(IsPlaying(m_SimState));

	button = wxDynamicCast(FindWindow(ID_SimReset), wxButton);
	wxCHECK(button, );
	button->Enable(m_SimState != SimInactive);
}

void MapSidebar::OnSimPlay(wxCommandEvent& event)
{
	float speed = 1.f;
	int newState = SimPlaying;
	if (event.GetId() == ID_SimFast)
	{
		speed = 8.f;
		newState = SimPlayingFast;
	}
	else if (event.GetId() == ID_SimSlow)
	{
		speed = 0.125f;
		newState = SimPlayingSlow;
	}

	if (m_SimState == SimInactive)
	{
		POST_MESSAGE(SimStateSave, (L"default"));
		POST_MESSAGE(SimPlay, (speed));
		m_SimState = newState;
	}
	else // paused or already playing at a different speed
	{
		POST_MESSAGE(SimPlay, (speed));
		m_SimState = newState;
	}
	UpdateSimButtons();
}

void MapSidebar::OnSimPause(wxCommandEvent& WXUNUSED(event))
{
	if (IsPlaying(m_SimState))
	{
		POST_MESSAGE(SimPlay, (0.f));
		m_SimState = SimPaused;
	}
	UpdateSimButtons();
}

void MapSidebar::OnSimReset(wxCommandEvent& WXUNUSED(event))
{
	if (IsPlaying(m_SimState))
	{
		POST_MESSAGE(SimPlay, (0.f));
		POST_MESSAGE(SimStateRestore, (L"default"));
		m_SimState = SimInactive;
	}
	else if (m_SimState == SimPaused)
	{
		POST_MESSAGE(SimStateRestore, (L"default"));
		m_SimState = SimInactive;
	}
	UpdateSimButtons();
}

BEGIN_EVENT_TABLE(MapSidebar, Sidebar)
	EVT_BUTTON(ID_GenerateMap, MapSidebar::GenerateMap)
	EVT_BUTTON(ID_GenerateRMS, MapSidebar::GenerateRMS)
	EVT_BUTTON(ID_SimPlay, MapSidebar::OnSimPlay)
	EVT_BUTTON(ID_SimFast, MapSidebar::OnSimPlay)
	EVT_BUTTON(ID_SimSlow, MapSidebar::OnSimPlay)
	EVT_BUTTON(ID_SimPause, MapSidebar::OnSimPause)
	EVT_BUTTON(ID_SimReset, MapSidebar::OnSimReset)
END_EVENT_TABLE();
