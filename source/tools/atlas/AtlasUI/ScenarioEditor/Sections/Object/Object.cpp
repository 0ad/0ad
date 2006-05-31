#include "stdafx.h"

#include "Object.h"

#include "Buttons/ActionButton.h"
#include "Buttons/ToolButton.h"
#include "ScenarioEditor/Tools/Common/Tools.h"
#include "ScenarioEditor/Tools/Common/ObjectSettings.h"
#include "ScenarioEditor/Tools/Common/MiscState.h"

#include "GameInterface/Messages.h"

class ObjectSelectListBox : public wxListBox
{
public:
	ObjectSelectListBox(wxWindow* parent)
		: wxListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_HSCROLL)
	{
	}

	void OnSelect(wxCommandEvent& evt)
	{
		// On selecting an object, enable the PlaceObject tool with this object
		wxString id = static_cast<wxStringClientData*>(evt.GetClientObject())->GetData();
		SetCurrentTool(_T("PlaceObject"), &id);
	}

private:
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

struct ObjectSidebarImpl
{
	ObjectSidebarImpl() : m_BottomBar(NULL), m_ObjectListBox(NULL) { }
	wxWindow* m_BottomBar;
	wxListBox* m_ObjectListBox;
	std::vector<AtlasMessage::sObjectsListItem> m_Objects;
};

ObjectSidebar::ObjectSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(sidebarContainer, bottomBarContainer), p(new ObjectSidebarImpl())
{
	wxArrayString strings;
	strings.Add(_("Entities"));
	strings.Add(_("Actors (all)"));
	m_MainSizer->Add(new ObjectChoiceCtrl(this, strings, *this), wxSizerFlags().Expand());

	p->m_ObjectListBox = new ObjectSelectListBox(this);
	m_MainSizer->Add(p->m_ObjectListBox, wxSizerFlags().Proportion(1).Expand());

	m_BottomBar = new ObjectBottomBar(bottomBarContainer);
}

ObjectSidebar::~ObjectSidebar()
{
	delete p;
}

