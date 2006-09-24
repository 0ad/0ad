#include "stdafx.h"

#include "Map.h"

#include "General/Datafile.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

#include "GameInterface/Messages.h"

#include "wx/filename.h"

enum
{
	ID_GenerateMap,
	ID_GenerateRMS
};

MapSidebar::MapSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
	: Sidebar(sidebarContainer, bottomBarContainer)
{
	// TODO: Less ugliness
	// TODO: Intercept arrow keys and send them to the GL window

	m_MainSizer->Add(new wxButton(this, ID_GenerateMap, _("Generate empty map")));

	wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	m_MainSizer->Add(sizer);
	m_RMSText = new wxTextCtrl(this, wxID_ANY, _T("cantabrian_highlands"));
	sizer->Add(m_RMSText);
	sizer->Add(new wxButton(this, ID_GenerateRMS, _("Generate RMS")));
}

void MapSidebar::GenerateMap(wxCommandEvent& WXUNUSED(event))
{
	POST_MESSAGE(GenerateMap, (9));
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

BEGIN_EVENT_TABLE(MapSidebar, Sidebar)
	EVT_BUTTON(ID_GenerateMap, MapSidebar::GenerateMap)
	EVT_BUTTON(ID_GenerateRMS, MapSidebar::GenerateRMS)
END_EVENT_TABLE();
