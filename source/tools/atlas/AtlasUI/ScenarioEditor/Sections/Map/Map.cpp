#include "stdafx.h"

#include "Map.h"

#include "ActionButton.h"
#include "Datafile.h"

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
		POST_COMMAND(LoadMap(map));
	}
	
	wxCHECK_RET(cwd == wxFileName::GetCwd(), _T("cwd changed"));
		// paranoia - MSDN says "OFN_NOCHANGEDIR ... is ineffective for GetOpenFileName"
		// but it seems to work anyway

	// TODO: Make this a non-undoable command
}

static void GenerateMap(void*)
{
	POST_COMMAND(GenerateMap(9));
}

static void GenerateRMS(void* data)
{
	wxChar* argv[] = { _T("rmgen.exe"), 0, _T("_atlasrm"), 0 };
	wxString scriptName = ((wxTextCtrl*)data)->GetValue();
	argv[1] = const_cast<wxChar*>(scriptName.c_str());
	wxExecute(argv, wxEXEC_SYNC);
	POST_COMMAND(LoadMap(L"_atlasrm.pmp"));
}

//////////////////////////////////////////////////////////////////////////

MapSidebar::MapSidebar(wxWindow* parent)
	: Sidebar(parent)
{
	// TODO: Less ugliness
	// TODO: Intercept arrow keys and send them to the GL window

	m_MainSizer->Add(new ActionButton(this, _T("Load existing map"), &LoadMap, NULL));
	m_MainSizer->Add(new ActionButton(this, _T("Generate empty map"), &GenerateMap, NULL));

	{
		wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
		m_MainSizer->Add(sizer);
		wxTextCtrl* text = new wxTextCtrl(this, wxID_ANY, _T("cantabrian_highlands"));
		sizer->Add(text);
		sizer->Add(new ActionButton(this, _T("Generate RMS"), &GenerateRMS, text));
	}
}