void ObjectSidebar::OnFirstDisplay()
{
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
	PlayerComboBox(wxWindow* parent, wxArrayString& choices)
		: wxComboBox(parent, -1, choices[g_ObjectSettings.GetPlayerID()], wxDefaultPosition, wxDefaultSize, choices, wxCB_READONLY)
	{
		m_Conn = g_ObjectSettings.RegisterObserver(1, &PlayerComboBox::OnObjectSettingsChange, this);
	}

	~PlayerComboBox()
	{
		g_ObjectSettings.RemoveObserver(m_Conn);
	}

private:

	ObservableConnection m_Conn;

	void OnObjectSettingsChange(const ObjectSettings& settings)
	{
		SetSelection(settings.GetPlayerID());
	}

	void OnSelect(wxCommandEvent& evt)
	{
		g_ObjectSettings.SetPlayerID(evt.GetInt());
		g_ObjectSettings.NotifyObserversExcept(m_Conn);
	}

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(PlayerComboBox, wxComboBox)
	EVT_COMBOBOX(wxID_ANY, OnSelect)
END_EVENT_TABLE();


class VariationDisplay : public wxScrolledWindow
{
public:
	VariationDisplay(wxWindow* parent)
		: wxScrolledWindow(parent, -1)
	{
		m_Conn = g_ObjectSettings.RegisterObserver(1, &VariationDisplay::OnObjectSettingsChange, this);

		SetMinSize(wxSize(160, wxDefaultCoord));

		SetScrollRate(0, 5);

		m_Sizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(m_Sizer);
	}

	~VariationDisplay()
	{
		g_ObjectSettings.RemoveObserver(m_Conn);
	}

private:

	ObservableConnection m_Conn;

	std::vector<wxWindow*> m_ComboBoxes;
	wxSizer* m_Sizer;

	// Event handler shared by all the combo boxes created by this window
	void OnSelect(wxCommandEvent& evt)
	{
		std::set<wxString> selections;

		// It's possible for a variant name to appear in multiple groups.
		// If so, assume that all the names in each group are the same, so
		// we don't have to worry about some impossible combinations (e.g.
		// one group "a,b", a second "b,c", and a third "c,a", where's there's
		// no set of selections that matches one (and only one) of each group).
		//
		// So... When a combo box is changed from 'a' to 'b', add 'b' to the new
		// selections and make sure any other combo boxes containing both 'a' and
		// 'b' no longer contain 'a'.

		wxComboBox* thisComboBox = wxDynamicCast(evt.GetEventObject(), wxComboBox);
		wxString newValue = thisComboBox->GetValue();

		selections.insert(newValue);

		for (size_t i = 0; i < m_ComboBoxes.size(); ++i)
		{
			wxComboBox* comboBox = wxDynamicCast(m_ComboBoxes[i], wxComboBox);
			wxCHECK(comboBox != NULL, );
			// If our newly selected value is used in another combobox, we want
			// that combobox to use the new value, so don't add its old value
			// to the list of selections
			if (comboBox->FindString(newValue) == wxNOT_FOUND)
				selections.insert(comboBox->GetValue());
		}

		g_ObjectSettings.SetActorSelections(selections);
		g_ObjectSettings.NotifyObserversExcept(m_Conn);
		RefreshObjectSettings();
	}

	void OnObjectSettingsChange(const ObjectSettings& settings)
	{
		Freeze();

		const std::vector<ObjectSettings::Group>& variation = settings.GetActorVariation();

		// Creating combo boxes seems to be pretty expensive - so we create as
		// few as possible, by never deleting any.

		size_t oldCount = m_ComboBoxes.size();
		size_t newCount = variation.size();

		// If we have too many combo boxes, hide the excess ones
		for (size_t i = newCount; i < oldCount; ++i)
		{
			m_ComboBoxes[i]->Show(false);
		}

		for (size_t i = 0; i < variation.size(); ++i)
		{
			const ObjectSettings::Group& group = variation[i];

			if (i < oldCount)
			{
				// Already got enough boxes available, so use an old one
				wxComboBox* comboBox = wxDynamicCast(m_ComboBoxes[i], wxComboBox);
				wxCHECK(comboBox != NULL, );
				// Replace the contents of the old combobox with the new data
				comboBox->Freeze();
				comboBox->Clear();
				comboBox->Append(group.variants);
				comboBox->SetValue(group.chosen);
				comboBox->Show(true);
				comboBox->Thaw();
			}
			else
			{
				// Create an initially empty combobox, because we can fill it
				// quicker than the default constructor can
				wxComboBox* combo = new wxComboBox(this, -1, wxEmptyString, wxDefaultPosition,
					wxSize(130, wxDefaultCoord), wxArrayString(), wxCB_READONLY);
				// Freeze it before adding all the values
				combo->Freeze();
				combo->Append(group.variants);
				combo->SetValue(group.chosen);
				combo->Thaw();
				// Add the on-select event handler
				combo->Connect(wxID_ANY, wxEVT_COMMAND_COMBOBOX_SELECTED,
					wxCommandEventHandler(VariationDisplay::OnSelect), NULL, this);
				// Add box to sizer and list
				m_Sizer->Add(combo);
				m_ComboBoxes.push_back(combo);
			}
		}

		Layout();

		// Make the scrollbars appear when appropriate
		FitInside();

		Thaw();
	}

	void RefreshObjectSettings()
	{
		const std::vector<ObjectSettings::Group>& variation = g_ObjectSettings.GetActorVariation();

		// For each group, set the corresponding combobox's value to the chosen one
		size_t i = 0;
		for (std::vector<ObjectSettings::Group>::const_iterator group = variation.begin();
			group != variation.end() && i < m_ComboBoxes.size();
			++group, ++i)
		{
			wxComboBox* comboBox = wxDynamicCast(m_ComboBoxes[i], wxComboBox);
			wxCHECK(comboBox != NULL, );

			comboBox->SetValue(group->chosen);
		}
	}
};

//////////////////////////////////////////////////////////////////////////

ObjectBottomBar::ObjectBottomBar(wxWindow* parent)
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
	wxComboBox* playerSelect = new PlayerComboBox(this, players);
	sizer->Add(playerSelect);

	wxWindow* variationSelect = new VariationDisplay(this);
	wxSizer* variationSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Variation"));
	variationSizer->Add(variationSelect, wxSizerFlags().Proportion(1).Expand());
	sizer->Add(variationSizer, wxSizerFlags().Proportion(1));

	SetSizer(sizer);
}
