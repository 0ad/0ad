/* Copyright (C) 2021 Wildfire Games.
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

class TexturePreviewPanel;

class TerrainSidebar : public Sidebar
{
public:
	TerrainSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

	void OnShutdown() override;

protected:
	void OnFirstDisplay() override;

private:
	void OnPassabilityChoice(wxCommandEvent& evt);
	void OnShowPriorities(wxCommandEvent& evt);

	wxChoice* m_PassabilityChoice;
	TexturePreviewPanel* m_TexturePreview;

	DECLARE_EVENT_TABLE();
};
