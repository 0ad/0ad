#include "stdafx.h"

#include "Map.h"

#include "Buttons/ActionButton.h"
#include "General/Datafile.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

#include "GameInterface/Messages.h"

static void LoadMap(void*)
{
	wxFileDialog dlg (NULL, wxFileSelectorPromptStr, Datafile::GetDataDirectory()+_T("/mods/official/maps/scenarios"),
		_T(""), _T("PMP files (*.pmp)|*.pmp|All files (*.*)|*.*"), wxOPEN);

	wxString cwd = wxFileName::GetCwd();
	
	if (dlg.ShowModal() == wxID_OK)
	{
		// TODO: Work when the map is not in .../maps/scenarios/
		std::wstring map = dlg.GetFilename().c_str();

		// Deactivate tools, so they don't carry forwards into the new CWorld
		// and crash.
		SetCurrentTool(_T(""));
		// TODO: clear the undo buffer, etc

		POST_MESSAGE(LoadMap, (map));
	}
	
	wxCHECK_RET(cwd == wxFileName::GetCwd(), _T("cwd changed"));
		// paranoia - MSDN says "OFN_NOCHANGEDIR ... is ineffective for GetOpenFileName"
		// but it seems to work anyway

	// TODO: Make this a non-undoable command
}

static void SaveMap(void*)
{
	wxFileDialog dlg (NULL, wxFileSelectorPromptStr, Datafile::GetDataDirectory()+_T("/mods/official/maps/scenarios"),
		_T(""), _T("PMP files (*.pmp)|*.pmp|All files (*.*)|*.*"), wxSAVE|wxOVERWRITE_PROMPT);

	if (dlg.ShowModal() == wxID_OK)
	{
		// TODO: Work when the map is not in .../maps/scenarios/
		std::wstring map = dlg.GetFilename().c_str();
		POST_MESSAGE(SaveMap, (map));
	}
}

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

MapSidebar::MapSidebar(wxWindow* parent)
	: Sidebar(parent)
{
	// TODO: Less ugliness
	// TODO: Intercept arrow keys and send them to the GL window

	m_MainSizer->Add(new ActionButton(this, _T("Load existing map"), &LoadMap, NULL));
	m_MainSizer->Add(new ActionButton(this, _T("Save map"), &SaveMap, NULL));
	m_MainSizer->Add(new ActionButton(this, _T("Generate empty map"), &GenerateMap, NULL));

	{
		wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
		m_MainSizer->Add(sizer);
		wxTextCtrl* text = new wxTextCtrl(this, wxID_ANY, _T("cantabrian_highlands"));
		sizer->Add(text);
		sizer->Add(new ActionButton(this, _T("Generate RMS"), &GenerateRMS, text));
	}
}
