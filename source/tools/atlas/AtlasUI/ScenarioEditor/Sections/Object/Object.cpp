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

#include "Object.h"

#include "Buttons/ToolButton.h"
#include "General/Datafile.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "ScenarioEditor/Tools/Common/ObjectSettings.h"
#include "ScenarioEditor/Tools/Common/MiscState.h"
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
	ID_ViewerWater,
	ID_ViewerShadows,
	ID_ViewerPolyCount,
	ID_ViewerAnimation,
	ID_ViewerBoundingBox,
	ID_ViewerAxesMarker,
	ID_ViewerPropPoints,
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
	ObjectBottomBar(
		wxWindow* parent,
		Observable<ObjectSettings>& objectSettings,
		Observable<AtObj>& mapSettings,
		ObjectSidebarImpl* p
	);

	void OnFirstDisplay();
	void ShowActorViewer(bool show);
	void OnSelectedObjectsChange(const std::vector<AtlasMessage::ObjectID>& selectedObjects);

private:
	void OnViewerSetting(wxCommandEvent& evt);
	void OnSelectAnim(wxCommandEvent& evt);
	void OnSpeed(wxCommandEvent& evt);

	bool m_ViewerWireframe;
	bool m_ViewerMove;
	bool m_ViewerGround;
	bool m_ViewerWater;
	bool m_ViewerShadows;
	bool m_ViewerPolyCount;
	bool m_ViewerBoundingBox;
	bool m_ViewerAxesMarker;
	int m_ViewerPropPointsMode; // 0 disabled, 1 for point markers, 2 for point markers + axes

	wxPanel* m_ViewerPanel;
	wxScrolledWindow* m_TemplateNames;

	ObjectSidebarImpl* p;
	DECLARE_EVENT_TABLE();
};

struct ObjectSidebarImpl
{
	ObjectSidebarImpl(ScenarioEditor& scenarioEditor) :
		m_ObjectListBox(NULL), m_ActorViewerActive(false),
		m_ActorViewerEntity(_T("actor|structures/fndn_1x1.xml")),
		m_ActorViewerAnimation(_T("idle")), m_ActorViewerSpeed(0.f),
		m_ObjectSettings(scenarioEditor.GetObjectSettings())
	{
	}

	wxListBox* m_ObjectListBox;
	std::vector<AtlasMessage::sObjectsListItem> m_Objects;
	ObservableScopedConnection m_ToolConn;

	bool m_ActorViewerActive;
	wxString m_ActorViewerEntity;
	wxString m_ActorViewerAnimation;
	float m_ActorViewerSpeed;
	Observable<ObjectSettings>& m_ObjectSettings;

	void ActorViewerPostToGame()
	{
		POST_MESSAGE(SetActorViewer, ((std::wstring)m_ActorViewerEntity.wc_str(), (std::wstring)m_ActorViewerAnimation.wc_str(), m_ObjectSettings.GetPlayerID(), m_ActorViewerSpeed, false));
	}
};

