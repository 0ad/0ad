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

#include "precompiled.h"

#include "wx/filename.h"
#include "wx/wfstream.h"

#include "SectionLayout.h"

#include "CustomControls/SnapSplitterWindow/SnapSplitterWindow.h"

#include "ScenarioEditor.h"

#include "Sections/Cinema/Cinema.h"
#include "Sections/Environment/Environment.h"
#include "Sections/Map/Map.h"
#include "Sections/Object/Object.h"
#include "Sections/Player/Player.h"
#include "Sections/Terrain/Terrain.h"

#include "General/Datafile.h"

//////////////////////////////////////////////////////////////////////////

class SidebarButton : public wxBitmapButton
{
public:
	SidebarButton(wxWindow* parent, const wxBitmap& bitmap, SidebarBook* book, size_t id)
		: wxBitmapButton(parent, wxID_ANY, bitmap, wxDefaultPosition, wxSize(34, 32))
		, m_Book(book), m_Id(id)
	{
		SetSelectedAppearance(false);
	}

	void OnClick(wxCommandEvent& event);

	void SetSelectedAppearance(bool selected)
	{
		if (selected)
			SetBackgroundColour(wxColor(0xee, 0xcc, 0x55));
		else
			SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	}

private:
	SidebarBook* m_Book;
	size_t m_Id;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(SidebarButton, wxBitmapButton)
	EVT_BUTTON(wxID_ANY, SidebarButton::OnClick)
END_EVENT_TABLE();


class SidebarBook : public wxPanel
{
private:
	struct SidebarPage
	{
		SidebarPage() : button(NULL), bar(NULL) {}
		SidebarPage(SidebarButton* button, Sidebar* bar) : button(button), bar(bar) {}
		SidebarButton* button;
		Sidebar* bar;
	};

public:
	SidebarBook(wxWindow *parent, SnapSplitterWindow* splitter)
		: wxPanel(parent), m_Splitter(splitter), m_SelectedPage(-1)
	{
		m_ButtonsSizer = new wxGridSizer(6, 0, 0);

		wxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

		mainSizer->Add(m_ButtonsSizer);

		m_ContentWindow = new wxPanel(this);
		mainSizer->Add(m_ContentWindow, wxSizerFlags().Expand().Proportion(1).Border(wxALL, 4));

		SetSizer(mainSizer);
	}

	// Only allow Sidebar objects to be added
	bool AddPage(Sidebar* sidebar, const wxString& iconPNGFilename, const wxString& tooltip)
	{
		wxImage img (1, 1, true);

		// Load the icon
		wxFileName iconPath (_T("tools/atlas/toolbar/"));
		iconPath.MakeAbsolute(Datafile::GetDataDirectory());
		iconPath.SetFullName(iconPNGFilename);
		wxFFileInputStream fstr (iconPath.GetFullPath());
		if (! fstr.Ok())
		{
			wxLogError(_("Failed to open toolbar icon file '%s'"), iconPath.GetFullPath().c_str());
		}
		else
		{
			img = wxImage(fstr, wxBITMAP_TYPE_PNG);
			if (! img.Ok())
			{
				wxLogError(_("Failed to load toolbar icon image '%s'"), iconPath.GetFullPath().c_str());
				img = wxImage (1, 1, true);
			}
		}

		// Create the button for the sidebar toolbar
		SidebarButton* button = new SidebarButton(this, img, this, m_Pages.size());
		button->SetToolTip(tooltip);

		m_ButtonsSizer->Add(button);

		m_Pages.push_back(SidebarPage(button, sidebar));

		sidebar->Show(false);

		return true;
	}

	size_t GetPageCount()
	{
		return m_Pages.size();
	}

	wxWindow* GetContentWindow()
	{
		return m_ContentWindow;
	}

	void RepositionSelectedPage()
	{
		if (m_SelectedPage != -1 && m_Pages[m_SelectedPage].bar)
		{
			m_Pages[m_SelectedPage].bar->SetSize(m_ContentWindow->GetSize());
		}
	}

	void OnSize(wxSizeEvent& event)
	{
		Layout();
		RepositionSelectedPage();
		event.Skip();
	}

	void SetSelection(size_t page)
	{
		if (page < m_Pages.size())
		{
			// If selecting the same one twice, don't do anything
			if ((ssize_t)page == m_SelectedPage)
				return;

			SidebarPage oldPage;
			if (m_SelectedPage != -1)
				oldPage = m_Pages[m_SelectedPage];

			if (oldPage.bar)
				oldPage.bar->Show(false);

			m_SelectedPage = (ssize_t)page;
			RepositionSelectedPage();
			m_Pages[m_SelectedPage].bar->Show(true);

			OnPageChanged(oldPage, m_Pages[m_SelectedPage]);
		}
	}

