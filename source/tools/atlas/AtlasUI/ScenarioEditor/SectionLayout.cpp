#include "stdafx.h"

#include "SectionLayout.h"

#include "SnapSplitterWindow/SnapSplitterWindow.h"

#include "Sections/Map/Map.h"
#include "Sections/Terrain/Terrain.h"

//////////////////////////////////////////////////////////////////////////

class SidebarNotebook : public wxNotebook
{
public:
	SidebarNotebook(wxWindow *parent, SnapSplitterWindow* splitter)
		: wxNotebook(parent, wxID_ANY), m_Splitter(splitter)
	{
	}

	// Only allow Sidebar objects to be added
	bool AddPage(Sidebar* sidebar, const wxString& text)
	{
		return wxNotebook::AddPage(sidebar, text);
	}

protected:
	void OnPageChanged(wxNotebookEvent& event)
	{
		Sidebar* oldPage = NULL;
		Sidebar* newPage = NULL;

		if (event.GetOldSelection() != -1)
			oldPage = wxDynamicCast(GetPage(event.GetOldSelection()), Sidebar);

		if (event.GetSelection() != -1)
			newPage = wxDynamicCast(GetPage(event.GetSelection()), Sidebar);

		if (m_Splitter->IsSplit())
		{
			wxWindow* bottom;
			if (newPage && NULL != (bottom = newPage->GetBottomBar(m_Splitter)))
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
			if (newPage && NULL != (bottom = newPage->GetBottomBar(m_Splitter)))
			{
				m_Splitter->SplitHorizontally(m_Splitter->GetWindow1(), bottom);
			}
		}
		event.Skip();
	}

private:
	SnapSplitterWindow* m_Splitter;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(SidebarNotebook, wxNotebook)
	EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, SidebarNotebook::OnPageChanged)
END_EVENT_TABLE();

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

void SectionLayout::Build()
{
	// TODO: wxWidgets bug (http://sourceforge.net/tracker/index.php?func=detail&aid=1298803&group_id=9863&atid=109863)
	// - pressing menu keys (e.g. alt+f) with notebook tab focussed causes application to freeze
	wxNotebook* sidebar = new SidebarNotebook(m_HorizSplitter, m_VertSplitter);
	sidebar->AddPage(new MapSidebar(sidebar), _("Map"), false);
	sidebar->AddPage(new TerrainSidebar(sidebar), _("Terrain"), false);

	m_VertSplitter->SetDefaultSashPosition(-165);
	m_VertSplitter->Initialize(m_Canvas);

	m_HorizSplitter->SetDefaultSashPosition(200);
	m_HorizSplitter->SplitVertically(sidebar, m_VertSplitter);
}
