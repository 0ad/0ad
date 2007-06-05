#include "stdafx.h"

#include "Map.h"

#include "General/Datafile.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

#include "AtlasScript/ScriptInterface.h"

#include "wx/filename.h"

MapSidebar::MapSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
	: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer)
{
	wxPanel* panel = m_ScenarioEditor.GetScriptInterface().LoadScriptAsPanel(_T("section/map"), this);
	m_MainSizer->Add(panel, wxSizerFlags().Expand());
}
