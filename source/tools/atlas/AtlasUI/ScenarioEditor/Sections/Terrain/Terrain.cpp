#include "stdafx.h"

#include "Terrain.h"

#include "ActionButton.h"
#include "Datafile.h"
#include "ScenarioEditor/Tools/Common/Brushes.h"

#include "GameInterface/Messages.h"

#include "wx/spinctrl.h"

TerrainSidebar::TerrainSidebar(wxWindow* parent)
: Sidebar(parent)
{
	// TODO: Less ugliness
	// TODO: Intercept arrow keys and send them to the GL window

	m_MainSizer->Add(new wxStaticText(this, wxID_ANY, _T("TODO: Make this much less ugly\n")));

	{
		wxSizer* sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Elevation tools"));
		sizer->Add(new wxButton(this, wxID_ANY, _("Modify"), wxDefaultPosition, wxSize(50,20)));
		sizer->Add(new wxButton(this, wxID_ANY, _("Smooth"), wxDefaultPosition, wxSize(50,20)));
		sizer->Add(new wxButton(this, wxID_ANY, _("Sample"), wxDefaultPosition, wxSize(50,20)));
		m_MainSizer->Add(sizer);
	}

	{
		wxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Brush"));
		g_Brush_Elevation.CreateUI(this, sizer);
		m_MainSizer->Add(sizer);
	}

}
