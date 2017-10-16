/* Copyright (C) 2017 Wildfire Games.
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

#ifndef INCLUDED_SECTIONLAYOUT
#define INCLUDED_SECTIONLAYOUT

#include <map>
#include <string>

// Some platform dependent sizes
#if defined(__WXGTK__)
	#define SIDEBAR_SIZE 285
	#define BOTTOMBAR_SIZE 200
#elif defined(__WXOSX__) || defined(__WXMAC__)
	#define SIDEBAR_SIZE 285
	#define BOTTOMBAR_SIZE 210
#else	// __MSW__
	#define SIDEBAR_SIZE 235
	#define BOTTOMBAR_SIZE 180
#endif

class ScenarioEditor;
class SnapSplitterWindow;
class SidebarBook;
class wxWindow;

class SectionLayout
{
public:
	SectionLayout();
	~SectionLayout();

	void SetWindow(wxWindow* window);
	wxWindow* GetCanvasParent();
	void SetCanvas(wxWindow*);
	void Build(ScenarioEditor&);

	void SelectPage(const wxString& classname);

	void OnMapReload();

private:
	SidebarBook* m_SidebarBook;
	wxWindow* m_Canvas;
	SnapSplitterWindow* m_HorizSplitter;
	SnapSplitterWindow* m_VertSplitter;
	std::map<std::wstring, int> m_PageMappings;
};

#endif // INCLUDED_SECTIONLAYOUT
