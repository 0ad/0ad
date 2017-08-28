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

#include "Player.h"

#include "AtlasObject/AtlasObject.h"
#include "CustomControls/ColorDialog/ColorDialog.h"
#include "ScenarioEditor/ScenarioEditor.h"

#include "wx/choicebk.h"

enum
{
	ID_NumPlayers,
	ID_PlayerFood,
	ID_PlayerWood,
	ID_PlayerMetal,
	ID_PlayerStone,
	ID_PlayerPop,
	ID_PlayerColor,

	ID_DefaultName,
	ID_DefaultCiv,
	ID_DefaultColor,
	ID_DefaultAI,
	ID_DefaultFood,
	ID_DefaultWood,
	ID_DefaultMetal,
	ID_DefaultStone,
	ID_DefaultPop,
	ID_DefaultTeam,

	ID_CameraSet,
	ID_CameraView,
	ID_CameraClear
};

// TODO: Some of these helper things should be moved out of this file
// and into shared locations

// Helper function for adding tooltips
static wxWindow* Tooltipped(wxWindow* window, const wxString& tip)
{
	window->SetToolTip(tip);
	return window;
}

//////////////////////////////////////////////////////////////////////////

class DefaultCheckbox : public wxCheckBox
{
public:
	DefaultCheckbox(wxWindow* parent, wxWindowID id, wxWindow* control, bool initialValue = false)
		: wxCheckBox(parent, id, wxEmptyString), m_Control(control)
	{
		SetValue(initialValue);
	}

	virtual void SetValue(bool value)
	{
		m_Control->Enable(value);
		wxCheckBox::SetValue(value);
	}

	void OnChecked(wxCommandEvent& evt)
	{
		m_Control->Enable(evt.IsChecked());

		evt.Skip();
	}

private:
	wxWindow* m_Control;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(DefaultCheckbox, wxCheckBox)
	EVT_CHECKBOX(wxID_ANY, DefaultCheckbox::OnChecked)
END_EVENT_TABLE();


class PlayerNotebookPage : public wxPanel
{

public:
	PlayerNotebookPage(wxWindow* parent, const wxString& name, size_t playerID)
		: wxPanel(parent, wxID_ANY), m_Name(name), m_PlayerID(playerID)
	{

		m_Controls.page = this;

		Freeze();

		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer);

