#include "stdafx.h"

#include "Environment.h"

#include "GameInterface/Messages.h"
#include "ScenarioEditor/Tools/Common/Tools.h"
#include "General/Observable.h"
#include "CustomControls/ColourDialog/ColourDialog.h"

static Observable<AtlasMessage::sEnvironmentSettings> g_EnvironmentSettings;

//////////////////////////////////////////////////////////////////////////

class VariableSlider : public wxSlider
{
	static const int range = 1024;
public:
	VariableSlider(wxWindow* parent, float& var, float min, float max)
		: wxSlider(parent, -1, 0, 0, range),
		m_Var(var), m_Min(min), m_Max(max)
	{
	}

	void OnScroll(wxScrollEvent& evt)
	{
		m_Var = m_Min + (m_Max - m_Min)*(evt.GetInt() / (float)range);
		POST_COMMAND(SetEnvironmentSettings, (g_EnvironmentSettings));
	}

	void UpdateFromVar()
	{
		SetValue((m_Var - m_Min) * (range / (m_Max - m_Min)));
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
		m_Slider = new VariableSlider(parent, var, min, max);
		Add(m_Slider);

		m_Conn = g_EnvironmentSettings.RegisterObserver(0, &VariableSliderBox::OnSettingsChange, this);
	}
	
	~VariableSliderBox()
	{
		m_Conn.disconnect();
	}

	void OnSettingsChange(const AtlasMessage::sEnvironmentSettings& WXUNUSED(env))
	{
		m_Slider->UpdateFromVar();
	}

private:
	ObservableConnection m_Conn;
	VariableSlider* m_Slider;
};

//////////////////////////////////////////////////////////////////////////

class VariableColourButton : public wxButton
{
public:
	VariableColourButton(wxWindow* parent, AtlasMessage::Colour& colour)
		: wxButton(parent, -1),	m_Colour(colour)
	{
		UpdateDisplay();
	}

	void OnClick(wxCommandEvent& WXUNUSED(evt))
	{
		ColourDialog dlg (NULL, _T("Scenario Editor/LightingColour"),
			wxColour(m_Colour.r, m_Colour.g, m_Colour.b));

		if (dlg.ShowModal() == wxID_OK)
		{
			wxColour& c = dlg.GetColourData().GetColour();
			m_Colour = AtlasMessage::Colour(c.Red(), c.Green(), c.Blue());
			UpdateDisplay();

			POST_COMMAND(SetEnvironmentSettings, (g_EnvironmentSettings));
		}
	}

	void UpdateDisplay()
	{
		SetBackgroundColour(wxColour(m_Colour.r, m_Colour.g, m_Colour.b));
		SetLabel(wxString::Format(_T("%02X %02X %02X"), m_Colour.r, m_Colour.g, m_Colour.b));

		int y = 3*m_Colour.r + 6*m_Colour.g + 1*m_Colour.b;
		if (y > 1280)
			SetForegroundColour(wxColour(0, 0, 0));
		else
			SetForegroundColour(wxColour(255, 255, 255));
	}

private:
	AtlasMessage::Colour& m_Colour;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(VariableColourButton, wxButton)
	EVT_BUTTON(wxID_ANY, OnClick)
END_EVENT_TABLE()

class VariableColourBox : public wxStaticBoxSizer
{
public:
	VariableColourBox(wxWindow* parent, const wxString& label, AtlasMessage::Colour& colour)
		: wxStaticBoxSizer(wxVERTICAL, parent, label)
	{
		m_Button = new VariableColourButton(parent, colour);
		Add(m_Button);

		m_Conn = g_EnvironmentSettings.RegisterObserver(0, &VariableColourBox::OnSettingsChange, this);
	}

	~VariableColourBox()
	{
		m_Conn.disconnect();
	}

	void OnSettingsChange(const AtlasMessage::sEnvironmentSettings& WXUNUSED(env))
	{
		m_Button->UpdateDisplay();
	}

private:
	ObservableConnection m_Conn;
	VariableColourButton* m_Button;
};

//////////////////////////////////////////////////////////////////////////

EnvironmentSidebar::EnvironmentSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(sidebarContainer, bottomBarContainer)
{
	m_MainSizer->Add(new VariableSliderBox(this, _("Water height"), g_EnvironmentSettings.waterheight, 0, 1.2f));
	m_MainSizer->Add(new VariableSliderBox(this, _("Water shininess"), g_EnvironmentSettings.watershininess, 0, 500.f));
	m_MainSizer->Add(new VariableSliderBox(this, _("Water waviness"), g_EnvironmentSettings.waterwaviness, 0, 10.f));
	m_MainSizer->Add(new VariableSliderBox(this, _("Sun rotation"), g_EnvironmentSettings.sunrotation, 0, 2*M_PI));
	m_MainSizer->Add(new VariableSliderBox(this, _("Sun elevation"), g_EnvironmentSettings.sunelevation, -M_PI/2, M_PI/2));
	m_MainSizer->Add(new VariableColourBox(this, _("Sun colour"), g_EnvironmentSettings.suncolour));
	m_MainSizer->Add(new VariableColourBox(this, _("Terrain ambient colour"), g_EnvironmentSettings.terraincolour));
	m_MainSizer->Add(new VariableColourBox(this, _("Object ambient colour"), g_EnvironmentSettings.unitcolour));
}

void EnvironmentSidebar::OnFirstDisplay()
{
	AtlasMessage::qGetEnvironmentSettings qry;
	qry.Post();
	g_EnvironmentSettings = qry.settings;

	g_EnvironmentSettings.NotifyObservers();
	// TODO: reupdate everything when loading a new map...
}
