/* Copyright (C) 2019 Wildfire Games.
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

#include "Environment.h"
#include "LightControl.h"

#include "GameInterface/Messages.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "General/Observable.h"
#include "CustomControls/ColorDialog/ColorDialog.h"

using AtlasMessage::Shareable;

static Observable<AtlasMessage::sEnvironmentSettings> g_EnvironmentSettings;

const float M_PIf = 3.14159265f;

//////////////////////////////////////////////////////////////////////////

class VariableSliderBox : public wxPanel
{
	static const int range = 1024;
public:
	VariableSliderBox(wxWindow* parent, const wxString& label, Shareable<float>& var, float min, float max)
		: wxPanel(parent),
		m_Var(var), m_Min(min), m_Max(max)
	{
		m_Conn = g_EnvironmentSettings.RegisterObserver(0, &VariableSliderBox::OnSettingsChange, this);

		m_Sizer = new wxStaticBoxSizer(wxVERTICAL, this, label);
		SetSizer(m_Sizer);

		m_Slider = new wxSlider(this, -1, 0, 0, range);
		m_Sizer->Add(m_Slider, wxSizerFlags().Expand());
	}

	void OnSettingsChange(const AtlasMessage::sEnvironmentSettings& WXUNUSED(env))
	{
		m_Slider->SetValue((m_Var - m_Min) * (range / (m_Max - m_Min)));
	}

	void OnScroll(wxScrollEvent& evt)
	{
		m_Var = m_Min + (m_Max - m_Min)*(evt.GetInt() / (float)range);

		g_EnvironmentSettings.NotifyObserversExcept(m_Conn);
	}

private:
	ObservableScopedConnection m_Conn;
	wxStaticBoxSizer* m_Sizer;
	wxSlider* m_Slider;
	Shareable<float>& m_Var;
	float m_Min, m_Max;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(VariableSliderBox, wxPanel)
	EVT_SCROLL(VariableSliderBox::OnScroll)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

class VariableListBox : public wxPanel
{
public:
	VariableListBox(wxWindow* parent, const wxString& label, Shareable<std::wstring>& var)
		: wxPanel(parent),
		m_Var(var)
	{
		m_Conn = g_EnvironmentSettings.RegisterObserver(0, &VariableListBox::OnSettingsChange, this);

		m_Sizer = new wxStaticBoxSizer(wxVERTICAL, this, label);
		SetSizer(m_Sizer);

		m_Combo = new wxComboBox(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxArrayString(), wxCB_READONLY),
		m_Sizer->Add(m_Combo, wxSizerFlags().Expand());
	}

	void SetChoices(const std::vector<std::wstring>& choices)
	{
		wxArrayString choices_arraystr;
		for (size_t i = 0; i < choices.size(); ++i)
			choices_arraystr.Add(choices[i].c_str());

		m_Combo->Clear();
		m_Combo->Append(choices_arraystr);

		m_Combo->SetValue(m_Var.c_str());
	}

	void OnSettingsChange(const AtlasMessage::sEnvironmentSettings& WXUNUSED(env))
	{
		m_Combo->SetValue(m_Var.c_str());
	}

	void OnSelect(wxCommandEvent& WXUNUSED(evt))
	{
		m_Var = std::wstring(m_Combo->GetValue().c_str());

		g_EnvironmentSettings.NotifyObserversExcept(m_Conn);
	}

private:
	ObservableScopedConnection m_Conn;
	wxStaticBoxSizer* m_Sizer;
	wxComboBox* m_Combo;
	Shareable<std::wstring>& m_Var;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(VariableListBox, wxPanel)
	EVT_COMBOBOX(wxID_ANY, VariableListBox::OnSelect)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

class VariableColorBox : public wxPanel
{
public:
	VariableColorBox(wxWindow* parent, const wxString& label, Shareable<AtlasMessage::Color>& color)
		: wxPanel(parent),
		m_Color(color)
	{
		m_Conn = g_EnvironmentSettings.RegisterObserver(0, &VariableColorBox::OnSettingsChange, this);

		m_Sizer = new wxStaticBoxSizer(wxVERTICAL, this, label);
		SetSizer(m_Sizer);

		m_Button = new wxButton(this, -1);
		m_Sizer->Add(m_Button, wxSizerFlags().Expand());
	}

	void OnSettingsChange(const AtlasMessage::sEnvironmentSettings& WXUNUSED(env))
	{
		UpdateButton();
	}

	void OnClick(wxCommandEvent& WXUNUSED(evt))
	{
		ColorDialog dlg (this, _T("Scenario Editor/LightingColor"),
			wxColor(m_Color->r, m_Color->g, m_Color->b));

		if (dlg.ShowModal() == wxID_OK)
		{
			wxColor& c = dlg.GetColourData().GetColour();
			m_Color = AtlasMessage::Color(c.Red(), c.Green(), c.Blue());
			UpdateButton();

			g_EnvironmentSettings.NotifyObserversExcept(m_Conn);
		}
	}

	void UpdateButton()
	{
		m_Button->SetBackgroundColour(wxColor(m_Color->r, m_Color->g, m_Color->b));
		m_Button->SetLabel(wxString::Format(_T("%02X %02X %02X"), m_Color->r, m_Color->g, m_Color->b));

		int y = 3*m_Color->r + 6*m_Color->g + 1*m_Color->b;
		if (y > 1280)
			m_Button->SetForegroundColour(wxColor(0, 0, 0));
		else
			m_Button->SetForegroundColour(wxColor(255, 255, 255));
	}


private:
	ObservableScopedConnection m_Conn;
	wxStaticBoxSizer* m_Sizer;
	wxButton* m_Button;
	Shareable<AtlasMessage::Color>& m_Color;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(VariableColorBox, wxPanel)
	EVT_BUTTON(wxID_ANY, VariableColorBox::OnClick)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

enum {
	ID_RecomputeWaterData,
	ID_PickWaterHeight
};
static void SendToGame(const AtlasMessage::sEnvironmentSettings& settings)
{
	POST_COMMAND(SetEnvironmentSettings, (settings));
}

EnvironmentSidebar::EnvironmentSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer)
{
	wxSizer* scrollSizer = new wxBoxSizer(wxVERTICAL);
	wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this);
	scrolledWindow->SetScrollRate(10, 10);
	scrolledWindow->SetSizer(scrollSizer);
	m_MainSizer->Add(scrolledWindow,  wxSizerFlags().Proportion(1).Expand());

	wxSizer* waterSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _T("Water settings"));
	scrollSizer->Add(waterSizer, wxSizerFlags().Expand());
	waterSizer->Add(new wxButton(scrolledWindow, ID_RecomputeWaterData, _("Reset Water Data")), wxSizerFlags().Expand());
	waterSizer->Add(m_WaterTypeList = new VariableListBox(scrolledWindow, _("Water Type"), g_EnvironmentSettings.watertype), wxSizerFlags().Expand());
	waterSizer->Add(new VariableSliderBox(scrolledWindow, _("Water height"), g_EnvironmentSettings.waterheight, 0.f, 1.2f), wxSizerFlags().Expand());
	waterSizer->Add(new wxButton(scrolledWindow, ID_PickWaterHeight, _("Pick Water Height")), wxSizerFlags().Expand());
	waterSizer->Add(new VariableSliderBox(scrolledWindow, _("Water waviness"), g_EnvironmentSettings.waterwaviness, 0.f, 10.f), wxSizerFlags().Expand());
	waterSizer->Add(new VariableSliderBox(scrolledWindow, _("Water murkiness"), g_EnvironmentSettings.watermurkiness, 0.f, 1.f), wxSizerFlags().Expand());
	waterSizer->Add(new VariableSliderBox(scrolledWindow, _("Wind angle"), g_EnvironmentSettings.windangle, -M_PIf, M_PIf), wxSizerFlags().Expand());
	waterSizer->Add(new VariableColorBox(scrolledWindow, _("Water color"), g_EnvironmentSettings.watercolor), wxSizerFlags().Expand());
	waterSizer->Add(new VariableColorBox(scrolledWindow, _("Water tint"), g_EnvironmentSettings.watertint), wxSizerFlags().Expand());

	std::vector<std::wstring> list;
	list.push_back(L"ocean"); list.push_back(L"lake"); list.push_back(L"clap");
	m_WaterTypeList->SetChoices(list);


	wxSizer* sunSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _T("Sun / lighting settings"));
	scrollSizer->Add(sunSizer, wxSizerFlags().Expand().Border(wxTOP, 8));

	sunSizer->Add(new VariableSliderBox(scrolledWindow, _("Sun rotation"), g_EnvironmentSettings.sunrotation, -M_PIf, M_PIf), wxSizerFlags().Expand());
	sunSizer->Add(new VariableSliderBox(scrolledWindow, _("Sun elevation"), g_EnvironmentSettings.sunelevation, -M_PIf/2, M_PIf/2), wxSizerFlags().Expand());
	sunSizer->Add(new VariableSliderBox(scrolledWindow, _("Sun overbrightness"), g_EnvironmentSettings.sunoverbrightness, 1.0f, 3.0f), wxSizerFlags().Expand());
	sunSizer->Add(new LightControl(scrolledWindow, wxSize(150, 150), g_EnvironmentSettings));
	sunSizer->Add(new VariableColorBox(scrolledWindow, _("Sun color"), g_EnvironmentSettings.suncolor), wxSizerFlags().Expand());
	sunSizer->Add(m_SkyList = new VariableListBox(scrolledWindow, _("Sky set"), g_EnvironmentSettings.skyset), wxSizerFlags().Expand());
	sunSizer->Add(new VariableSliderBox(scrolledWindow, _("Fog Factor"), g_EnvironmentSettings.fogfactor, 0.0f, 0.01f), wxSizerFlags().Expand());
	sunSizer->Add(new VariableSliderBox(scrolledWindow, _("Fog Thickness"), g_EnvironmentSettings.fogmax, 0.5f, 0.0f), wxSizerFlags().Expand());
	sunSizer->Add(new VariableColorBox(scrolledWindow, _("Fog color"), g_EnvironmentSettings.fogcolor), wxSizerFlags().Expand());
	sunSizer->Add(new VariableColorBox(scrolledWindow, _("Terrain ambient color"), g_EnvironmentSettings.terraincolor), wxSizerFlags().Expand());
	sunSizer->Add(new VariableColorBox(scrolledWindow, _("Object ambient color"), g_EnvironmentSettings.unitcolor), wxSizerFlags().Expand());

	wxSizer* postProcSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _T("Post-processing settings"));
	scrollSizer->Add(postProcSizer, wxSizerFlags().Expand().Border(wxTOP, 8));

	postProcSizer->Add(m_PostEffectList = new VariableListBox(scrolledWindow, _("Post Effect"), g_EnvironmentSettings.posteffect), wxSizerFlags().Expand());
	postProcSizer->Add(new VariableSliderBox(scrolledWindow, _("Brightness"), g_EnvironmentSettings.brightness, -0.5f, 0.5f), wxSizerFlags().Expand());
	postProcSizer->Add(new VariableSliderBox(scrolledWindow, _("Contrast (HDR)"), g_EnvironmentSettings.contrast, 0.5f, 1.5f), wxSizerFlags().Expand());
	postProcSizer->Add(new VariableSliderBox(scrolledWindow, _("Saturation"), g_EnvironmentSettings.saturation, 0.0f, 2.0f), wxSizerFlags().Expand());
	postProcSizer->Add(new VariableSliderBox(scrolledWindow, _("Bloom"), g_EnvironmentSettings.bloom, 0.2f, 0.0f), wxSizerFlags().Expand());

	m_Conn = g_EnvironmentSettings.RegisterObserver(0, &SendToGame);
}

void EnvironmentSidebar::OnFirstDisplay()
{
	// Load the list of skies. (Can only be done now rather than in the constructor,
	// after the game has been initialised.)
	AtlasMessage::qGetSkySets qry_skysets;
	qry_skysets.Post();
	m_SkyList->SetChoices(*qry_skysets.skysets);

	AtlasMessage::qGetPostEffects qry_effects;
	qry_effects.Post();
	m_PostEffectList->SetChoices(*qry_effects.posteffects);

	UpdateEnvironmentSettings();
}

void EnvironmentSidebar::OnMapReload()
{
	UpdateEnvironmentSettings();
}

void EnvironmentSidebar::RecomputeWaterData(wxCommandEvent& WXUNUSED(evt))
{
	POST_COMMAND(RecalculateWaterData, (0.0f));
}

void EnvironmentSidebar::UpdateEnvironmentSettings()
{
	AtlasMessage::qGetEnvironmentSettings qry_env;
	qry_env.Post();
	g_EnvironmentSettings = qry_env.settings;

	g_EnvironmentSettings.NotifyObservers();
}

void EnvironmentSidebar::OnPickWaterHeight(wxCommandEvent& WXUNUSED(evt))
{
	m_ScenarioEditor.GetToolManager().SetCurrentTool(_T("PickWaterHeight"), this);
}

BEGIN_EVENT_TABLE(EnvironmentSidebar, Sidebar)
	EVT_BUTTON(ID_RecomputeWaterData, EnvironmentSidebar::RecomputeWaterData)
	EVT_BUTTON(ID_PickWaterHeight, EnvironmentSidebar::OnPickWaterHeight)
END_EVENT_TABLE();

