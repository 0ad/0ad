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

/*
   Desc: receives user input and communicates with the engine
        to perform various cinematic functions.
*/

#include "../Common/Sidebar.h"

#include "GameInterface/Messages.h"

class PathListCtrl;
class NodeListCtrl;
class PathSlider;
class CinemaSpinnerBox;
class CinemaInfoBox;
class CinematicBottomBar;
class CinemaButtonBox;
class wxImage;

class CinematicSidebar : public Sidebar
{
	//For ease (from lazyness)
	friend class CinemaButtonBox;
	friend class PathSlider;

public:
	
	CinematicSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer);
	
	//The actual data is stored in bottom bar, but is controled from here
	void SelectPath(ssize_t n);
	//avoid excessive shareable->vector conversion with size paramater
	void SelectSplineNode(ssize_t n, ssize_t size = -1);
	
	void AddPath(std::wstring& name, int count);
	void AddNode(float px, float py, float pz, float rx, float ry, float rz, int count);
	void UpdatePath(std::wstring name, float timescale);
	void UpdateNode(float px, float py, float pz, float rx, float ry, float rz, bool absoluteOveride, float t=-1);

	void DeleteTrack();
	void DeletePath();
	void DeleteNode();
	
	void SetSpinners(CinemaSpinnerBox* box) { m_SpinnerBox = box; }
	
	const AtlasMessage::sCinemaPath* GetCurrentPath() const;
	AtlasMessage::sCinemaSplineNode GetCurrentNode() const;
	
	std::wstring GetSelectedPathName() const;
	int GetSelectedPath() const { return m_SelectedPath; }
	int GetSelectedNode() const { return m_SelectedSplineNode; }

	void GotoNode(ssize_t index=-1);

	void UpdatePathInfo(int mode, int style, float growth, float change, bool drawCurrent, bool drawLine);
	void UpdateSpinners();
	void UpdateTexts();

	void UpdateEngineData();
	void SendEngineSelection();

	float m_TimeElapsed;	//path time
	float m_AbsoluteTime;	//track time
	
	bool m_RotationAbsolute;	//rotation display flag in spinner box
	bool m_Playing;

protected:
	virtual void OnFirstDisplay();

private:
	// Stores all cinematics data for this map. Initialised by OnFirstDisplay.
	// Sent back to the game by [TODO]. (TODO: handle 'undo' correctly)
	std::vector<AtlasMessage::sCinemaPath> m_Paths;
	CinemaButtonBox* m_IconSizer;

	ssize_t m_SelectedPath; // -1 for none
	ssize_t m_SelectedSplineNode; // -1 for none
	
	PathListCtrl* m_PathList;
	NodeListCtrl* m_NodeList;

	PathSlider* m_PathSlider;
	CinemaSpinnerBox* m_SpinnerBox;		//We must update the display
	CinemaInfoBox* m_InfoBox;	// ^same^
	CinematicBottomBar* m_CinemaBottomBar;

	wxImage LoadIcon(const wxString& filename);
	void LoadIcons();
};
