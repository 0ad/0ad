/* Copyright (C) 2017 Wildfire Games.
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

#include "Map.h"

#include "AtlasObject/AtlasObject.h"
#include "GameInterface/Messages.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "ScenarioEditor/Tools/Common/Tools.h"

#include "wx/busyinfo.h"
#include "wx/filename.h"

enum
{
	ID_MapName,
	ID_MapDescription,
	ID_MapReveal,
	ID_MapType,
	ID_MapPreview,
	ID_MapTeams,
	ID_MapKW_Demo,
	ID_MapKW_Naval,
	ID_RandomScript,
	ID_RandomSize,
	ID_RandomSeed,
	ID_RandomReseed,
	ID_RandomGenerate,
	ID_SimPlay,
	ID_SimFast,
	ID_SimSlow,
	ID_SimPause,
	ID_SimReset,
	ID_OpenPlayerPanel
};

enum
{
	SimInactive,
	SimPlaying,
	SimPlayingFast,
	SimPlayingSlow,
	SimPaused
};

bool IsPlaying(int s) { return (s == SimPlaying || s == SimPlayingFast || s == SimPlayingSlow); }

// TODO: Some of these helper things should be moved out of this file
// and into shared locations

// Helper function for adding tooltips
static wxWindow* Tooltipped(wxWindow* window, const wxString& tip)
{
	window->SetToolTip(tip);
	return window;
}

// Helper class for storing AtObjs
class AtObjClientData : public wxClientData
{
public:
	AtObjClientData(const AtObj& obj) : obj(obj) {}
	virtual ~AtObjClientData() {}
	AtObj GetValue() { return obj; }
private:
	AtObj obj;
};

class MapSettingsControl : public wxPanel
{
public:
	MapSettingsControl(wxWindow* parent, ScenarioEditor& scenarioEditor);
	void CreateWidgets();
	void ReadFromEngine();
	void SetMapSettings(const AtObj& obj);
	AtObj UpdateSettingsObject();
private:
	void SendToEngine();

	void OnEdit(wxCommandEvent& WXUNUSED(evt))
	{
		SendToEngine();
	}

	std::set<std::wstring> m_MapSettingsKeywords;
	std::vector<wxChoice*> m_PlayerCivChoices;
	Observable<AtObj>& m_MapSettings;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(MapSettingsControl, wxPanel)
	EVT_TEXT(ID_MapName, MapSettingsControl::OnEdit)
	EVT_TEXT(ID_MapDescription, MapSettingsControl::OnEdit)
	EVT_TEXT(ID_MapPreview, MapSettingsControl::OnEdit)
	EVT_CHECKBOX(wxID_ANY, MapSettingsControl::OnEdit)
	EVT_CHOICE(wxID_ANY, MapSettingsControl::OnEdit)
END_EVENT_TABLE();

MapSettingsControl::MapSettingsControl(wxWindow* parent, ScenarioEditor& scenarioEditor)
	: wxPanel(parent, wxID_ANY), m_MapSettings(scenarioEditor.GetMapSettings())
{
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Map settings"));
	SetSizer(sizer);
}

void MapSettingsControl::CreateWidgets()
{
	wxSizer* sizer = GetSizer();

	/////////////////////////////////////////////////////////////////////////
	// Map settings
	wxBoxSizer* nameSizer = new wxBoxSizer(wxHORIZONTAL);
	nameSizer->Add(new wxStaticText(this, wxID_ANY, _("Name")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	nameSizer->Add(8, 0);
	nameSizer->Add(Tooltipped(new wxTextCtrl(this, ID_MapName),
			_("Displayed name of the map")), wxSizerFlags().Proportion(1));
	sizer->Add(nameSizer, wxSizerFlags().Expand());

	sizer->Add(0, 2);

	sizer->Add(new wxStaticText(this, wxID_ANY, _("Description")));
	sizer->Add(Tooltipped(new wxTextCtrl(this, ID_MapDescription, wxEmptyString, wxDefaultPosition, wxSize(-1, 100), wxTE_MULTILINE),
			_("Short description used on the map selection screen")), wxSizerFlags().Expand());

	sizer->AddSpacer(5);

	// TODO: replace by filenames in binaries/data/mods/public/simulation/data/settings/victory_conditions/
	wxArrayString gameTypes;
	gameTypes.Add(_T("conquest"));
	gameTypes.Add(_T("conquest_structures"));
	gameTypes.Add(_T("conquest_units"));
	gameTypes.Add(_T("wonder"));
	gameTypes.Add(_T("endless"));
	gameTypes.Add(_T("regicide"));
	gameTypes.Add(_T("capture_the_relic"));

	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 5, 5);
	gridSizer->AddGrowableCol(1);
	// TODO: have preview selector tool?
	gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Preview")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	gridSizer->Add(Tooltipped(new wxTextCtrl(this, ID_MapPreview, wxEmptyString),
		_("Texture used for map preview")), wxSizerFlags().Expand());
	gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Reveal map")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	gridSizer->Add(Tooltipped(new wxCheckBox(this, ID_MapReveal, wxEmptyString),
		_("If checked, players won't need to explore")));
	gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Game type")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	gridSizer->Add(Tooltipped(new wxChoice(this, ID_MapType, wxDefaultPosition, wxDefaultSize, gameTypes),
		_("Select the game type (or victory condition)")), wxSizerFlags().Expand());
	gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Lock teams")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	gridSizer->Add(Tooltipped(new wxCheckBox(this, ID_MapTeams, wxEmptyString),
		_("If checked, teams will be locked")));
	sizer->Add(gridSizer, wxSizerFlags().Expand());

	sizer->AddSpacer(5);

	wxStaticBoxSizer* keywordsSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Keywords"));
	wxFlexGridSizer* kwGridSizer = new wxFlexGridSizer(4, 5, 5);
	kwGridSizer->Add(new wxStaticText(this, wxID_ANY, _("Demo")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	kwGridSizer->Add(Tooltipped(new wxCheckBox(this, ID_MapKW_Demo, wxEmptyString),
		_("If checked, map will only be visible using filters in game setup")));
	kwGridSizer->Add(new wxStaticText(this, wxID_ANY, _("Naval")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	kwGridSizer->Add(Tooltipped(new wxCheckBox(this, ID_MapKW_Naval, wxEmptyString),
		_("If checked, map will only be visible using filters in game setup")));
	keywordsSizer->Add(kwGridSizer);
	sizer->Add(keywordsSizer, wxSizerFlags().Expand());
}

void MapSettingsControl::ReadFromEngine()
{
	AtlasMessage::qGetMapSettings qry;
	qry.Post();
	if (!(*qry.settings).empty())
	{
		// Prevent error if there's no map settings to parse
		m_MapSettings = AtlasObject::LoadFromJSON(*qry.settings);
	}

	// map name
	wxDynamicCast(FindWindow(ID_MapName), wxTextCtrl)->ChangeValue(wxString(m_MapSettings["Name"]));

	// map description
	wxDynamicCast(FindWindow(ID_MapDescription), wxTextCtrl)->ChangeValue(wxString(m_MapSettings["Description"]));

	// map preview
	wxDynamicCast(FindWindow(ID_MapPreview), wxTextCtrl)->ChangeValue(wxString(m_MapSettings["Preview"]));

	// reveal map
	wxDynamicCast(FindWindow(ID_MapReveal), wxCheckBox)->SetValue(wxString(m_MapSettings["RevealMap"]) == L"true");

	// game type / victory conditions
	if (m_MapSettings["GameType"].defined())
		wxDynamicCast(FindWindow(ID_MapType), wxChoice)->SetStringSelection(wxString(m_MapSettings["GameType"]));
	else
		wxDynamicCast(FindWindow(ID_MapType), wxChoice)->SetSelection(0);

	// lock teams
	wxDynamicCast(FindWindow(ID_MapTeams), wxCheckBox)->SetValue(wxString(m_MapSettings["LockTeams"]) == L"true");

	// keywords
	{
		m_MapSettingsKeywords.clear();
		for (AtIter keyword = m_MapSettings["Keywords"]["item"]; keyword.defined(); ++keyword)
			m_MapSettingsKeywords.insert(std::wstring(keyword));

		wxDynamicCast(FindWindow(ID_MapKW_Demo), wxCheckBox)->SetValue(m_MapSettingsKeywords.count(L"demo") != 0);
		wxDynamicCast(FindWindow(ID_MapKW_Naval), wxCheckBox)->SetValue(m_MapSettingsKeywords.count(L"naval") != 0);
	}
}

void MapSettingsControl::SetMapSettings(const AtObj& obj)
{
	m_MapSettings = obj;
	m_MapSettings.NotifyObservers();

	SendToEngine();
}

AtObj MapSettingsControl::UpdateSettingsObject()
{
	// map name
	m_MapSettings.set("Name", wxDynamicCast(FindWindow(ID_MapName), wxTextCtrl)->GetValue());

	// map description
	m_MapSettings.set("Description", wxDynamicCast(FindWindow(ID_MapDescription), wxTextCtrl)->GetValue());

	// map preview
	m_MapSettings.set("Preview", wxDynamicCast(FindWindow(ID_MapPreview), wxTextCtrl)->GetValue());

	// reveal map
	m_MapSettings.setBool("RevealMap", wxDynamicCast(FindWindow(ID_MapReveal), wxCheckBox)->GetValue());

	// game type / victory conditions
	m_MapSettings.set("GameType", wxDynamicCast(FindWindow(ID_MapType), wxChoice)->GetStringSelection());

	// keywords
	{
		if (wxDynamicCast(FindWindow(ID_MapKW_Demo), wxCheckBox)->GetValue())
			m_MapSettingsKeywords.insert(L"demo");
		else
			m_MapSettingsKeywords.erase(L"demo");

		if (wxDynamicCast(FindWindow(ID_MapKW_Naval), wxCheckBox)->GetValue())
			m_MapSettingsKeywords.insert(L"naval");
		else
			m_MapSettingsKeywords.erase(L"naval");

		AtObj keywords;
		keywords.set("@array", L"");
		for (std::set<std::wstring>::iterator it = m_MapSettingsKeywords.begin(); it != m_MapSettingsKeywords.end(); ++it)
			keywords.add("item", it->c_str());
		m_MapSettings.set("Keywords", keywords);
	}

	// teams locked
	m_MapSettings.setBool("LockTeams", wxDynamicCast(FindWindow(ID_MapTeams), wxCheckBox)->GetValue());

	return m_MapSettings;
}

void MapSettingsControl::SendToEngine()
{
	UpdateSettingsObject();

	std::string json = AtlasObject::SaveToJSON(m_MapSettings);

	// TODO: would be nice if we supported undo for settings changes

	POST_COMMAND(SetMapSettings, (json));
}


MapSidebar::MapSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
	: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer), m_SimState(SimInactive)
{
	wxSizer* scrollSizer = new wxBoxSizer(wxVERTICAL);
	wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this);
	scrolledWindow->SetScrollRate(10, 10);
	scrolledWindow->SetSizer(scrollSizer);
	m_MainSizer->Add(scrolledWindow, wxSizerFlags().Expand().Proportion(1));

	m_MapSettingsCtrl = new MapSettingsControl(scrolledWindow, m_ScenarioEditor);
	scrollSizer->Add(m_MapSettingsCtrl, wxSizerFlags().Expand());

	{
		/////////////////////////////////////////////////////////////////////////
		// Random map settings
		wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _("Random map"));
		scrollSizer->Add(sizer, wxSizerFlags().Expand());

		sizer->Add(new wxChoice(scrolledWindow, ID_RandomScript), wxSizerFlags().Expand());

		sizer->AddSpacer(5);

		sizer->Add(new wxButton(scrolledWindow, ID_OpenPlayerPanel, _T("Change players")), wxSizerFlags().Expand());

		sizer->AddSpacer(5);

		wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 5, 5);
		gridSizer->AddGrowableCol(1);

		wxChoice* sizeChoice = new wxChoice(scrolledWindow, ID_RandomSize);
		gridSizer->Add(new wxStaticText(scrolledWindow, wxID_ANY, _("Map size")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
		gridSizer->Add(sizeChoice, wxSizerFlags().Expand());

		gridSizer->Add(new wxStaticText(scrolledWindow, wxID_ANY, _("Random seed")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
		wxBoxSizer* seedSizer = new wxBoxSizer(wxHORIZONTAL);
		seedSizer->Add(Tooltipped(new wxTextCtrl(scrolledWindow, ID_RandomSeed, _T("0"), wxDefaultPosition, wxDefaultSize, 0, wxTextValidator(wxFILTER_NUMERIC)),
			_("Seed value for random map")), wxSizerFlags(1).Expand());
		seedSizer->Add(Tooltipped(new wxButton(scrolledWindow, ID_RandomReseed, _("R"), wxDefaultPosition, wxSize(24, -1)),
			_("New random seed")));
		gridSizer->Add(seedSizer, wxSizerFlags().Expand());

		sizer->Add(gridSizer, wxSizerFlags().Expand());

		sizer->AddSpacer(5);

		sizer->Add(Tooltipped(new wxButton(scrolledWindow, ID_RandomGenerate, _("Generate map")),
			_("Run selected random map script")), wxSizerFlags().Expand());
	}

	{
		/////////////////////////////////////////////////////////////////////////
		// Simulation buttons
		wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, _("Simulation test"));
		scrollSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 8));

		wxGridSizer* gridSizer = new wxGridSizer(5);
		gridSizer->Add(Tooltipped(new wxButton(scrolledWindow, ID_SimPlay, _("Play"), wxDefaultPosition, wxSize(48, -1)),
			_("Run the simulation at normal speed")), wxSizerFlags().Expand());
		gridSizer->Add(Tooltipped(new wxButton(scrolledWindow, ID_SimFast, _("Fast"), wxDefaultPosition, wxSize(48, -1)),
			_("Run the simulation at 8x speed")), wxSizerFlags().Expand());
		gridSizer->Add(Tooltipped(new wxButton(scrolledWindow, ID_SimSlow, _("Slow"), wxDefaultPosition, wxSize(48, -1)),
			_("Run the simulation at 1/8x speed")), wxSizerFlags().Expand());
		gridSizer->Add(Tooltipped(new wxButton(scrolledWindow, ID_SimPause, _("Pause"), wxDefaultPosition, wxSize(48, -1)),
			_("Pause the simulation")), wxSizerFlags().Expand());
		gridSizer->Add(Tooltipped(new wxButton(scrolledWindow, ID_SimReset, _("Reset"), wxDefaultPosition, wxSize(48, -1)),
			_("Reset the editor to initial state")), wxSizerFlags().Expand());
		sizer->Add(gridSizer, wxSizerFlags().Expand());
		UpdateSimButtons();
	}
}

void MapSidebar::OnCollapse(wxCollapsiblePaneEvent& WXUNUSED(evt))
{
	Freeze();

	// Toggling the collapsing doesn't seem to update the sidebar layout
	// automatically, so do it explicitly here
	Layout();

	Refresh(); // fixes repaint glitch on Windows

	Thaw();
}

void MapSidebar::OnFirstDisplay()
{
	// We do this here becase messages are used which requires simulation to be init'd
	m_MapSettingsCtrl->CreateWidgets();
	m_MapSettingsCtrl->ReadFromEngine();

	// Load the map sizes list
	AtlasMessage::qGetMapSizes qrySizes;
	qrySizes.Post();
	AtObj sizes = AtlasObject::LoadFromJSON(*qrySizes.sizes);
	wxChoice* sizeChoice = wxDynamicCast(FindWindow(ID_RandomSize), wxChoice);
	for (AtIter s = sizes["Data"]["item"]; s.defined(); ++s)
	{
		long tiles = 0;
		wxString(s["Tiles"]).ToLong(&tiles);
		sizeChoice->Append(wxString(s["Name"]), (void*)(intptr_t)tiles);
	}
	sizeChoice->SetSelection(0);

	// Load the RMS script list
	AtlasMessage::qGetRMSData qry;
	qry.Post();
	std::vector<std::string> scripts = *qry.data;
	wxChoice* scriptChoice = wxDynamicCast(FindWindow(ID_RandomScript), wxChoice);
	scriptChoice->Clear();
	for (size_t i = 0; i < scripts.size(); ++i)
	{
		AtObj data = AtlasObject::LoadFromJSON(scripts[i]);
		wxString name(data["settings"]["Name"]);
		scriptChoice->Append(name, new AtObjClientData(*data["settings"]));
	}
	scriptChoice->SetSelection(0);

	Layout();
}

void MapSidebar::OnMapReload()
{
	m_MapSettingsCtrl->ReadFromEngine();

	// Reset sim test buttons
	POST_MESSAGE(SimPlay, (0.f, false));
	POST_MESSAGE(SimStopMusic, ());
	POST_MESSAGE(GuiSwitchPage, (L"page_atlas.xml"));
	m_SimState = SimInactive;
	UpdateSimButtons();
}

void MapSidebar::UpdateSimButtons()
{
	wxButton* button;

	button = wxDynamicCast(FindWindow(ID_SimPlay), wxButton);
	wxCHECK(button, );
	button->Enable(m_SimState != SimPlaying);

	button = wxDynamicCast(FindWindow(ID_SimFast), wxButton);
	wxCHECK(button, );
	button->Enable(m_SimState != SimPlayingFast);

	button = wxDynamicCast(FindWindow(ID_SimSlow), wxButton);
	wxCHECK(button, );
	button->Enable(m_SimState != SimPlayingSlow);

	button = wxDynamicCast(FindWindow(ID_SimPause), wxButton);
	wxCHECK(button, );
	button->Enable(IsPlaying(m_SimState));

	button = wxDynamicCast(FindWindow(ID_SimReset), wxButton);
	wxCHECK(button, );
	button->Enable(m_SimState != SimInactive);
}

void MapSidebar::OnSimPlay(wxCommandEvent& event)
{
	float speed = 1.f;
	int newState = SimPlaying;
	if (event.GetId() == ID_SimFast)
	{
		speed = 8.f;
		newState = SimPlayingFast;
	}
	else if (event.GetId() == ID_SimSlow)
	{
		speed = 0.125f;
		newState = SimPlayingSlow;
	}

	if (m_SimState == SimInactive)
	{
		// Force update of player settings
		POST_MESSAGE(LoadPlayerSettings, (false));

		POST_MESSAGE(SimStateSave, (L"default"));
		POST_MESSAGE(GuiSwitchPage, (L"page_session.xml"));
		POST_MESSAGE(SimPlay, (speed, true));
		m_SimState = newState;
	}
	else // paused or already playing at a different speed
	{
		POST_MESSAGE(SimPlay, (speed, true));
		m_SimState = newState;
	}
	UpdateSimButtons();
}

void MapSidebar::OnSimPause(wxCommandEvent& WXUNUSED(event))
{
	if (IsPlaying(m_SimState))
	{
		POST_MESSAGE(SimPlay, (0.f, true));
		m_SimState = SimPaused;
	}
	UpdateSimButtons();
}

void MapSidebar::OnSimReset(wxCommandEvent& WXUNUSED(event))
{
	if (IsPlaying(m_SimState))
	{
		POST_MESSAGE(SimPlay, (0.f, true));
		POST_MESSAGE(SimStateRestore, (L"default"));
		POST_MESSAGE(SimStopMusic, ());
		POST_MESSAGE(SimPlay, (0.f, false));
		POST_MESSAGE(GuiSwitchPage, (L"page_atlas.xml"));
		m_SimState = SimInactive;
	}
	else if (m_SimState == SimPaused)
	{
		POST_MESSAGE(SimPlay, (0.f, true));
		POST_MESSAGE(SimStateRestore, (L"default"));
		POST_MESSAGE(SimStopMusic, ());
		POST_MESSAGE(SimPlay, (0.f, false));
		POST_MESSAGE(GuiSwitchPage, (L"page_atlas.xml"));
		m_SimState = SimInactive;
	}
	UpdateSimButtons();
}

void MapSidebar::OnRandomReseed(wxCommandEvent& WXUNUSED(evt))
{
	// Pick a shortish randomish value
	wxString seed;
	seed << (int)floor((rand() / (float)RAND_MAX) * 10000.f);
	wxDynamicCast(FindWindow(ID_RandomSeed), wxTextCtrl)->SetValue(seed);
}

void MapSidebar::OnRandomGenerate(wxCommandEvent& WXUNUSED(evt))
{
	if (m_ScenarioEditor.DiscardChangesDialog())
		return;

	wxChoice* scriptChoice = wxDynamicCast(FindWindow(ID_RandomScript), wxChoice);

	if (scriptChoice->GetSelection() < 0)
		return;

	// TODO: this settings thing seems a bit of a mess,
	// since it's mixing data from three different sources

	AtObj settings = m_MapSettingsCtrl->UpdateSettingsObject();

	AtObj scriptSettings = dynamic_cast<AtObjClientData*>(scriptChoice->GetClientObject(scriptChoice->GetSelection()))->GetValue();

	settings.addOverlay(scriptSettings);

	wxChoice* sizeChoice = wxDynamicCast(FindWindow(ID_RandomSize), wxChoice);
	wxString size;
	size << (intptr_t)sizeChoice->GetClientData(sizeChoice->GetSelection());
	settings.setInt("Size", wxAtoi(size));

	settings.setInt("Seed", wxAtoi(wxDynamicCast(FindWindow(ID_RandomSeed), wxTextCtrl)->GetValue()));

	std::string json = AtlasObject::SaveToJSON(settings);

	wxBusyInfo busy(_("Generating map"));
	wxBusyCursor busyc;

	wxString scriptName(settings["Script"]);

	// Copy the old map settings, so we don't lose them if the map generation fails
	AtObj oldSettings = settings;

	AtlasMessage::qGenerateMap qry((std::wstring)scriptName.wc_str(), json);
	qry.Post();

	if (qry.status < 0)
	{
		// Display error message and revert to old map settings
		wxLogError(_("Random map script '%ls' failed"), scriptName.wc_str());
		m_MapSettingsCtrl->SetMapSettings(oldSettings);
	}

	m_ScenarioEditor.NotifyOnMapReload();
}

void MapSidebar::OnOpenPlayerPanel(wxCommandEvent& WXUNUSED(evt))
{
	m_ScenarioEditor.SelectPage(_T("PlayerSidebar"));
}

BEGIN_EVENT_TABLE(MapSidebar, Sidebar)
	EVT_COLLAPSIBLEPANE_CHANGED(wxID_ANY, MapSidebar::OnCollapse)
	EVT_BUTTON(ID_SimPlay, MapSidebar::OnSimPlay)
	EVT_BUTTON(ID_SimFast, MapSidebar::OnSimPlay)
	EVT_BUTTON(ID_SimSlow, MapSidebar::OnSimPlay)
	EVT_BUTTON(ID_SimPause, MapSidebar::OnSimPause)
	EVT_BUTTON(ID_SimReset, MapSidebar::OnSimReset)
	EVT_BUTTON(ID_RandomReseed, MapSidebar::OnRandomReseed)
	EVT_BUTTON(ID_RandomGenerate, MapSidebar::OnRandomGenerate)
	EVT_BUTTON(ID_OpenPlayerPanel, MapSidebar::OnOpenPlayerPanel)
END_EVENT_TABLE();
