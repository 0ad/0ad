/* Copyright (C) 2009 Wildfire Games.
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
#include "ScenarioEditor/ScenarioEditor.h"
#include "ScenarioEditor/Tools/Common/ObjectSettings.h"
#include "ScenarioEditor/Tools/Common/MiscState.h"
#include "VariationControl.h"

#include "GameInterface/Messages.h"

#include "wx/busyinfo.h"

class ObjectSelectListBox : public wxListBox
{
public:
	ObjectSelectListBox(wxWindow* parent, ToolManager& toolManager)
		: wxListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_HSCROLL)
		, m_ToolManager(toolManager)
	{
	}

	void OnSelect(wxCommandEvent& evt)
	{
		// On selecting an object, enable the PlaceObject tool with this object
		wxString id = static_cast<wxStringClientData*>(evt.GetClientObject())->GetData();
		m_ToolManager.SetCurrentTool(_T("PlaceObject"), &id);
	}

private:
	ToolManager& m_ToolManager;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(ObjectSelectListBox, wxListBox)
	EVT_LISTBOX(wxID_ANY, ObjectSelectListBox::OnSelect)
END_EVENT_TABLE();


class ObjectChoiceCtrl : public wxChoice
{
public:
	ObjectChoiceCtrl(wxWindow* parent, const wxArrayString& strings, ObjectSidebar& sidebar)
		: wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, strings),
		m_Sidebar(sidebar)
	{
		SetSelection(0);
	}

	void OnSelect(wxCommandEvent& evt)
	{
		// Switch between displayed lists of objects (e.g. entities vs actors)
		m_Sidebar.SetObjectFilter(evt.GetSelection());
	}

private:
	ObjectSidebar& m_Sidebar;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(ObjectChoiceCtrl, wxChoice)
	EVT_CHOICE(wxID_ANY, ObjectChoiceCtrl::OnSelect)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////


class ObjectBottomBar : public wxPanel
{
public:
	ObjectBottomBar(wxWindow* parent, Observable<ObjectSettings>& objectSettings);
};

struct ObjectSidebarImpl
{
	ObjectSidebarImpl() : m_BottomBar(NULL), m_ObjectListBox(NULL) { }
	wxWindow* m_BottomBar;
	wxListBox* m_ObjectListBox;
	std::vector<AtlasMessage::sObjectsListItem> m_Objects;
};

ObjectSidebar::ObjectSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer), p(new ObjectSidebarImpl())
{
	wxArrayString strings;
	strings.Add(_("Entities"));
	strings.Add(_("Actors (all)"));
	m_MainSizer->Add(new ObjectChoiceCtrl(this, strings, *this), wxSizerFlags().Expand());

	p->m_ObjectListBox = new ObjectSelectListBox(this, scenarioEditor.GetToolManager());
	m_MainSizer->Add(p->m_ObjectListBox, wxSizerFlags().Proportion(1).Expand());

	m_BottomBar = new ObjectBottomBar(bottomBarContainer, scenarioEditor.GetObjectSettings());
}

ObjectSidebar::~ObjectSidebar()
{
	delete p;
}

void ObjectSidebar::OnFirstDisplay()
{
	wxBusyInfo busy (_("Loading list of objects"));

	// Get the list of objects from the game
	AtlasMessage::qGetObjectsList qry;
	qry.Post();
	p->m_Objects = *qry.objects;
	// Display first group of objects
	SetObjectFilter(0);
}

void ObjectSidebar::SetObjectFilter(int type)
{
	p->m_ObjectListBox->Freeze();
	p->m_ObjectListBox->Clear();
	for (std::vector<AtlasMessage::sObjectsListItem>::iterator it = p->m_Objects.begin(); it != p->m_Objects.end(); ++it)
	{
		if (it->type == type)
		{
			wxString id = it->id.c_str();
			wxString name = it->name.c_str();
			p->m_ObjectListBox->Append(name, new wxStringClientData(id));
		}
	}
	p->m_ObjectListBox->Thaw();
}

//////////////////////////////////////////////////////////////////////////

class PlayerComboBox : public wxComboBox
{
public:
	PlayerComboBox(wxWindow* parent, wxArrayString& choices, Observable<ObjectSettings>& objectSettings)
		: wxComboBox(parent, -1, choices[objectSettings.GetPlayerID()], wxDefaultPosition, wxDefaultSize, choices, wxCB_READONLY)
		, m_ObjectSettings(objectSettings)
	{
		m_Conn = m_ObjectSettings.RegisterObserver(1, &PlayerComboBox::OnObjectSettingsChange, this);
	}

private:

	ObservableScopedConnection m_Conn;
	Observable<ObjectSettings>& m_ObjectSettings;

	void OnObjectSettingsChange(const ObjectSettings& settings)
	{
		SetSelection((long)settings.GetPlayerID());
	}

	void OnSelect(wxCommandEvent& evt)
	{
		m_ObjectSettings.SetPlayerID(evt.GetInt());
		m_ObjectSettings.NotifyObserversExcept(m_Conn);
	}

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(PlayerComboBox, wxComboBox)
	EVT_COMBOBOX(wxID_ANY, PlayerComboBox::OnSelect)
END_EVENT_TABLE();

//////////////////////////////////////////////////////////////////////////

ObjectBottomBar::ObjectBottomBar(wxWindow* parent, Observable<ObjectSettings>& objectSettings)
	: wxPanel(parent, wxID_ANY)
{
	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	
	wxArrayString players;
	// TODO: get proper player names
	players.Add(_("Gaia"));
	players.Add(_("Player 1"));
	players.Add(_("Player 2"));
	players.Add(_("Player 3"));
	players.Add(_("Player 4"));
	players.Add(_("Player 5"));
	players.Add(_("Player 6"));
	players.Add(_("Player 7"));
	players.Add(_("Player 8"));
	wxComboBox* playerSelect = new PlayerComboBox(this, players, objectSettings);
	sizer->Add(playerSelect);

	wxWindow* variationSelect = new VariationControl(this, objectSettings);
	variationSelect->SetMinSize(wxSize(160, -1));
	wxSizer* variationSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Variation"));
	variationSizer->Add(variationSelect, wxSizerFlags().Proportion(1).Expand());
	sizer->Add(variationSizer, wxSizerFlags().Proportion(1));

	SetSizer(sizer);
}
