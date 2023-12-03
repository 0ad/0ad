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

#include "Sidebar.h"

Sidebar::Sidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* WXUNUSED(bottomBarContainer))
	: wxPanel(sidebarContainer), m_ScenarioEditor(scenarioEditor), m_BottomBar(NULL), m_AlreadyDisplayed(false)
{
	m_MainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_MainSizer);
}

void Sidebar::OnSwitchAway()
{
	if (m_BottomBar)
		m_BottomBar->Show(false);
}

void Sidebar::OnSwitchTo()
{
	if (! m_AlreadyDisplayed)
	{
		m_AlreadyDisplayed = true;
		OnFirstDisplay();
	}

	if (m_BottomBar)
		m_BottomBar->Show(true);
}