ObjectSidebar::ObjectSidebar(
	ScenarioEditor& scenarioEditor,
	wxWindow* sidebarContainer,
	wxWindow* bottomBarContainer
)
	: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer),
	m_Impl(new ObjectSidebarImpl(scenarioEditor))
{
	wxSizer* scrollSizer = new wxBoxSizer(wxVERTICAL);
	wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this);
	scrolledWindow->SetScrollRate(10, 10);
	scrolledWindow->SetSizer(scrollSizer);
	m_MainSizer->Add(scrolledWindow, wxSizerFlags().Proportion(1).Expand());

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(new wxStaticText(scrolledWindow, wxID_ANY, _("Filter")), wxSizerFlags().Align(wxALIGN_CENTER));
	sizer->Add(
		Tooltipped(
			new wxTextCtrl(scrolledWindow, ID_ObjectFilter),
			_("Enter text to filter object list")
		),
		wxSizerFlags().Expand().Proportion(1)
	);
	scrollSizer->Add(sizer, wxSizerFlags().Expand());
	scrollSizer->AddSpacer(3);

	// ------------------------------------------------------------------------------------------

	wxArrayString strings;
	strings.Add(_("Entities"));
	strings.Add(_("Actors (all)"));
	wxChoice* objectType = new wxChoice(scrolledWindow, ID_ObjectType, wxDefaultPosition, wxDefaultSize, strings);
	objectType->SetSelection(0);
	scrollSizer->Add(objectType, wxSizerFlags().Expand());
	scrollSizer->AddSpacer(3);

	// ------------------------------------------------------------------------------------------

	m_Impl->m_ObjectListBox = new wxListBox(scrolledWindow, ID_SelectObject, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE|wxLB_HSCROLL);
	scrollSizer->Add(m_Impl->m_ObjectListBox, wxSizerFlags().Proportion(1).Expand());
	scrollSizer->AddSpacer(3);

	// ------------------------------------------------------------------------------------------

	scrollSizer->Add(new wxButton(scrolledWindow, ID_ToggleViewer, _("Switch to Actor Viewer")), wxSizerFlags().Expand());

	// ------------------------------------------------------------------------------------------

	m_BottomBar = new ObjectBottomBar(
		bottomBarContainer,
		scenarioEditor.GetObjectSettings(),
		scenarioEditor.GetMapSettings(),
		m_Impl
	);

	m_Impl->m_ToolConn = scenarioEditor.GetToolManager().GetCurrentTool().RegisterObserver(0, &ObjectSidebar::OnToolChange, this);
}

ObjectSidebar::~ObjectSidebar()
{
	delete m_Impl;
}

void ObjectSidebar::OnToolChange(ITool* tool)
{
	if (wxString(tool->GetClassInfo()->GetClassName()) == _T("ActorViewerTool"))
	{
		m_Impl->m_ActorViewerActive = true;
		m_Impl->ActorViewerPostToGame();
		wxDynamicCast(FindWindow(ID_ToggleViewer), wxButton)->SetLabel(_("Return to game view"));
	}
	else
	{
		m_Impl->m_ActorViewerActive = false;
		wxDynamicCast(FindWindow(ID_ToggleViewer), wxButton)->SetLabel(_("Switch to Actor Viewer"));
	}

	static_cast<ObjectBottomBar*>(m_BottomBar)->ShowActorViewer(m_Impl->m_ActorViewerActive);
}

void ObjectSidebar::OnFirstDisplay()
{
	static_cast<ObjectBottomBar*>(m_BottomBar)->OnFirstDisplay();

	wxBusyInfo busy (_("Loading list of objects"));

	// Get the list of objects from the game
	AtlasMessage::qGetObjectsList qry;
	qry.Post();
	m_Impl->m_Objects = *qry.objects;
	// Display first group of objects
	FilterObjects();
}

void ObjectSidebar::FilterObjects()
{
	int filterType = wxDynamicCast(FindWindow(ID_ObjectType), wxChoice)->GetSelection();
	wxString filterName = wxDynamicCast(FindWindow(ID_ObjectFilter), wxTextCtrl)->GetValue();

	m_Impl->m_ObjectListBox->Freeze();
	m_Impl->m_ObjectListBox->Clear();
	for (std::vector<AtlasMessage::sObjectsListItem>::iterator it = m_Impl->m_Objects.begin(); it != m_Impl->m_Objects.end(); ++it)
	{
		if (it->type == filterType)
		{
			wxString id = it->id.c_str();
			wxString name = it->name.c_str();

			if (name.Lower().Find(filterName.Lower()) != wxNOT_FOUND)
			{
				m_Impl->m_ObjectListBox->Append(name, new wxStringClientData(id));
			}
		}
	}
	m_Impl->m_ObjectListBox->Thaw();
}