	void OnMapReload()
	{
		for (size_t i = 0; i < m_Pages.size(); ++i)
			m_Pages[i].bar->OnMapReload();
	}

protected:

	void OnPageChanged(SidebarPage oldPage, SidebarPage newPage)
	{
		if (oldPage.bar)
		{
			oldPage.bar->OnSwitchAway();
			oldPage.button->SetSelectedAppearance(false);
		}

		if (newPage.bar)
		{
			newPage.bar->OnSwitchTo();
			newPage.button->SetSelectedAppearance(true);
		}

		if (m_Splitter->IsSplit())
		{
			wxWindow* bottom;
			if (newPage.bar && NULL != (bottom = newPage.bar->GetBottomBar()))
			{
				m_Splitter->ReplaceWindow(m_Splitter->GetWindow2(), bottom);
			}
			else
			{
				m_Splitter->Unsplit();
			}
		}
		else
		{
			wxWindow* bottom;
			if (newPage.bar && NULL != (bottom = newPage.bar->GetBottomBar()))
			{
				m_Splitter->SplitHorizontally(m_Splitter->GetWindow1(), bottom);
			}
		}
	}

private:
	wxSizer* m_ButtonsSizer;
	wxWindow* m_ContentWindow;
	SnapSplitterWindow* m_Splitter;

	std::vector<SidebarPage> m_Pages;
	ssize_t m_SelectedPage;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(SidebarBook, wxPanel)
	EVT_SIZE(SidebarBook::OnSize)
END_EVENT_TABLE();

void SidebarButton::OnClick(wxCommandEvent& WXUNUSED(event))
{
	m_Book->SetSelection(m_Id);
}

//////////////////////////////////////////////////////////////////////////

SectionLayout::SectionLayout()
{
}

SectionLayout::~SectionLayout()
{
}

void SectionLayout::SetWindow(wxWindow* window)
{
	m_HorizSplitter = new SnapSplitterWindow(window, wxSP_NOBORDER);
	m_VertSplitter = new SnapSplitterWindow(m_HorizSplitter, wxSP_3D);
}

wxWindow* SectionLayout::GetCanvasParent()
{
	return m_VertSplitter;
}

void SectionLayout::SetCanvas(wxWindow* canvas)
{
	m_Canvas = canvas;
}

void SectionLayout::Build(ScenarioEditor& scenarioEditor)
{
	// TODO: wxWidgets bug (http://sourceforge.net/tracker/index.php?func=detail&aid=1298803&group_id=9863&atid=109863)
	// - pressing menu keys (e.g. alt+f) with notebook tab focussed causes application to freeze

	m_SidebarBook = new SidebarBook(m_HorizSplitter, m_VertSplitter);
	Sidebar* sidebar;

	#define ADD_SIDEBAR(classname, icon, tooltip) \
		sidebar = new classname(scenarioEditor, m_SidebarBook->GetContentWindow(), m_VertSplitter); \
		if (sidebar->GetBottomBar()) \
			sidebar->GetBottomBar()->Show(false); \
		m_SidebarBook->AddPage(sidebar, icon, tooltip); \
		m_PageMappings.insert(std::make_pair(L###classname, (int)m_SidebarBook->GetPageCount()-1));

	ADD_SIDEBAR(MapSidebar,             _T("map.png"),         _("Map"));
	ADD_SIDEBAR(PlayerSidebar,          _T("player.png"),      _("Player"));
	ADD_SIDEBAR(TerrainSidebar,         _T("terrain.png"),     _("Terrain"));
	ADD_SIDEBAR(ObjectSidebar,          _T("object.png"),      _("Object"));
	ADD_SIDEBAR(EnvironmentSidebar,     _T("environment.png"), _("Environment"));
	ADD_SIDEBAR(CinemaSidebar,          _T("cinematic.png"),   _("Cinema"));

	#undef ADD_SIDEBAR

	m_VertSplitter->SetDefaultSashPosition(-BOTTOMBAR_SIZE);
	m_VertSplitter->Initialize(m_Canvas);

	m_HorizSplitter->SetDefaultSashPosition(SIDEBAR_SIZE);
	m_HorizSplitter->SplitVertically(m_SidebarBook, m_VertSplitter);
}

void SectionLayout::SelectPage(const wxString& classname)
{
	std::map<std::wstring, int>::iterator it = m_PageMappings.find((std::wstring)classname.wc_str());
	if (it != m_PageMappings.end())
		m_SidebarBook->SetSelection(it->second);
}

void SectionLayout::OnMapReload()
{
	m_SidebarBook->OnMapReload();
}
