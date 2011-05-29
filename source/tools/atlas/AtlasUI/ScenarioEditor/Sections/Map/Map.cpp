/* Copyright (C) 2011 Wildfire Games.
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

#include "General/Datafile.h"
#include "ScenarioEditor/Tools/Common/Tools.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "AtlasScript/ScriptInterface.h"

#include "GameInterface/Messages.h"

#include "wx/busyinfo.h"
#include "wx/filename.h"

enum
{
	ID_MapName,
	ID_MapDescription,
	ID_MapReveal,
	ID_MapType,
	ID_MapNumPlayers,
	ID_MapKW_Demo,
	ID_MapKW_Hidden,
	ID_RandomScript,
	ID_RandomSize,
	ID_RandomSeed,
	ID_RandomReseed,
	ID_RandomGenerate,
	ID_SimPlay,
	ID_SimFast,
	ID_SimSlow,
	ID_SimPause,
	ID_SimReset
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

// TODO: Some of these helper things should be moved out of this file
// and into shared locations

class MapSettings : public wxPanel
{
public:
	MapSettings(wxWindow* parent, ScenarioEditor& scenarioEditor);
	void CreateWidgets();
	void ReadFromEngine();
	AtObj UpdateSettingsObject();
private:
	void SendToEngine();

	void OnEdit(wxCommandEvent& WXUNUSED(evt))
	{
		SendToEngine();
	}

	void OnEditSpin(wxSpinEvent& WXUNUSED(evt))
	{
		SendToEngine();
	}

	static const size_t MAX_NUM_PLAYERS = 8;

	AtObj m_MapSettings;
	std::set<std::wstring> m_MapSettingsKeywords;

	std::vector<wxChoice*> m_PlayerCivChoices;

	ScenarioEditor& m_ScenarioEditor;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(MapSettings, wxPanel)
	EVT_TEXT(ID_MapName, MapSettings::OnEdit)
	EVT_TEXT(ID_MapDescription, MapSettings::OnEdit)
	EVT_CHECKBOX(wxID_ANY, MapSettings::OnEdit)
	EVT_CHOICE(wxID_ANY, MapSettings::OnEdit)
	EVT_SPINCTRL(ID_MapNumPlayers, MapSettings::OnEditSpin)
END_EVENT_TABLE();

MapSettings::MapSettings(wxWindow* parent, ScenarioEditor& scenarioEditor)
	: wxPanel(parent, wxID_ANY), m_ScenarioEditor(scenarioEditor)
{
	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Map settings"));
	SetSizer(sizer);
}

void MapSettings::CreateWidgets()
{
	wxSizer* sizer = GetSizer();

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

	wxArrayString gameTypes;
	gameTypes.Add(_T("conquest"));
	gameTypes.Add(_T("endless"));

	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2);
	gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Reveal map")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	gridSizer->Add(new wxCheckBox(this, ID_MapReveal, wxEmptyString));
	gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Game type")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	gridSizer->Add(new wxChoice(this, ID_MapType, wxDefaultPosition, wxDefaultSize, gameTypes));
	gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Num players")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	wxSpinCtrl* numPlayersSpin = new wxSpinCtrl(this, ID_MapNumPlayers, wxEmptyString, wxDefaultPosition, wxSize(40, -1));
	numPlayersSpin->SetRange(1, MAX_NUM_PLAYERS);
	numPlayersSpin->SetValue(MAX_NUM_PLAYERS);
	gridSizer->Add(numPlayersSpin);

	sizer->Add(gridSizer);


	wxArrayString civNames;
	wxArrayString civCodes;
	AtlasMessage::qGetCivData qryCiv;
	qryCiv.Post();
	std::vector<std::string> civData = *qryCiv.data;
	for (size_t i = 0; i < civData.size(); ++i)
	{
		AtObj civ = AtlasObject::LoadFromJSON(m_ScenarioEditor.GetScriptInterface().GetContext(), civData[i]);
		civNames.Add(wxString(civ["Name"]));
		civCodes.Add(wxString(civ["Code"]));
	}

	wxCollapsiblePane* playersPane = new wxCollapsiblePane(this, wxID_ANY, _("Player settings"), wxDefaultPosition, wxDefaultSize, wxCP_NO_TLW_RESIZE);
	wxFlexGridSizer* playersPaneSizer = new wxFlexGridSizer(2);
	playersPaneSizer->Add(new wxStaticText(playersPane->GetPane(), wxID_ANY, _T("")));
	playersPaneSizer->Add(new wxStaticText(playersPane->GetPane(), wxID_ANY, _("Civ")));
	for (size_t i = 0; i < MAX_NUM_PLAYERS; ++i)
	{
		wxString idStr;
		idStr << (i+1);
		playersPaneSizer->Add(new wxStaticText(playersPane->GetPane(), wxID_ANY, idStr), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
		wxChoice* civChoice = new wxChoice(playersPane->GetPane(), wxID_ANY);
		for (size_t j = 0; j < civNames.Count(); ++j)
			civChoice->Append(civNames[j], new wxStringClientData(civCodes[j]));
		m_PlayerCivChoices.push_back(civChoice);
		playersPaneSizer->Add(civChoice);

		// TODO: Team
		// TODO: Resources?
	}
	playersPane->GetPane()->SetSizer(playersPaneSizer);
	sizer->Add(playersPane, wxSizerFlags().Expand());

	wxStaticBoxSizer* keywordsSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Keywords"));
	wxFlexGridSizer* kwGridSizer = new wxFlexGridSizer(2);
	kwGridSizer->Add(new wxStaticText(this, wxID_ANY, _("Demo")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	kwGridSizer->Add(new wxCheckBox(this, ID_MapKW_Demo, wxEmptyString));
	kwGridSizer->Add(new wxStaticText(this, wxID_ANY, _("Hidden")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	kwGridSizer->Add(new wxCheckBox(this, ID_MapKW_Hidden, wxEmptyString));
	keywordsSizer->Add(kwGridSizer);
	sizer->Add(keywordsSizer, wxSizerFlags().Expand());
}

void MapSettings::ReadFromEngine()
{
	AtlasMessage::qGetMapSettings qry;
	qry.Post();
	m_MapSettings = AtlasObject::LoadFromJSON(m_ScenarioEditor.GetScriptInterface().GetContext(), *qry.settings);

	m_MapSettingsKeywords.clear();
	for (AtIter keyword = m_MapSettings["Keywords"]["item"]; keyword.defined(); ++keyword)
		m_MapSettingsKeywords.insert(std::wstring(keyword));

	wxDynamicCast(FindWindow(ID_MapName), wxTextCtrl)->ChangeValue(wxString(m_MapSettings["Name"]));

	wxDynamicCast(FindWindow(ID_MapDescription), wxTextCtrl)->ChangeValue(wxString(m_MapSettings["Description"]));

	wxDynamicCast(FindWindow(ID_MapReveal), wxCheckBox)->SetValue(wxString(m_MapSettings["RevealMap"]) == L"true");

	wxDynamicCast(FindWindow(ID_MapType), wxChoice)->SetStringSelection(wxString(m_MapSettings["GameType"]));

	size_t numPlayers = m_MapSettings["PlayerData"]["item"].count();
	wxDynamicCast(FindWindow(ID_MapNumPlayers), wxSpinCtrl)->SetValue(numPlayers);

	AtIter player = m_MapSettings["PlayerData"]["item"];
	for (size_t i = 0; i < numPlayers && i < MAX_NUM_PLAYERS; ++i, ++player)
	{
		wxChoice* choice = m_PlayerCivChoices[i];
		choice->Enable(true);
		wxString civCode(player["Civ"]);
		for (size_t j = 0; j < choice->GetCount(); ++j)
		{
			wxStringClientData* str = dynamic_cast<wxStringClientData*>(choice->GetClientObject(j));
			if (str->GetData() == civCode)
			{
				choice->SetSelection(j);
				break;
			}
		}
	}
	for (size_t i = numPlayers; i < MAX_NUM_PLAYERS; ++i)
	{
		wxChoice* choice = m_PlayerCivChoices[i];
		choice->SetSelection(0);
		choice->Enable(false);
	}

	wxDynamicCast(FindWindow(ID_MapKW_Demo), wxCheckBox)->SetValue(m_MapSettingsKeywords.count(L"demo") != 0);
	wxDynamicCast(FindWindow(ID_MapKW_Hidden), wxCheckBox)->SetValue(m_MapSettingsKeywords.count(L"hidden") != 0);
}

AtObj MapSettings::UpdateSettingsObject()
{
	m_MapSettings.set("Name", wxDynamicCast(FindWindow(ID_MapName), wxTextCtrl)->GetValue());

	m_MapSettings.set("Description", wxDynamicCast(FindWindow(ID_MapDescription), wxTextCtrl)->GetValue());

	m_MapSettings.setBool("RevealMap", wxDynamicCast(FindWindow(ID_MapReveal), wxCheckBox)->GetValue());

	m_MapSettings.set("MapType", wxDynamicCast(FindWindow(ID_MapType), wxChoice)->GetStringSelection());

	AtIter oldPlayer = m_MapSettings["PlayerData"]["item"];
	AtObj players;
	players.set("@array", L"");
	size_t numPlayers = (size_t)wxDynamicCast(FindWindow(ID_MapNumPlayers), wxSpinCtrl)->GetValue();
	for (size_t i = 0; i < numPlayers && i < MAX_NUM_PLAYERS; ++i)
	{
		wxChoice* choice = m_PlayerCivChoices[i];
		choice->Enable(true);
		AtObj player = *oldPlayer;
		if (choice->GetSelection() >= 0)
		{
			wxStringClientData* str = dynamic_cast<wxStringClientData*>(choice->GetClientObject(choice->GetSelection()));
			player.set("Civ", str->GetData());
		}
		players.add("item", player);
		if (oldPlayer.defined())
			++oldPlayer;
	}
	for (size_t i = numPlayers; i < MAX_NUM_PLAYERS; ++i)
	{
		wxChoice* choice = m_PlayerCivChoices[i];
		choice->Enable(false);
	}
	m_MapSettings.set("PlayerData", players);

	if (wxDynamicCast(FindWindow(ID_MapKW_Demo), wxCheckBox)->GetValue())
		m_MapSettingsKeywords.insert(L"demo");
	else
		m_MapSettingsKeywords.erase(L"demo");

	if (wxDynamicCast(FindWindow(ID_MapKW_Hidden), wxCheckBox)->GetValue())
		m_MapSettingsKeywords.insert(L"hidden");
	else
		m_MapSettingsKeywords.erase(L"hidden");

	AtObj keywords;
	keywords.set("@array", L"");
	for (std::set<std::wstring>::iterator it = m_MapSettingsKeywords.begin(); it != m_MapSettingsKeywords.end(); ++it)
		keywords.add("item", it->c_str());
	m_MapSettings.set("Keywords", keywords);

	return m_MapSettings;
}

void MapSettings::SendToEngine()
{
	UpdateSettingsObject();

	std::string json = AtlasObject::SaveToJSON(m_ScenarioEditor.GetScriptInterface().GetContext(), m_MapSettings);

	// TODO: would be nice if we supported undo for settings changes

	POST_MESSAGE(SetMapSettings, (json));
}


MapSidebar::MapSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
	: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer), m_SimState(SimInactive)
{
	m_MapSettings = new MapSettings(this, m_ScenarioEditor);
	m_MainSizer->Add(m_MapSettings, wxSizerFlags().Expand());

	{
		wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Random map"));

		sizer->Add(new wxChoice(this, ID_RandomScript), wxSizerFlags().Expand());

		wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2);
		gridSizer->AddGrowableCol(1);

		wxChoice* sizeChoice = new wxChoice(this, ID_RandomSize);
		AtObj sizes(Datafile::ReadList("mapsizes"));
		for (AtIter s = sizes["size"]; s.defined(); ++s)
		{
			long tiles = 0;
			wxString(s["@tiles"]).ToLong(&tiles);
			sizeChoice->Append(wxString(s["@name"]), (void*)(intptr_t)tiles);
		}
		sizeChoice->SetSelection(0);
		gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Map size")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
		gridSizer->Add(sizeChoice, wxSizerFlags().Expand());

		gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Random seed")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
		wxBoxSizer* seedSizer = new wxBoxSizer(wxHORIZONTAL);
		seedSizer->Add(new wxTextCtrl(this, ID_RandomSeed, _T("0")), wxSizerFlags(1).Expand());
		seedSizer->Add(new wxButton(this, ID_RandomReseed, _("R"), wxDefaultPosition, wxSize(24, -1)));
		gridSizer->Add(seedSizer, wxSizerFlags().Expand());

		sizer->Add(gridSizer, wxSizerFlags().Expand());

		sizer->Add(new wxButton(this, ID_RandomGenerate, _("Generate map")), wxSizerFlags().Expand());

		m_MainSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 16));
	}

	{
		wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Simulation test"));
		sizer->Add(new wxButton(this, ID_SimPlay, _("Play")), wxSizerFlags().Proportion(1));
		sizer->Add(new wxButton(this, ID_SimFast, _("Fast")), wxSizerFlags().Proportion(1));
		sizer->Add(new wxButton(this, ID_SimSlow, _("Slow")), wxSizerFlags().Proportion(1));
		sizer->Add(new wxButton(this, ID_SimPause, _("Pause")), wxSizerFlags().Proportion(1));
		sizer->Add(new wxButton(this, ID_SimReset, _("Reset")), wxSizerFlags().Proportion(1));
		UpdateSimButtons();
		m_MainSizer->Add(sizer, wxSizerFlags().Expand().Border(wxTOP, 16));
	}
}

void MapSidebar::OnCollapse(wxCollapsiblePaneEvent& WXUNUSED(evt))
{
	// Toggling the collapsing doesn't seem to update the sidebar layout
	// automatically, so do it explicitly here
	Layout();
}

void MapSidebar::OnFirstDisplay()
{
	m_MapSettings->CreateWidgets();
	m_MapSettings->ReadFromEngine();

	// Load the RMS script list:

	AtlasMessage::qGetRMSData qry;
	qry.Post();
	std::vector<std::string> scripts = *qry.data;

	wxChoice* scriptChoice = wxDynamicCast(FindWindow(ID_RandomScript), wxChoice);
	scriptChoice->Clear();
	for (size_t i = 0; i < scripts.size(); ++i)
	{
		AtObj data = AtlasObject::LoadFromJSON(m_ScenarioEditor.GetScriptInterface().GetContext(), scripts[i]);
		wxString name(data["settings"]["Name"]);
		scriptChoice->Append(name, new AtObjClientData(*data["settings"]));
	}
	scriptChoice->SetSelection(0);
}

void MapSidebar::OnMapReload()
{
	m_MapSettings->ReadFromEngine();
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
		POST_MESSAGE(SimStateSave, (L"default"));
		POST_MESSAGE(GuiSwitchPage, (L"page_session.xml"));
		POST_MESSAGE(SimPlay, (speed));
		m_SimState = newState;
	}
	else // paused or already playing at a different speed
	{
		POST_MESSAGE(SimPlay, (speed));
		m_SimState = newState;
	}
	UpdateSimButtons();
}

void MapSidebar::OnSimPause(wxCommandEvent& WXUNUSED(event))
{
	if (IsPlaying(m_SimState))
	{
		POST_MESSAGE(SimPlay, (0.f));
		m_SimState = SimPaused;
	}
	UpdateSimButtons();
}

void MapSidebar::OnSimReset(wxCommandEvent& WXUNUSED(event))
{
	if (IsPlaying(m_SimState))
	{
		POST_MESSAGE(SimPlay, (0.f));
		POST_MESSAGE(SimStateRestore, (L"default"));
		POST_MESSAGE(GuiSwitchPage, (L"page_atlas.xml"));
		m_SimState = SimInactive;
	}
	else if (m_SimState == SimPaused)
	{
		POST_MESSAGE(SimStateRestore, (L"default"));
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
	wxChoice* scriptChoice = wxDynamicCast(FindWindow(ID_RandomScript), wxChoice);

	if (scriptChoice->GetSelection() < 0)
		return;

	// TODO: this settings thing seems a bit of a mess,
	// since it's mixing data from three different sources

	AtObj settings = m_MapSettings->UpdateSettingsObject();

	AtObj scriptSettings = dynamic_cast<AtObjClientData*>(scriptChoice->GetClientObject(scriptChoice->GetSelection()))->GetValue();

	settings.addOverlay(scriptSettings);

	wxChoice* sizeChoice = wxDynamicCast(FindWindow(ID_RandomSize), wxChoice);
	wxString size;
	size << (intptr_t)sizeChoice->GetClientData(sizeChoice->GetSelection());
	AtObj sizeObj;
	sizeObj.setString(size);
	sizeObj.set("@number", L"");
	settings.set("Size", sizeObj);

	AtObj seedObj;
	seedObj.setString(wxDynamicCast(FindWindow(ID_RandomSeed), wxTextCtrl)->GetValue());
	seedObj.set("@number", L"");
	settings.set("Seed", seedObj);

	std::string json = AtlasObject::SaveToJSON(m_ScenarioEditor.GetScriptInterface().GetContext(), settings);

	wxBusyInfo(_("Generating map"));

	wxString scriptName(settings["Script"]);

	AtlasMessage::qGenerateMap qry(scriptName.c_str(), json);
	qry.Post();

	if (qry.status < 0)
		wxLogError(_("Random map script '%ls' failed"), scriptName.c_str());

	m_ScenarioEditor.NotifyOnMapReload();
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
END_EVENT_TABLE();
