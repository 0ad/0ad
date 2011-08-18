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

#include "Object.h"

#include "Buttons/ToolButton.h"
#include "General/Datafile.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "ScenarioEditor/Tools/Common/ObjectSettings.h"
#include "ScenarioEditor/Tools/Common/MiscState.h"
#include "AtlasScript/ScriptInterface.h"
#include "VariationControl.h"

#include "GameInterface/Messages.h"

#include "wx/busyinfo.h"

enum
{
	ID_ObjectType = 1,
	ID_ObjectFilter,
	ID_PlayerSelect,
	ID_SelectObject,
	ID_ToggleViewer,
	ID_ViewerWireframe,
	ID_ViewerMove,
	ID_ViewerGround,
	ID_ViewerShadows,
	ID_ViewerPolyCount,
	ID_ViewerAnimation,
	ID_ViewerPlay,
	ID_ViewerPause,
	ID_ViewerSlow
};

// Helper function for adding tooltips
static wxWindow* Tooltipped(wxWindow* window, const wxString& tip)
{
	window->SetToolTip(tip);
	return window;
}

class ObjectBottomBar : public wxPanel
{
public:
	ObjectBottomBar(wxWindow* parent, ScenarioEditor& scenarioEditor, Observable<ObjectSettings>& objectSettings, Observable<AtObj>& mapSettings, ObjectSidebarImpl* p);

	void OnFirstDisplay();

	void ShowActorViewer(bool show);

private:
	void OnViewerSetting(wxCommandEvent& evt);
	void OnSelectAnim(wxCommandEvent& evt);
	void OnSpeed(wxCommandEvent& evt);

	bool m_ViewerWireframe;
	bool m_ViewerMove;
	bool m_ViewerGround;
	bool m_ViewerShadows;
	bool m_ViewerPolyCount;

	wxPanel* m_ViewerPanel;

	ObjectSidebarImpl* p;

	ScenarioEditor& m_ScenarioEditor;

	DECLARE_EVENT_TABLE();
};

struct ObjectSidebarImpl
{
	ObjectSidebarImpl() :
		m_ObjectListBox(NULL), m_ActorViewerActive(false),
		m_ActorViewerEntity(_T("actor|structures/fndn_1x1.xml")), m_ActorViewerAnimation(_T("idle")), m_ActorViewerSpeed(0.f)
	{
	}

	wxListBox* m_ObjectListBox;
	std::vector<AtlasMessage::sObjectsListItem> m_Objects;
	ObservableScopedConnection m_ToolConn;

	bool m_ActorViewerActive;
	wxString m_ActorViewerEntity;
	wxString m_ActorViewerAnimation;
	float m_ActorViewerSpeed;

	void ActorViewerPostToGame()
	{
		POST_MESSAGE(SetActorViewer, ((std::wstring)m_ActorViewerEntity.wc_str(), (std::wstring)m_ActorViewerAnimation.wc_str(), m_ActorViewerSpeed, false));
	}
};

ObjectSidebar::ObjectSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer), p(new ObjectSidebarImpl())
{
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(new wxStaticText(this, wxID_ANY, _("Filter")), wxSizerFlags().Align(wxALIGN_CENTER));
	wxTextCtrl* objectFilter = new wxTextCtrl(this, ID_ObjectFilter);
	sizer->Add(objectFilter, wxSizerFlags().Expand().Proportion(1));
	m_MainSizer->Add(sizer, wxSizerFlags().Expand());

	wxArrayString strings;
	strings.Add(_("Entities"));
	strings.Add(_("Actors (all)"));
	wxChoice* objectType = new wxChoice(this, ID_ObjectType, wxDefaultPosition, wxDefaultSize, strings);
	objectType->SetSelection(0);
	m_MainSizer->Add(objectType, wxSizerFlags().Expand());

	p->m_ObjectListBox = new wxListBox(this, ID_SelectObject, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_HSCROLL);
	m_MainSizer->Add(p->m_ObjectListBox, wxSizerFlags().Proportion(1).Expand());

	m_MainSizer->Add(new wxButton(this, ID_ToggleViewer, _("Switch to Actor Viewer")), wxSizerFlags().Expand());

	m_BottomBar = new ObjectBottomBar(bottomBarContainer, scenarioEditor, scenarioEditor.GetObjectSettings(), scenarioEditor.GetMapSettings(), p);

	p->m_ToolConn = scenarioEditor.GetToolManager().GetCurrentTool().RegisterObserver(0, &ObjectSidebar::OnToolChange, this);
}

