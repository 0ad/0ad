#include "stdafx.h"

#include "Map.h"

#include "Buttons/ActionButton.h"
#include "General/Datafile.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

#include "GameInterface/Messages.h"

#include "wx/filename.h"

static void GenerateMap(void*)
{
	POST_MESSAGE(GenerateMap, (9));
}

static void GenerateRMS(void* data)
{
	wxChar* argv[] = { _T("rmgen.exe"), 0, _T("_atlasrm"), 0 };
	wxString scriptName = ((wxTextCtrl*)data)->GetValue();
	argv[1] = const_cast<wxChar*>(scriptName.c_str());

	wxString cwd = wxFileName::GetCwd();
	wxFileName::SetCwd(Datafile::GetDataDirectory());
	wxExecute(argv, wxEXEC_SYNC);
	wxFileName::SetCwd(cwd);

	POST_MESSAGE(LoadMap, (L"_atlasrm.pmp"));
}

//////////////////////////////////////////////////////////////////////////

MapSidebar::MapSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
	: Sidebar(sidebarContainer, bottomBarContainer)
{
	// TODO: Less ugliness
	// TODO: Intercept arrow keys and send them to the GL window

	m_MainSizer->Add(new ActionButton(this, _T("Generate empty map"), &GenerateMap, NULL));

	{
		wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
		m_MainSizer->Add(sizer);
		wxTextCtrl* text = new wxTextCtrl(this, wxID_ANY, _T("cantabrian_highlands"));
		sizer->Add(text);
		sizer->Add(new ActionButton(this, _T("Generate RMS"), &GenerateRMS, text));
	}
}