		{
			/////////////////////////////////////////////////////////////////////////
			// Player Info
			wxStaticBoxSizer* playerInfoSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Player info"));
			wxFlexGridSizer* gridSizer = new wxFlexGridSizer(3, 5, 5);
			gridSizer->AddGrowableCol(2);

			wxTextCtrl* nameCtrl = new wxTextCtrl(this, wxID_ANY);
			gridSizer->Add(new DefaultCheckbox(this, ID_DefaultName, nameCtrl), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
			gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Name")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
			gridSizer->Add(nameCtrl, wxSizerFlags(1).Expand().Align(wxALIGN_RIGHT));
			m_Controls.name = nameCtrl;

			wxChoice* civChoice = new wxChoice(this, wxID_ANY);
			gridSizer->Add(new DefaultCheckbox(this, ID_DefaultCiv, civChoice), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
			gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Civilisation")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
			gridSizer->Add(civChoice, wxSizerFlags(1).Expand().Align(wxALIGN_RIGHT));
			m_Controls.civ = civChoice;

			wxButton* colorButton = new wxButton(this, ID_PlayerColor);
			gridSizer->Add(new DefaultCheckbox(this, ID_DefaultColor, colorButton), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
			gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Color")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
			gridSizer->Add(Tooltipped(colorButton,
				_("Set player color")), wxSizerFlags(1).Expand().Align(wxALIGN_RIGHT));
			m_Controls.color = colorButton;

			wxChoice* aiChoice = new wxChoice(this, wxID_ANY);
			gridSizer->Add(new DefaultCheckbox(this, ID_DefaultAI, aiChoice), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
			gridSizer->Add(new wxStaticText(this, wxID_ANY, _("AI")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
			gridSizer->Add(Tooltipped(aiChoice,
				_("Select AI")), wxSizerFlags(1).Expand().Align(wxALIGN_RIGHT));
			m_Controls.ai = aiChoice;

			playerInfoSizer->Add(gridSizer, wxSizerFlags(1).Expand());
			sizer->Add(playerInfoSizer, wxSizerFlags().Expand().Border(wxTOP, 10));
		}

		{
			/////////////////////////////////////////////////////////////////////////
			// Resources
			wxStaticBoxSizer* resourceSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Resources"));
			wxFlexGridSizer* gridSizer = new wxFlexGridSizer(3, 5, 5);
			gridSizer->AddGrowableCol(2);

			wxSpinCtrl* foodCtrl = new wxSpinCtrl(this, ID_PlayerFood, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, INT_MAX);
			gridSizer->Add(new DefaultCheckbox(this, ID_DefaultFood, foodCtrl), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
			gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Food")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
			gridSizer->Add(Tooltipped(foodCtrl,
				_("Initial value of food resource")), wxSizerFlags().Expand());
			m_Controls.food = foodCtrl;

			wxSpinCtrl* woodCtrl = new wxSpinCtrl(this, ID_PlayerWood, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, INT_MAX);
			gridSizer->Add(new DefaultCheckbox(this, ID_DefaultWood, woodCtrl), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
			gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Wood")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
			gridSizer->Add(Tooltipped(woodCtrl,
				_("Initial value of wood resource")), wxSizerFlags().Expand());
			m_Controls.wood = woodCtrl;

			wxSpinCtrl* metalCtrl = new wxSpinCtrl(this, ID_PlayerMetal, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, INT_MAX);
			gridSizer->Add(new DefaultCheckbox(this, ID_DefaultMetal, metalCtrl), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
			gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Metal")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
			gridSizer->Add(Tooltipped(metalCtrl,
				_("Initial value of metal resource")), wxSizerFlags().Expand());
			m_Controls.metal = metalCtrl;

			wxSpinCtrl* stoneCtrl = new wxSpinCtrl(this, ID_PlayerStone, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, INT_MAX);
			gridSizer->Add(new DefaultCheckbox(this, ID_DefaultStone, stoneCtrl), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
			gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Stone")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
			gridSizer->Add(Tooltipped(stoneCtrl,
				_("Initial value of stone resource")), wxSizerFlags().Expand());
			m_Controls.stone = stoneCtrl;

			wxSpinCtrl* popCtrl = new wxSpinCtrl(this, ID_PlayerPop, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, INT_MAX);
			gridSizer->Add(new DefaultCheckbox(this, ID_DefaultPop, popCtrl), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
			gridSizer->Add(new wxStaticText(this, wxID_ANY, _("Pop limit")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
			gridSizer->Add(Tooltipped(popCtrl,
				_("Population limit for this player")), wxSizerFlags().Expand());
			m_Controls.pop = popCtrl;

			resourceSizer->Add(gridSizer, wxSizerFlags(1).Expand());
			sizer->Add(resourceSizer, wxSizerFlags().Expand().Border(wxTOP, 10));
		}
		{
			/////////////////////////////////////////////////////////////////////////
			// Diplomacy
			wxStaticBoxSizer* diplomacySizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Diplomacy"));
			wxBoxSizer* boxSizer = new wxBoxSizer(wxHORIZONTAL);
			wxChoice* teamCtrl = new wxChoice(this, wxID_ANY);
			boxSizer->Add(new DefaultCheckbox(this, ID_DefaultTeam, teamCtrl), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
			boxSizer->AddSpacer(5);
			boxSizer->Add(new wxStaticText(this, wxID_ANY, _("Team")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
			boxSizer->AddSpacer(5);
			teamCtrl->Append(_("None"));
			teamCtrl->Append(_T("1"));
			teamCtrl->Append(_T("2"));
			teamCtrl->Append(_T("3"));
			teamCtrl->Append(_T("4"));
			boxSizer->Add(teamCtrl);
			m_Controls.team = teamCtrl;
			diplomacySizer->Add(boxSizer, wxSizerFlags(1).Expand());

			// TODO: possibly have advanced panel where each player's diplomacy can be set?
			// Advanced panel
			/*wxCollapsiblePane* advPane = new wxCollapsiblePane(this, wxID_ANY, _("Advanced"));
			wxWindow* pane = advPane->GetPane();
			diplomacySizer->Add(advPane, 0, wxGROW | wxALL, 2);*/

			sizer->Add(diplomacySizer, wxSizerFlags().Expand().Border(wxTOP, 10));
		}

		{
			/////////////////////////////////////////////////////////////////////////
			// Camera
			wxStaticBoxSizer* cameraSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Starting Camera"));
			wxGridSizer* gridSizer = new wxGridSizer(3);
			wxButton* cameraSet = new wxButton(this, ID_CameraSet, _("Set"), wxDefaultPosition, wxSize(48, -1));
			gridSizer->Add(Tooltipped(cameraSet,
				_("Set player camera to this view")), wxSizerFlags().Expand());
			wxButton* cameraView = new wxButton(this, ID_CameraView, _("View"), wxDefaultPosition, wxSize(48, -1));
			cameraView->Enable(false);
			gridSizer->Add(Tooltipped(cameraView,
				_("View the player camera")), wxSizerFlags().Expand());
			wxButton* cameraClear = new wxButton(this, ID_CameraClear, _("Clear"), wxDefaultPosition, wxSize(48, -1));
			cameraClear->Enable(false);
			gridSizer->Add(Tooltipped(cameraClear,
				_("Clear player camera")), wxSizerFlags().Expand());
			cameraSizer->Add(gridSizer, wxSizerFlags().Expand());

			sizer->Add(cameraSizer, wxSizerFlags().Expand().Border(wxTOP, 10));
		}

		Layout();
		Thaw();

	}

	void OnDisplay()
	{
	}

	PlayerPageControls GetControls()
	{
		return m_Controls;
	}

	wxString GetPlayerName()
	{
		return m_Name;
	}

	size_t GetPlayerID()
	{
		return m_PlayerID;
	}

	bool IsCameraDefined()
	{
		return m_CameraDefined;
	}

	sCameraInfo GetCamera()
	{
		return m_Camera;
	}

	void SetCamera(sCameraInfo info, bool isDefined = true)
	{
		m_Camera = info;
		m_CameraDefined = isDefined;

		// Enable/disable controls
		wxDynamicCast(FindWindow(ID_CameraView), wxButton)->Enable(isDefined);
		wxDynamicCast(FindWindow(ID_CameraClear), wxButton)->Enable(isDefined);
	}

private:
	void OnColor(wxCommandEvent& evt)
	{
		// Show color dialog
		ColorDialog colorDlg(this, _T("Scenario Editor/PlayerColor"), m_Controls.color->GetBackgroundColour());

		if (colorDlg.ShowModal() == wxID_OK)
		{
			m_Controls.color->SetBackgroundColour(colorDlg.GetColourData().GetColour());

			// Pass event on to next handler
			evt.Skip();
		}
	}

	void OnCameraSet(wxCommandEvent& evt)
	{
		AtlasMessage::qGetView qryView;
		qryView.Post();
		SetCamera(qryView.info, true);

		// Pass event on to next handler
		evt.Skip();
	}

	void OnCameraView(wxCommandEvent& WXUNUSED(evt))
	{
		POST_MESSAGE(SetView, (m_Camera));
	}

	void OnCameraClear(wxCommandEvent& evt)
	{
		SetCamera(sCameraInfo(), false);

		// Pass event on to next handler
		evt.Skip();
	}

	sCameraInfo m_Camera;
	bool m_CameraDefined;
	wxString m_Name;
	size_t m_PlayerID;

	PlayerPageControls m_Controls;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(PlayerNotebookPage, wxPanel)
	EVT_BUTTON(ID_PlayerColor, PlayerNotebookPage::OnColor)
	EVT_BUTTON(ID_CameraSet, PlayerNotebookPage::OnCameraSet)
	EVT_BUTTON(ID_CameraView, PlayerNotebookPage::OnCameraView)
	EVT_BUTTON(ID_CameraClear, PlayerNotebookPage::OnCameraClear)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////

class PlayerNotebook : public wxChoicebook
{
public:
	PlayerNotebook(wxWindow *parent)
		: wxChoicebook(parent, wxID_ANY/*, wxDefaultPosition, wxDefaultSize, wxNB_FIXEDWIDTH*/)
	{
	}

	PlayerPageControls AddPlayer(wxString name, size_t player)
	{
		PlayerNotebookPage* playerPage = new PlayerNotebookPage(this, name, player);
		AddPage(playerPage, name);
		m_Pages.push_back(playerPage);
		return playerPage->GetControls();
	}

	void ResizePlayers(size_t numPlayers)
	{
		wxASSERT(numPlayers <= m_Pages.size());

		// We don't really want to destroy the windows corresponding
		//	to the tabs, so we've kept them in a vector and will
		//	only remove and add them to the notebook as needed
		int selection = GetSelection();
		size_t pageCount = GetPageCount();

		if (numPlayers > pageCount)
		{
			// Add previously removed pages
			for (size_t i = pageCount; i < numPlayers; ++i)
			{
				AddPage(m_Pages[i], m_Pages[i]->GetPlayerName());
			}
		}
		else
		{
			// Remove previously added pages
			// we have to manually hide them or they remain visible
			for (size_t i = pageCount - 1; i >= numPlayers; --i)
			{
				m_Pages[i]->Hide();
				RemovePage(i);
			}
		}

		// Workaround for bug on wxGTK 2.8: wxChoice selection doesn't update
		//	(in fact it loses its selection when adding/removing pages)
		GetChoiceCtrl()->SetSelection(selection);
	}

protected:
	void OnPageChanged(wxChoicebookEvent& evt)
	{
		if (evt.GetSelection() >= 0 && evt.GetSelection() < (int)GetPageCount())
		{
			static_cast<PlayerNotebookPage*>(GetPage(evt.GetSelection()))->OnDisplay();
		}
		evt.Skip();
	}

private:
	std::vector<PlayerNotebookPage*> m_Pages;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(PlayerNotebook, wxChoicebook)
	EVT_CHOICEBOOK_PAGE_CHANGED(wxID_ANY, PlayerNotebook::OnPageChanged)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////

class PlayerSettingsControl : public wxPanel
{
public:
	PlayerSettingsControl(wxWindow* parent, ScenarioEditor& scenarioEditor);
	void CreateWidgets();
	void LoadDefaults();
	void ReadFromEngine();
	AtObj UpdateSettingsObject();

private:
	void SendToEngine();

	void OnEdit(wxCommandEvent& WXUNUSED(evt))
	{
		if (!m_InGUIUpdate)
		{
			SendToEngine();
		}
	}

	void OnEditSpin(wxSpinEvent& WXUNUSED(evt))
	{
		if (!m_InGUIUpdate)
		{
			SendToEngine();
		}
	}

	void OnPlayerColor(wxCommandEvent& WXUNUSED(evt))
	{
		if (!m_InGUIUpdate)
		{
			SendToEngine();

			// Update player settings, to show new color
			POST_MESSAGE(LoadPlayerSettings, (false));
		}
	}

	void OnNumPlayersText(wxCommandEvent& WXUNUSED(evt))
	{	// Ignore because it will also trigger EVT_SPINCTRL
		//	and we don't want to handle the same event twice
	}

	void OnNumPlayersSpin(wxSpinEvent& evt)
	{
		if (!m_InGUIUpdate)
		{
			wxASSERT(evt.GetInt() > 0);

			// When wxMessageBox pops up, wxSpinCtrl loses focus, which
			//	forces another EVT_SPINCTRL event, which we don't want
			//	to handle, so we check here for a change
			if (evt.GetInt() == (int)m_NumPlayers)
			{
				return;	// No change
			}

			size_t oldNumPlayers = m_NumPlayers;
			m_NumPlayers = evt.GetInt();

			if (m_NumPlayers < oldNumPlayers)
			{
				// Remove players, but check if they own any entities
				bool notified = false;
				for (size_t i = oldNumPlayers; i > m_NumPlayers; --i)
				{
					qGetPlayerObjects objectsQry(i);
					objectsQry.Post();

					std::vector<AtlasMessage::ObjectID> ids = *objectsQry.ids;

					if (ids.size() > 0)
					{
						if (!notified)
						{
							// TODO: Add option to reassign objects?
							if (wxMessageBox(_("WARNING: All objects belonging to the removed players will be deleted. Continue anyway?"), _("Remove player confirmation"), wxICON_EXCLAMATION | wxYES_NO) != wxYES)
							{
								// Restore previous player count
								m_NumPlayers = oldNumPlayers;
								wxDynamicCast(FindWindow(ID_NumPlayers), wxSpinCtrl)->SetValue(m_NumPlayers);
								return;
							}

							notified = true;
						}

						// Delete objects
						// TODO: Merge multiple commands?
						POST_COMMAND(DeleteObjects, (ids));
					}
				}
			}

			m_Players->ResizePlayers(m_NumPlayers);
			SendToEngine();

			// Reload players, notify observers
			POST_MESSAGE(LoadPlayerSettings, (true));
			m_MapSettings.NotifyObservers();
		}
	}

	// TODO: we shouldn't hardcode this, but instead dynamically create
	//	new player notebook pages on demand; of course the default data
	//	will be limited by the entries in player_defaults.json
	static const size_t MAX_NUM_PLAYERS = 8;

	bool m_InGUIUpdate;
	AtObj m_PlayerDefaults;
	PlayerNotebook* m_Players;
	std::vector<PlayerPageControls> m_PlayerControls;
	Observable<AtObj>& m_MapSettings;
	size_t m_NumPlayers;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(PlayerSettingsControl, wxPanel)
	EVT_BUTTON(ID_PlayerColor, PlayerSettingsControl::OnPlayerColor)
	EVT_BUTTON(ID_CameraSet, PlayerSettingsControl::OnEdit)
	EVT_BUTTON(ID_CameraClear, PlayerSettingsControl::OnEdit)
	EVT_CHECKBOX(wxID_ANY, PlayerSettingsControl::OnEdit)
	EVT_CHOICE(wxID_ANY, PlayerSettingsControl::OnEdit)
	EVT_TEXT(ID_NumPlayers, PlayerSettingsControl::OnNumPlayersText)
	EVT_TEXT(wxID_ANY, PlayerSettingsControl::OnEdit)
	EVT_SPINCTRL(ID_NumPlayers, PlayerSettingsControl::OnNumPlayersSpin)
	EVT_SPINCTRL(ID_PlayerFood, PlayerSettingsControl::OnEditSpin)
	EVT_SPINCTRL(ID_PlayerWood, PlayerSettingsControl::OnEditSpin)
	EVT_SPINCTRL(ID_PlayerMetal, PlayerSettingsControl::OnEditSpin)
	EVT_SPINCTRL(ID_PlayerStone, PlayerSettingsControl::OnEditSpin)
	EVT_SPINCTRL(ID_PlayerPop, PlayerSettingsControl::OnEditSpin)
END_EVENT_TABLE();

PlayerSettingsControl::PlayerSettingsControl(wxWindow* parent, ScenarioEditor& scenarioEditor)
	: wxPanel(parent, wxID_ANY), m_InGUIUpdate(false), m_MapSettings(scenarioEditor.GetMapSettings()), m_NumPlayers(0)
{
	// To prevent recursion, don't handle GUI events right now
	m_InGUIUpdate = true;

	wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Player settings"));
	SetSizer(sizer);

	wxBoxSizer* boxSizer = new wxBoxSizer(wxHORIZONTAL);
	boxSizer->Add(new wxStaticText(this, wxID_ANY, _("Num players")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
	wxSpinCtrl* numPlayersSpin = new wxSpinCtrl(this, ID_NumPlayers, wxEmptyString, wxDefaultPosition, wxSize(40, -1));
	numPlayersSpin->SetValue(MAX_NUM_PLAYERS);
	numPlayersSpin->SetRange(1, MAX_NUM_PLAYERS);
	boxSizer->Add(numPlayersSpin);
	sizer->Add(boxSizer, wxSizerFlags().Expand().Proportion(0));
	sizer->AddSpacer(5);
	m_Players = new PlayerNotebook(this);
	sizer->Add(m_Players, wxSizerFlags().Expand().Proportion(1));

	m_InGUIUpdate = false;
}

void PlayerSettingsControl::CreateWidgets()
{
	// To prevent recursion, don't handle GUI events right now
	m_InGUIUpdate = true;

	// Load default civ and player data
	wxArrayString civNames;
	wxArrayString civCodes;
	AtlasMessage::qGetCivData qryCiv;
	qryCiv.Post();
	std::vector<std::string> civData = *qryCiv.data;
	for (size_t i = 0; i < civData.size(); ++i)
	{
		AtObj civ = AtlasObject::LoadFromJSON(civData[i]);
		civNames.Add(wxString(civ["Name"]));
		civCodes.Add(wxString(civ["Code"]));
	}

	// Load AI data
	ArrayOfAIData ais(AIData::CompareAIData);
	AtlasMessage::qGetAIData qryAI;
	qryAI.Post();
	AtObj aiData = AtlasObject::LoadFromJSON(*qryAI.data);
	for (AtIter a = aiData["AIData"]["item"]; a.defined(); ++a)
	{
		ais.Add(new AIData(wxString(a["id"]), wxString(a["data"]["name"])));
	}

	// Create player pages
	AtIter playerDefs = m_PlayerDefaults["item"];
	if (playerDefs.defined())
		++playerDefs;	// Skip gaia

	for (size_t i = 0; i < MAX_NUM_PLAYERS; ++i)
	{
		// Create new player tab and get controls
		wxString name(_("Unknown"));
		if (playerDefs["Name"].defined())
			name = playerDefs["Name"];

		PlayerPageControls controls = m_Players->AddPlayer(name, i);
		m_PlayerControls.push_back(controls);

		// Populate civ choice box
		wxChoice* civChoice = controls.civ;
		for (size_t j = 0; j < civNames.Count(); ++j)
			civChoice->Append(civNames[j], new wxStringClientData(civCodes[j]));
		civChoice->SetSelection(0);

		// Populate ai choice box
		wxChoice* aiChoice = controls.ai;
		aiChoice->Append(_("<None>"), new wxStringClientData());
		for (size_t j = 0; j < ais.Count(); ++j)
			aiChoice->Append(ais[j]->GetName(), new wxStringClientData(ais[j]->GetID()));
		aiChoice->SetSelection(0);

		// Only increment AtIters if they are defined
		if (playerDefs.defined())
			++playerDefs;
	}

	m_InGUIUpdate = false;
}

void PlayerSettingsControl::LoadDefaults()
{
	AtlasMessage::qGetPlayerDefaults qryPlayers;
	qryPlayers.Post();
	AtObj playerData = AtlasObject::LoadFromJSON(*qryPlayers.defaults);
	m_PlayerDefaults = *playerData["PlayerData"];
}

void PlayerSettingsControl::ReadFromEngine()
{
	AtlasMessage::qGetMapSettings qry;
	qry.Post();

	if (!(*qry.settings).empty())
	{
		// Prevent error if there's no map settings to parse
		m_MapSettings = AtlasObject::LoadFromJSON(*qry.settings);
	}
	else
	{
		// Use blank object, it will be created next
		m_MapSettings = AtObj();
	}

	AtIter player = m_MapSettings["PlayerData"]["item"];
	if (!m_MapSettings.defined() || !player.defined() || player.count() == 0)
	{
		// Player data missing - set number of players to max
		m_NumPlayers = MAX_NUM_PLAYERS;
	}
	else
	{
		++player; // skip gaia
		m_NumPlayers = player.count();
	}

	wxASSERT(m_NumPlayers <= MAX_NUM_PLAYERS && m_NumPlayers != 0);

	// To prevent recursion, don't handle GUI events right now
	m_InGUIUpdate = true;

	wxDynamicCast(FindWindow(ID_NumPlayers), wxSpinCtrl)->SetValue(m_NumPlayers);

	// Remove / add extra player pages as needed
	m_Players->ResizePlayers(m_NumPlayers);

	// Update player controls with player data
	AtIter playerDefs = m_PlayerDefaults["item"];
	if (playerDefs.defined())
		++playerDefs;	// skip gaia

	for (size_t i = 0; i < MAX_NUM_PLAYERS; ++i)
	{
		const PlayerPageControls& controls = m_PlayerControls[i];

		// name
		wxString name(_("Unknown"));
		bool defined = player["Name"].defined();
		if (defined)
			name = wxString(player["Name"]);
		else if (playerDefs["Name"].defined())
			name = wxString(playerDefs["Name"]);

		controls.name->SetValue(name);
		wxDynamicCast(FindWindowById(ID_DefaultName, controls.page), DefaultCheckbox)->SetValue(defined);

		// civ
		wxChoice* choice = controls.civ;
		wxString civCode;
		defined = player["Civ"].defined();
		if (defined)
			civCode = wxString(player["Civ"]);
		else
			civCode = wxString(playerDefs["Civ"]);

		for (size_t j = 0; j < choice->GetCount(); ++j)
		{
			wxStringClientData* str = dynamic_cast<wxStringClientData*>(choice->GetClientObject(j));
			if (str->GetData() == civCode)
			{
				choice->SetSelection(j);
				break;
			}
		}
		wxDynamicCast(FindWindowById(ID_DefaultCiv, controls.page), DefaultCheckbox)->SetValue(defined);

		// color
		wxColor color;
		AtObj clrObj = *player["Color"];
		defined = clrObj.defined();
		if (!defined)
			clrObj = *playerDefs["Color"];
		color = wxColor((*clrObj["r"]).getInt(), (*clrObj["g"]).getInt(), (*clrObj["b"]).getInt());
		controls.color->SetBackgroundColour(color);
		wxDynamicCast(FindWindowById(ID_DefaultColor, controls.page), DefaultCheckbox)->SetValue(defined);

		// player type
		wxString aiID;
		defined = player["AI"].defined();
		if (defined)
			aiID = wxString(player["AI"]);
		else
			aiID = wxString(playerDefs["AI"]);

		choice = controls.ai;
		if (!aiID.empty())
		{
			// AI
			for (size_t j = 0; j < choice->GetCount(); ++j)
			{
				wxStringClientData* str = dynamic_cast<wxStringClientData*>(choice->GetClientObject(j));
				if (str->GetData() == aiID)
				{
					choice->SetSelection(j);
					break;
				}
			}
		}
		else // Human
			choice->SetSelection(0);
		wxDynamicCast(FindWindowById(ID_DefaultAI, controls.page), DefaultCheckbox)->SetValue(defined);

		// resources
		AtObj resObj = *player["Resources"];
		defined = resObj.defined() && resObj["food"].defined();
		if (defined)
			controls.food->SetValue(wxString(resObj["food"]));
		else
			controls.food->SetValue(0);
		wxDynamicCast(FindWindowById(ID_DefaultFood, controls.page), DefaultCheckbox)->SetValue(defined);

		defined = resObj.defined() && resObj["wood"].defined();
		if (defined)
			controls.wood->SetValue(wxString(resObj["wood"]));
		else
			controls.wood->SetValue(0);
		wxDynamicCast(FindWindowById(ID_DefaultWood, controls.page), DefaultCheckbox)->SetValue(defined);

		defined = resObj.defined() && resObj["metal"].defined();
		if (defined)
			controls.metal->SetValue(wxString(resObj["metal"]));
		else
			controls.metal->SetValue(0);
		wxDynamicCast(FindWindowById(ID_DefaultMetal, controls.page), DefaultCheckbox)->SetValue(defined);

		defined = resObj.defined() && resObj["stone"].defined();
		if (defined)
			controls.stone->SetValue(wxString(resObj["stone"]));
		else
			controls.stone->SetValue(0);
		wxDynamicCast(FindWindowById(ID_DefaultStone, controls.page), DefaultCheckbox)->SetValue(defined);

		// population limit
		defined = player["PopulationLimit"].defined();
		if (defined)
			controls.pop->SetValue(wxString(player["PopulationLimit"]));
		else
			controls.pop->SetValue(0);
		wxDynamicCast(FindWindowById(ID_DefaultPop, controls.page), DefaultCheckbox)->SetValue(defined);

		// team
		defined = player["Team"].defined();
		if (defined)
			controls.team->SetSelection((*player["Team"]).getInt() + 1);
		else
			controls.team->SetSelection(0);
		wxDynamicCast(FindWindowById(ID_DefaultTeam, controls.page), DefaultCheckbox)->SetValue(defined);

		// camera
		if (player["StartingCamera"].defined())
		{
			sCameraInfo info;
			// Don't use wxAtof because it depends on locales which
			//	may cause problems with decimal points
			//	see: http://www.wxwidgets.org/docs/faqgtk.htm#locale
			AtObj camPos = *player["StartingCamera"]["Position"];
			info.pX = (float)(*camPos["x"]).getDouble();
			info.pY = (float)(*camPos["y"]).getDouble();
			info.pZ = (float)(*camPos["z"]).getDouble();
			AtObj camRot = *player["StartingCamera"]["Rotation"];
			info.rX = (float)(*camRot["x"]).getDouble();
			info.rY = (float)(*camRot["y"]).getDouble();
			info.rZ = (float)(*camRot["z"]).getDouble();

			controls.page->SetCamera(info, true);
		}
		else
			controls.page->SetCamera(sCameraInfo(), false);

		// Only increment AtIters if they are defined
		if (player.defined())
			++player;
		if (playerDefs.defined())
			++playerDefs;
	}

	// Send default properties to engine, since they might not be set
	SendToEngine();

	m_InGUIUpdate = false;
}

AtObj PlayerSettingsControl::UpdateSettingsObject()
{
	// Update player data in the map settings
	AtObj players;
	players.set("@array", L"");

	wxASSERT(m_NumPlayers <= MAX_NUM_PLAYERS);

	for (size_t i = 0; i < m_NumPlayers; ++i)
	{
		PlayerPageControls controls = m_PlayerControls[i];

		AtObj player;

		// name
		wxTextCtrl* text = controls.name;
		if (text->IsEnabled())
			player.set("Name", text->GetValue());

		// civ
		wxChoice* choice = controls.civ;
		if (choice->IsEnabled() && choice->GetSelection() >= 0)
		{
			wxStringClientData* str = dynamic_cast<wxStringClientData*>(choice->GetClientObject(choice->GetSelection()));
			player.set("Civ", str->GetData());
		}

		// color
		if (controls.color->IsEnabled())
		{
			wxColor color = controls.color->GetBackgroundColour();
			AtObj clrObj;
			clrObj.setInt("r", (int)color.Red());
			clrObj.setInt("g", (int)color.Green());
			clrObj.setInt("b", (int)color.Blue());
			player.set("Color", clrObj);
		}

		// player type
		choice = controls.ai;
		if (choice->IsEnabled())
		{
			if (choice->GetSelection() > 0)
			{
				// ai - get id
				wxStringClientData* str = dynamic_cast<wxStringClientData*>(choice->GetClientObject(choice->GetSelection()));
				player.set("AI", str->GetData());
			}
			else // human
				player.set("AI", _T(""));
		}

		// resources
		AtObj resObj;
		if (controls.food->IsEnabled())
			resObj.setInt("food", controls.food->GetValue());
		if (controls.wood->IsEnabled())
			resObj.setInt("wood", controls.wood->GetValue());
		if (controls.metal->IsEnabled())
			resObj.setInt("metal", controls.metal->GetValue());
		if (controls.stone->IsEnabled())
			resObj.setInt("stone", controls.stone->GetValue());
		if (resObj.defined())
			player.set("Resources", resObj);

		// population limit
		if (controls.pop->IsEnabled())
			player.setInt("PopulationLimit", controls.pop->GetValue());

		// team
		choice = controls.team;
		if (choice->IsEnabled() && choice->GetSelection() >= 0)
			player.setInt("Team", choice->GetSelection() - 1);

		// camera
		AtObj camObj;
		if (controls.page->IsCameraDefined())
		{
			sCameraInfo cam = controls.page->GetCamera();
			AtObj camPos;
			camPos.setDouble("x", cam.pX);
			camPos.setDouble("y", cam.pY);
			camPos.setDouble("z", cam.pZ);
			camObj.set("Position", camPos);

			AtObj camRot;
			camRot.setDouble("x", cam.rX);
			camRot.setDouble("y", cam.rY);
			camRot.setDouble("z", cam.rZ);
			camObj.set("Rotation", camRot);
		}
		player.set("StartingCamera", camObj);

		players.add("item", player);
	}

	m_MapSettings.set("PlayerData", players);

	return m_MapSettings;
}

void PlayerSettingsControl::SendToEngine()
{
	UpdateSettingsObject();

	std::string json = AtlasObject::SaveToJSON(m_MapSettings);

	// TODO: would be nice if we supported undo for settings changes

	POST_COMMAND(SetMapSettings, (json));
}

//////////////////////////////////////////////////////////////////////////

PlayerSidebar::PlayerSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
	: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer), m_Loaded(false)
{
	wxSizer* scrollSizer = new wxBoxSizer(wxVERTICAL);
	wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this);
	scrolledWindow->SetScrollRate(10, 10);
	scrolledWindow->SetSizer(scrollSizer);
	m_MainSizer->Add(scrolledWindow, wxSizerFlags().Proportion(1).Expand());

	m_PlayerSettingsCtrl = new PlayerSettingsControl(scrolledWindow, m_ScenarioEditor);
	scrollSizer->Add(m_PlayerSettingsCtrl, wxSizerFlags().Expand());
}

void PlayerSidebar::OnCollapse(wxCollapsiblePaneEvent& WXUNUSED(evt))
{
	Freeze();

	// Toggling the collapsing doesn't seem to update the sidebar layout
	// automatically, so do it explicitly here
	Layout();

	Refresh(); // fixes repaint glitch on Windows

	Thaw();
}

void PlayerSidebar::OnFirstDisplay()
{
	// We do this here becase messages are used which requires simulation to be init'd
	m_PlayerSettingsCtrl->LoadDefaults();
	m_PlayerSettingsCtrl->CreateWidgets();
	m_PlayerSettingsCtrl->ReadFromEngine();

	m_Loaded = true;

	Layout();
}

void PlayerSidebar::OnMapReload()
{
	// Make sure we've loaded the controls
	if (m_Loaded)
	{
		m_PlayerSettingsCtrl->ReadFromEngine();
	}
}

BEGIN_EVENT_TABLE(PlayerSidebar, Sidebar)
	EVT_COLLAPSIBLEPANE_CHANGED(wxID_ANY, PlayerSidebar::OnCollapse)
END_EVENT_TABLE();