ObjectSidebar::~ObjectSidebar()
{
	delete p;
}

void ObjectSidebar::OnToolChange(ITool* tool)
{
	if (wxString(tool->GetClassInfo()->GetClassName()) == _T("ActorViewerTool"))
	{
		p->m_ActorViewerActive = true;
		p->ActorViewerPostToGame();
	}
	else
	{
		p->m_ActorViewerActive = false;
	}

	static_cast<ObjectBottomBar*>(m_BottomBar)->ShowActorViewer(p->m_ActorViewerActive);
}

void ObjectSidebar::OnFirstDisplay()
{
	static_cast<ObjectBottomBar*>(m_BottomBar)->OnFirstDisplay();

	wxBusyInfo busy (_("Loading list of objects"));

	// Get the list of objects from the game
	AtlasMessage::qGetObjectsList qry;
	qry.Post();
	p->m_Objects = *qry.objects;
	// Display first group of objects
	FilterObjects();
}

void ObjectSidebar::FilterObjects()
{
	int filterType = wxDynamicCast(FindWindow(ID_ObjectType), wxChoice)->GetSelection();
	wxString filterName = wxDynamicCast(FindWindow(ID_ObjectFilter), wxTextCtrl)->GetValue();

	p->m_ObjectListBox->Freeze();
	p->m_ObjectListBox->Clear();
	for (std::vector<AtlasMessage::sObjectsListItem>::iterator it = p->m_Objects.begin(); it != p->m_Objects.end(); ++it)
	{
		if (it->type == filterType)
		{
			wxString id = it->id.c_str();
			wxString name = it->name.c_str();

			if (name.Lower().Find(filterName.Lower()) != wxNOT_FOUND)
			{
				p->m_ObjectListBox->Append(name, new wxStringClientData(id));
			}
		}
	}
	p->m_ObjectListBox->Thaw();
}

void ObjectSidebar::ToggleViewer(wxCommandEvent& WXUNUSED(evt))
{
	if (p->m_ActorViewerActive)
		m_ScenarioEditor.GetToolManager().SetCurrentTool(_T(""), NULL);
	else
		m_ScenarioEditor.GetToolManager().SetCurrentTool(_T("ActorViewerTool"), NULL);
}

void ObjectSidebar::OnSelectType(wxCommandEvent& WXUNUSED(evt))
{
	FilterObjects();
}

void ObjectSidebar::OnSelectObject(wxCommandEvent& evt)
{
	if (evt.GetInt() < 0)
		return;

	wxString id = static_cast<wxStringClientData*>(evt.GetClientObject())->GetData();

	// Always update the actor viewer's state even if it's inactive,
	// so it will be correct when first enabled
	p->m_ActorViewerEntity = id;

	if (p->m_ActorViewerActive)
	{
		p->ActorViewerPostToGame();
	}
	else
	{
		// On selecting an object, enable the PlaceObject tool with this object
		m_ScenarioEditor.GetToolManager().SetCurrentTool(_T("PlaceObject"), &id);
	}
}

void ObjectSidebar::OnSelectFilter(wxCommandEvent& WXUNUSED(evt))
{
	FilterObjects();
}

BEGIN_EVENT_TABLE(ObjectSidebar, Sidebar)
	EVT_CHOICE(ID_ObjectType, ObjectSidebar::OnSelectType)
	EVT_TEXT(ID_ObjectFilter, ObjectSidebar::OnSelectFilter)
	EVT_LISTBOX(ID_SelectObject, ObjectSidebar::OnSelectObject)
	EVT_BUTTON(ID_ToggleViewer, ObjectSidebar::ToggleViewer)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////

class PlayerComboBox : public wxComboBox
{
public:
	PlayerComboBox(wxWindow* parent, Observable<ObjectSettings>& objectSettings, Observable<AtObj>& mapSettings)
		: wxComboBox(parent, ID_PlayerSelect, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, 0, wxCB_READONLY)
		, m_ObjectSettings(objectSettings), m_MapSettings(mapSettings)
	{
		m_ObjectConn = m_ObjectSettings.RegisterObserver(1, &PlayerComboBox::OnObjectSettingsChange, this);
		m_MapConn = m_MapSettings.RegisterObserver(1, &PlayerComboBox::OnMapSettingsChange, this);
	}

	void SetPlayers(wxArrayString& names)
	{
		m_Players = names;
		OnMapSettingsChange(m_MapSettings);
	}

private:

	ObservableScopedConnection m_ObjectConn;
	Observable<ObjectSettings>& m_ObjectSettings;
	ObservableScopedConnection m_MapConn;
	Observable<AtObj>& m_MapSettings;
	wxArrayString m_Players;

