#include "stdafx.h"

#include "Environment.h"

#include "GameInterface/Messages.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

static AtlasMessage::sEnvironmentSettings g_EnvironmentSettings;

class VariableSlider : public wxSlider
{
public:
	VariableSlider(wxWindow* parent, float& var, float min, float max)
		: wxSlider(parent, -1, 0, 0, 1024),
		m_Var(var), m_Min(min), m_Max(max)
	{
	}

	void OnScroll(wxScrollEvent& evt)
	{
		m_Var = m_Min + (m_Max - m_Min)*(evt.GetInt() / 1024.f);
		POST_COMMAND(SetEnvironmentSettings, (g_EnvironmentSettings));
	}

private:
	float& m_Var;
	float m_Min, m_Max;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(VariableSlider, wxSlider)
	EVT_SCROLL(OnScroll)
END_EVENT_TABLE()

class VariableSliderBox : public wxStaticBoxSizer
{
public:
	VariableSliderBox(wxWindow* parent, const wxString& label, float& var, float min, float max)
		: wxStaticBoxSizer(wxVERTICAL, parent, label)
	{
		Add(new VariableSlider(parent, var, min, max));
	}
};

//////////////////////////////////////////////////////////////////////////

EnvironmentSidebar::EnvironmentSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(sidebarContainer, bottomBarContainer)
{
	wxStaticText* warning = new wxStaticText(this, -1, _("WARNING: None of these settings are saved with the map, so don't do anything serious with them."));
	warning->Wrap(180);
	m_MainSizer->Add(warning, wxSizerFlags().Border());
	m_MainSizer->Add(new VariableSliderBox(this, _("Water height"), g_EnvironmentSettings.waterheight, 0, 1.2f));
	m_MainSizer->Add(new VariableSliderBox(this, _("Water shininess"), g_EnvironmentSettings.watershininess, 0, 500.f));
	m_MainSizer->Add(new VariableSliderBox(this, _("Water waviness"), g_EnvironmentSettings.waterwaviness, 0, 10.f));
	m_MainSizer->Add(new VariableSliderBox(this, _("Sun rotation"), g_EnvironmentSettings.sunrotation, 0, 2*M_PI));
	m_MainSizer->Add(new VariableSliderBox(this, _("Sun elevation"), g_EnvironmentSettings.sunelevation, -M_PI/2, M_PI/2));
}

void EnvironmentSidebar::OnFirstDisplay()
{
	AtlasMessage::qGetEnvironmentSettings qry;
	qry.Post();
	g_EnvironmentSettings = qry.settings;

	// TODO: set defaults of controls
}