void ObjectSidebar::OnToggleViewer(wxCommandEvent& WXUNUSED(evt))
{
	if (m_Impl->m_ActorViewerActive)
	{
		m_ScenarioEditor.GetToolManager().SetCurrentTool(_T(""), NULL);
	}
	else
	{
		m_ScenarioEditor.GetToolManager().SetCurrentTool(_T("ActorViewerTool"), NULL);
	}
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
	m_Impl->m_ActorViewerEntity = id;

	if (m_Impl->m_ActorViewerActive)
	{
		m_Impl->ActorViewerPostToGame();
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
	EVT_BUTTON(ID_ToggleViewer, ObjectSidebar::OnToggleViewer)
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

	void SetSelection(int playerID)
	{
		// This control may not be loaded yet (before first display)
		//	or may have less items than we expect, which could cause
		//	an assertion failure, so handle that here
		if ((unsigned int)playerID < GetCount())
		{
			wxComboBox::SetSelection(playerID);
		}
		else
		{
			// Invalid selection
			wxComboBox::SetSelection(wxNOT_FOUND);
		}
	}

	void OnObjectSettingsChange(const ObjectSettings& settings)
	{
		SetSelection(settings.GetPlayerID());
	}

	void OnMapSettingsChange(const AtObj& settings)
	{
		// Reload displayed player names
		Clear();
		size_t numPlayers = settings["PlayerData"]["item"].count();
		for (size_t i = 0; i <= numPlayers && i < m_Players.Count(); ++i)
		{
			Append(m_Players[i]);
		}

		SetSelection(m_ObjectSettings.GetPlayerID());
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

ObjectBottomBar::ObjectBottomBar(
	wxWindow* parent,
	Observable<ObjectSettings>& objectSettings,
	Observable<AtObj>& mapSettings,
	ObjectSidebarImpl* p
)
	: wxPanel(parent, wxID_ANY), p(p)
{
	m_ViewerWireframe = false;
	m_ViewerMove = false;
	m_ViewerGround = true;
	m_ViewerWater = false;
	m_ViewerShadows = true;
	m_ViewerPolyCount = false;
	m_ViewerBoundingBox = false;
	m_ViewerAxesMarker = false;
	m_ViewerPropPointsMode = 0;

	wxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

	// --- viewer options panel -------------------------------------------------------------------------------

	m_ViewerPanel = new wxPanel(this, wxID_ANY);
	wxSizer* viewerSizer = new wxBoxSizer(wxHORIZONTAL);

	wxSizer* viewerButtonsSizer = new wxStaticBoxSizer(wxHORIZONTAL, m_ViewerPanel, _("Display settings"));
	{
		wxSizer* viewerButtonsLeft = new wxBoxSizer(wxVERTICAL);
		viewerButtonsLeft->SetMinSize(110, -1);
		viewerButtonsLeft->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerWireframe,   _("Wireframe")),      _("Toggle wireframe / solid rendering")), wxSizerFlags().Expand());
		viewerButtonsLeft->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerMove,        _("Move")),           _("Toggle movement along ground when playing walk/run animations")), wxSizerFlags().Expand());
		viewerButtonsLeft->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerGround,      _("Ground")),         _("Toggle the ground plane")), wxSizerFlags().Expand());
		// TODO: disabled until http://trac.wildfiregames.com/ticket/2692 is fixed
		wxButton* waterButton = new wxButton(m_ViewerPanel, ID_ViewerWater, _("Water"));
		waterButton->Enable(false);
		viewerButtonsLeft->Add(Tooltipped(waterButton, _("Toggle the water plane")), wxSizerFlags().Expand());
		viewerButtonsLeft->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerShadows,     _("Shadows")),        _("Toggle shadow rendering")), wxSizerFlags().Expand());
		viewerButtonsLeft->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerPolyCount,   _("Poly count")),     _("Toggle polygon-count statistics - turn off ground and shadows for more useful data")), wxSizerFlags().Expand());

		wxSizer* viewerButtonsRight = new wxBoxSizer(wxVERTICAL);
		viewerButtonsRight->SetMinSize(110,-1);
		viewerButtonsRight->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerBoundingBox, _("Bounding Boxes")), _("Toggle bounding boxes")), wxSizerFlags().Expand());
		viewerButtonsRight->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerAxesMarker,  _("Axes Marker")), _("Toggle the axes marker (R=X, G=Y, B=Z)")), wxSizerFlags().Expand());
		viewerButtonsRight->Add(Tooltipped(new wxButton(m_ViewerPanel, ID_ViewerPropPoints,  _("Prop Points")), _("Toggle prop points (works best in wireframe mode)")), wxSizerFlags().Expand());

		viewerButtonsSizer->Add(viewerButtonsLeft, wxSizerFlags().Expand());
		viewerButtonsSizer->Add(viewerButtonsRight, wxSizerFlags().Expand());
	}

	viewerSizer->Add(viewerButtonsSizer, wxSizerFlags().Expand());
	viewerSizer->AddSpacer(3);

	// --- animations panel -------------------------------------------------------------------------------

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

	// --- add viewer-specific options -------------------------------------------------------------------------------

	m_ViewerPanel->SetSizer(viewerSizer);
	mainSizer->Add(m_ViewerPanel, wxSizerFlags().Expand());

	m_ViewerPanel->Layout(); // prevents strange visibility glitch of the animation buttons on my machine (Vista 32-bit SP1) -- vtsj
	m_ViewerPanel->Show(false);

	// --- add player/variation selection -------------------------------------------------------------------------------

	wxSizer* playerSelectionSizer = new wxBoxSizer(wxHORIZONTAL);
	wxSizer* playerVariationSizer = new wxBoxSizer(wxVERTICAL);

	// TODO: make this a wxChoice instead
	wxComboBox* playerSelect = new PlayerComboBox(this, objectSettings, mapSettings);
	playerSelectionSizer->Add(new wxStaticText(this, wxID_ANY, _("Player:")), wxSizerFlags().Align(wxALIGN_CENTER));
	playerSelectionSizer->AddSpacer(3);
	playerSelectionSizer->Add(playerSelect);

	playerVariationSizer->Add(playerSelectionSizer);
	playerVariationSizer->AddSpacer(3);


	wxWindow* variationSelect = new VariationControl(this, objectSettings);
	variationSelect->SetMinSize(wxSize(160, -1));
	wxSizer* variationSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Variation"));
	variationSizer->Add(variationSelect, wxSizerFlags().Proportion(1).Expand());
	playerVariationSizer->Add(variationSizer, wxSizerFlags().Proportion(1));

	mainSizer->AddSpacer(3);
	mainSizer->Add(playerVariationSizer, wxSizerFlags().Expand());

	// ----------------------------------------------------------------------------------
	// --- display template name
	wxSizer* displaySizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Selected entities"));
	m_TemplateNames = new wxScrolledWindow(this);
	m_TemplateNames->SetMinSize(wxSize(250, -1));
	m_TemplateNames->SetScrollRate(0, 5);
	wxSizer* scrollwindowSizer = new wxBoxSizer(wxVERTICAL);
	m_TemplateNames->SetSizer(scrollwindowSizer);
	displaySizer->Add(m_TemplateNames, wxSizerFlags().Proportion(1).Expand());
	m_TemplateNames->Layout();
	mainSizer->AddSpacer(3);
	mainSizer->Add(displaySizer, wxSizerFlags().Proportion(1).Expand());

	g_SelectedObjects.RegisterObserver(0, &ObjectBottomBar::OnSelectedObjectsChange, this);

	SetSizer(mainSizer);
}

