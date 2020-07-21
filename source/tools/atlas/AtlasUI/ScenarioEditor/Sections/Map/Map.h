/* Copyright (C) 2020 Wildfire Games.
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

#include <wx/collpane.h>

class MapSettingsControl;

class MapSidebar : public Sidebar
{
public:
	MapSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

	virtual void OnMapReload();

protected:
	virtual void OnFirstDisplay();

private:
	MapSettingsControl* m_MapSettingsCtrl;

	void OnCollapse(wxCollapsiblePaneEvent& evt);
	void OnOpenPlayerPanel(wxCommandEvent& evt);
	void OnRandomReseed(wxCommandEvent& evt);
	void OnRandomGenerate(wxCommandEvent& evt);
	void OnResizeMap(wxCommandEvent& evt);
	void OnSimPlay(wxCommandEvent& evt);
	void OnSimPause(wxCommandEvent& evt);
	void OnSimReset(wxCommandEvent& evt);
	void UpdateSimButtons();

	int m_SimState;

	DECLARE_EVENT_TABLE();
};