	void OnObjectSettingsChange(const ObjectSettings& settings)
	{
		SetSelection((long)settings.GetPlayerID());
	}

	void OnMapSettingsChange(const AtObj& settings)
	{
		// Adjust displayed number of players
		Clear();
		size_t numPlayers = settings["PlayerData"]["item"].count();
		for (size_t i = 0; i <= numPlayers && i < m_Players.Count(); ++i)
		{
			Append(m_Players[i]);
		}

		if (m_ObjectSettings.GetPlayerID() > numPlayers)
		{
			// Invalid player
			SetSelection((long)numPlayers);
		}
		else
		{
			SetSelection((long)m_ObjectSettings.GetPlayerID());
		}
	}

	void OnSelect(wxCommandEvent& evt)
	{
		m_ObjectSettings.SetPlayerID(evt.GetInt());
		m_ObjectSettings.NotifyObserversExcept(m_ObjectConn);
	}

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(PlayerComboBox, wxComboBox)
	EVT_COMBOBOX(wxID_ANY, PlayerComboBox::OnSelect)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////

ObjectBottomBar::ObjectBottomBar(wxWindow* parent, ScenarioEditor& scenarioEditor,  Observable<ObjectSettings>& objectSettings, Observable<AtObj>& mapSettings, ObjectSidebarImpl* p)
	: wxPanel(parent, wxID_ANY), p(p), m_ScenarioEditor(scenarioEditor)
{
	m_ViewerWireframe = false;
	m_ViewerMove = false;
	m_ViewerGround = true;
	m_ViewerShadows = true;
	m_ViewerPolyCount = false;

	wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);


	m_ViewerPanel = new wxPanel(this, wxID_ANY);
	wxSizer* viewerSizer = new wxBoxSizer(wxHORIZONTAL);

	wxSizer* viewerButtonsSizer = new wxStaticBoxSizer(wxVERTICAL, m_ViewerPanel, _("Display settings"));
	viewerButtonsSizer->SetMinSize(140, -1);
	viewerButtonsSizer->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerWireframe, _("Wireframe")), _("Toggle wireframe / solid rendering")), wxSizerFlags().Expand());
	viewerButtonsSizer->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerMove, _("Move")), _("Toggle movement along ground when playing walk/run animations")), wxSizerFlags().Expand());
	viewerButtonsSizer->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerGround, _("Ground")), _("Toggle the ground plane")), wxSizerFlags().Expand());
	viewerButtonsSizer->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerShadows, _("Shadows")), _("Toggle shadow rendering")), wxSizerFlags().Expand());
	viewerButtonsSizer->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerPolyCount, _("Poly count")), _("Toggle polygon-count statistics - turn off ground and shadows for more useful data")), wxSizerFlags().Expand());
	viewerSizer->Add(viewerButtonsSizer, wxSizerFlags().Expand());

	wxSizer* viewerAnimSizer = new wxStaticBoxSizer(wxVERTICAL, m_ViewerPanel, _("Animation"));

	// TODO: this list should come from the actor
	wxArrayString animChoices;
	AtObj anims (Datafile::ReadList("animations"));
	for (AtIter a = anims["item"]; a.defined(); ++a)
	{
		animChoices.Add(wxString(*a));
	}
	wxChoice* viewerAnimSelector = new wxChoice(m_ViewerPanel, ID_ViewerAnimation, wxDefaultPosition, wxDefaultSize, animChoices);
	viewerAnimSelector->SetSelection(0);
	viewerAnimSizer->Add(viewerAnimSelector, wxSizerFlags().Expand());

	wxSizer* viewerAnimSpeedSizer = new wxBoxSizer(wxHORIZONTAL);
	viewerAnimSpeedSizer->Add(new wxButton(m_ViewerPanel, ID_ViewerPlay, _("Play"), wxDefaultPosition, wxSize(50, -1)), wxSizerFlags().Expand());
	viewerAnimSpeedSizer->Add(new wxButton(m_ViewerPanel, ID_ViewerPause, _("Pause"), wxDefaultPosition, wxSize(50, -1)), wxSizerFlags().Expand());
	viewerAnimSpeedSizer->Add(new wxButton(m_ViewerPanel, ID_ViewerSlow, _("Slow"), wxDefaultPosition, wxSize(50, -1)), wxSizerFlags().Expand());
	viewerAnimSizer->Add(viewerAnimSpeedSizer);

	viewerSizer->Add(viewerAnimSizer, wxSizerFlags().Expand());

	m_ViewerPanel->SetSizer(viewerSizer);
	sizer->Add(m_ViewerPanel, wxSizerFlags().Expand());

	m_ViewerPanel->Show(false);


	wxSizer* playerVariationSizer = new wxBoxSizer(wxVERTICAL);

	// TODO: make this a wxChoice instead
	wxComboBox* playerSelect = new PlayerComboBox(this, objectSettings, mapSettings);
	playerVariationSizer->Add(playerSelect);

	wxWindow* variationSelect = new VariationControl(this, objectSettings);
	variationSelect->SetMinSize(wxSize(160, -1));
	wxSizer* variationSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Variation"));
	variationSizer->Add(variationSelect, wxSizerFlags().Proportion(1).Expand());
	playerVariationSizer->Add(variationSizer, wxSizerFlags().Proportion(1));

	sizer->Add(playerVariationSizer, wxSizerFlags().Expand());

	SetSizer(sizer);
}

