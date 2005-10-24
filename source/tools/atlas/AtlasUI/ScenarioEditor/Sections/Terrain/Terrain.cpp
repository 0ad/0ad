#include "stdafx.h"

#include "Terrain.h"

#include "Buttons/ActionButton.h"
#include "Buttons/ToolButton.h"
#include "General/Datafile.h"
#include "ScenarioEditor/Tools/Common/Brushes.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

#include "GameInterface/Messages.h"

#include "wx/spinctrl.h"

TerrainSidebar::TerrainSidebar(wxWindow* parent)
: Sidebar(parent)
{
	// TODO: Less ugliness

	m_MainSizer->Add(new wxStaticText(this, wxID_ANY, _T("TODO: Make this much less ugly\n")));

	{
		wxSizer* sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Elevation tools"));
		sizer->Add(new ToolButton(this, _("Modify"), _T("AlterElevation"), wxSize(50,20)));
		sizer->Add(new ToolButton(this, _("Smooth"), _T(""), wxSize(50,20)));
		sizer->Add(new ToolButton(this, _("Sample"), _T(""), wxSize(50,20)));
		m_MainSizer->Add(sizer);
	}

	{
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Brush"));
		g_Brush_Elevation.CreateUI(this, sizer);
		m_MainSizer->Add(sizer);
	}

}
