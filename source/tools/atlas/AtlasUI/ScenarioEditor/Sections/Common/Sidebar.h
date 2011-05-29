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

#ifndef INCLUDED_SIDEBAR
#define INCLUDED_SIDEBAR

class ScenarioEditor;

class Sidebar : public wxPanel
{
public:
	Sidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

	void OnSwitchAway();
	void OnSwitchTo();

	wxWindow* GetBottomBar() { return m_BottomBar; }

	virtual void OnMapReload() {}

protected:
	ScenarioEditor& m_ScenarioEditor;

	wxSizer* m_MainSizer; // vertical box sizer, used by most sidebars

	wxWindow* m_BottomBar; // window that goes at the bottom of the screen; may be NULL

	virtual void OnFirstDisplay() {}
		// should be overridden when sidebars need to do expensive construction,
		// so it can be delayed until it is required;

private:
	bool m_AlreadyDisplayed;
};

#endif // INCLUDED_SIDEBAR