void ObjectBottomBar::OnFirstDisplay()
{
	// We use messages here because the simulation is not init'd otherwise (causing a crash)

	// Get player names
	wxArrayString players;
	AtlasMessage::qGetPlayerDefaults qryPlayers;
	qryPlayers.Post();
	AtObj playerData = AtlasObject::LoadFromJSON(m_ScenarioEditor.GetScriptInterface().GetContext(), *qryPlayers.defaults);
	AtObj playerDefs = *playerData["PlayerData"];
	for (AtIter p = playerDefs["item"]; p.defined(); ++p)
	{
		players.Add(wxString(p["Name"]));
	}
	wxDynamicCast(FindWindow(ID_PlayerSelect), PlayerComboBox)->SetPlayers(players);

	// Initialise the game with the default settings
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"wireframe", m_ViewerWireframe));
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"walk", m_ViewerMove));
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"ground", m_ViewerGround));
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"shadows", m_ViewerShadows));
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"stats", m_ViewerPolyCount));
}

void ObjectBottomBar::ShowActorViewer(bool show)
{
	m_ViewerPanel->Show(show);
	Layout();
}

void ObjectBottomBar::OnViewerSetting(wxCommandEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_ViewerWireframe:
		m_ViewerWireframe = !m_ViewerWireframe;
		POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"wireframe", m_ViewerWireframe));
		break;
	case ID_ViewerMove:
		m_ViewerMove = !m_ViewerMove;
		POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"walk", m_ViewerMove));
		break;
	case ID_ViewerGround:
		m_ViewerGround = !m_ViewerGround;
		POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"ground", m_ViewerGround));
		break;
	case ID_ViewerShadows:
		m_ViewerShadows = !m_ViewerShadows;
		POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"shadows", m_ViewerShadows));
		break;
	case ID_ViewerPolyCount:
		m_ViewerPolyCount = !m_ViewerPolyCount;
		POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"stats", m_ViewerPolyCount));
		break;
	}
}

void ObjectBottomBar::OnSelectAnim(wxCommandEvent& evt)
{
	p->m_ActorViewerAnimation = evt.GetString();
	p->ActorViewerPostToGame();
}

void ObjectBottomBar::OnSpeed(wxCommandEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_ViewerPlay: p->m_ActorViewerSpeed = 1.0f; break;
	case ID_ViewerPause: p->m_ActorViewerSpeed = 0.0f; break;
	case ID_ViewerSlow: p->m_ActorViewerSpeed = 0.1f; break;
	}
	p->ActorViewerPostToGame();
}

BEGIN_EVENT_TABLE(ObjectBottomBar, wxPanel)
	EVT_BUTTON(ID_ViewerWireframe, ObjectBottomBar::OnViewerSetting)
	EVT_BUTTON(ID_ViewerMove, ObjectBottomBar::OnViewerSetting)
	EVT_BUTTON(ID_ViewerGround, ObjectBottomBar::OnViewerSetting)
	EVT_BUTTON(ID_ViewerShadows, ObjectBottomBar::OnViewerSetting)
	EVT_BUTTON(ID_ViewerPolyCount, ObjectBottomBar::OnViewerSetting)
	EVT_CHOICE(ID_ViewerAnimation, ObjectBottomBar::OnSelectAnim)
	EVT_BUTTON(ID_ViewerPlay, ObjectBottomBar::OnSpeed)
	EVT_BUTTON(ID_ViewerPause, ObjectBottomBar::OnSpeed)
	EVT_BUTTON(ID_ViewerSlow, ObjectBottomBar::OnSpeed)
END_EVENT_TABLE();