static wxControl* CreateTemplateNameObject(wxWindow* parent, const std::string& templateName, int counterTemplate)
{
	wxString idTemplate(wxString::FromUTF8(templateName.c_str()));
	if (counterTemplate > 1)
		idTemplate.Append(wxString::Format(wxT(" (%i)"), counterTemplate));

	wxStaticText* templateNameObject = new wxStaticText(parent, wxID_ANY, idTemplate);
	return templateNameObject;
}

void ObjectBottomBar::OnSelectedObjectsChange(const std::vector<AtlasMessage::ObjectID>& selectedObjects)
{
	Freeze();
	wxSizer* sizer = m_TemplateNames->GetSizer();
	sizer->Clear(true);

	AtlasMessage::qGetSelectedObjectsTemplateNames objectTemplatesName(selectedObjects);
	objectTemplatesName.Post();
	std::vector<std::string> names = *objectTemplatesName.names;

	int counterTemplate = 0;
	std::string lastTemplateName = "";
	for (std::vector<std::string>::const_iterator it = names.begin(); it != names.end(); ++it)
	{
		if (lastTemplateName == "")
			lastTemplateName = (*it);

		if (lastTemplateName == (*it))
		{
			++counterTemplate;
			continue;
		}

		sizer->Add(CreateTemplateNameObject(m_TemplateNames, lastTemplateName, counterTemplate), wxSizerFlags().Align(wxALIGN_LEFT));

		lastTemplateName = (*it);
		counterTemplate = 1;
	}
	// Add the remaining template
	sizer->Add(CreateTemplateNameObject(m_TemplateNames, lastTemplateName, counterTemplate), wxSizerFlags().Align(wxALIGN_LEFT));

	Thaw();
	sizer->FitInside(m_TemplateNames);
}

