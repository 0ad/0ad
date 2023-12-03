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

#include "../Common/Sidebar.h"

#include "GameInterface/Messages.h"

#include "wx/collpane.h"
#include "wx/spinctrl.h"

using namespace AtlasMessage;

class PlayerNotebookPage;
class PlayerSettingsControl;

class PlayerSidebar : public Sidebar
{
public:
	PlayerSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

	virtual void OnMapReload();

protected:
	virtual void OnFirstDisplay();

private:
	PlayerSettingsControl* m_PlayerSettingsCtrl;

	void OnCollapse(wxCollapsiblePaneEvent& evt);

	bool m_Loaded;

	DECLARE_EVENT_TABLE();
};

// Controls present on each player page
struct PlayerPageControls
{
	PlayerNotebookPage* page;

	wxTextCtrl* name;
	wxChoice* civ;
	wxButton* color;
	wxSpinCtrl* food;
	wxSpinCtrl* wood;
	wxSpinCtrl* stone;
	wxSpinCtrl* metal;
	wxSpinCtrl* pop;
	wxChoice* team;
	wxChoice* ai;
};

// Definitions for keeping AI data sorted
class AIData
{
public:
	AIData(const wxString& id, const wxString& name)
		: m_ID(id), m_Name(name)
	{
	}

	wxString& GetID()
	{
		return m_ID;
	}

	wxString& GetName()
	{
		return m_Name;
	}

	static int CompareAIData(AIData* ai1, AIData* ai2)
	{
		return ai1->m_Name.Cmp(ai2->m_Name);
	}

private:
	wxString m_ID;
	wxString m_Name;
};
WX_DEFINE_SORTED_ARRAY(AIData*, ArrayOfAIData);