void ObjectBottomBar::OnFirstDisplay()
{
	// We use messages here because the simulation is not init'd otherwise (causing a crash)

	// Get player names
	wxArrayString players;
	AtlasMessage::qGetPlayerDefaults qryPlayers;
	qryPlayers.Post();
	AtObj playerData = AtlasObject::LoadFromJSON(*qryPlayers.defaults);
	AtObj playerDefs = *playerData["PlayerData"];
	for (AtIter iterator = playerDefs["item"]; iterator.defined(); ++iterator)
		players.Add(wxString(iterator["Name"]));

	wxDynamicCast(FindWindow(ID_PlayerSelect), PlayerComboBox)->SetPlayers(players);

	// Initialise the game with the default settings
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"wireframe", m_ViewerWireframe));
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"walk", m_ViewerMove));
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"ground", m_ViewerGround));
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"water", m_ViewerWater));
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"shadows", m_ViewerShadows));
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"stats", m_ViewerPolyCount));
	POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"bounding_box", m_ViewerBoundingBox));
	POST_MESSAGE(SetViewParamI, (AtlasMessage::eRenderView::ACTOR, L"prop_points", m_ViewerPropPointsMode));
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
	case ID_ViewerWater:
		m_ViewerWater = !m_ViewerWater;
		POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"water", m_ViewerWater));
		break;
	case ID_ViewerShadows:
		m_ViewerShadows = !m_ViewerShadows;
		POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"shadows", m_ViewerShadows));
		break;
	case ID_ViewerPolyCount:
		m_ViewerPolyCount = !m_ViewerPolyCount;
		POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"stats", m_ViewerPolyCount));
		break;
	case ID_ViewerBoundingBox:
		m_ViewerBoundingBox = !m_ViewerBoundingBox;
		POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"bounding_box", m_ViewerBoundingBox));
		break;
	case ID_ViewerAxesMarker:
		m_ViewerAxesMarker = !m_ViewerAxesMarker;
		POST_MESSAGE(SetViewParamB, (AtlasMessage::eRenderView::ACTOR, L"axes_marker", m_ViewerAxesMarker));
		break;
	case ID_ViewerPropPoints:
		m_ViewerPropPointsMode = (m_ViewerPropPointsMode+1) % 3;
		POST_MESSAGE(SetViewParamI, (AtlasMessage::eRenderView::ACTOR, L"prop_points", m_ViewerPropPointsMode));
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
	EVT_BUTTON(ID_ViewerWater, ObjectBottomBar::OnViewerSetting)
	EVT_BUTTON(ID_ViewerShadows, ObjectBottomBar::OnViewerSetting)
	EVT_BUTTON(ID_ViewerPolyCount, ObjectBottomBar::OnViewerSetting)
	EVT_CHOICE(ID_ViewerAnimation, ObjectBottomBar::OnSelectAnim)
	EVT_BUTTON(ID_ViewerPlay, ObjectBottomBar::OnSpeed)
	EVT_BUTTON(ID_ViewerPause, ObjectBottomBar::OnSpeed)
	EVT_BUTTON(ID_ViewerSlow, ObjectBottomBar::OnSpeed)
	EVT_BUTTON(ID_ViewerBoundingBox, ObjectBottomBar::OnViewerSetting)
	EVT_BUTTON(ID_ViewerAxesMarker, ObjectBottomBar::OnViewerSetting)
	EVT_BUTTON(ID_ViewerPropPoints, ObjectBottomBar::OnViewerSetting)
END_EVENT_TABLE();
