#include "stdafx.h"

#include "Cinematic.h"

#include "GameInterface/Messages.h"

using namespace AtlasMessage;


class TrackListCtrl : public wxListCtrl
{
public:
	TrackListCtrl(CinematicSidebar* parent)
		: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|/*wxLC_EDIT_LABELS|*/wxLC_SINGLE_SEL),
		m_Sidebar(parent)
	{
		InsertColumn(0, _("Tracks"), wxLIST_FORMAT_LEFT, 180);
	}

	void OnSelect(wxListEvent& event)
	{
		m_Sidebar->SelectTrack(event.GetIndex());
	}

private:
	CinematicSidebar* m_Sidebar;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(TrackListCtrl, wxListCtrl)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, TrackListCtrl::OnSelect)
END_EVENT_TABLE()


class PathListCtrl : public wxListCtrl
{
public:
	PathListCtrl(CinematicSidebar* parent)
		: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL),
		m_Sidebar(parent)
	{
		InsertColumn(0, _("Paths"), wxLIST_FORMAT_LEFT, 180);
	}

	void OnSelect(wxListEvent& event)
	{
		m_Sidebar->SelectPath(event.GetIndex());
	}

private:
	CinematicSidebar* m_Sidebar;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(PathListCtrl, wxListCtrl)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, PathListCtrl::OnSelect)
END_EVENT_TABLE()


class NodeListCtrl : public wxListCtrl
{
public:
	NodeListCtrl(CinematicSidebar* parent)
		: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxSize(190, -1), wxLC_REPORT|wxLC_SINGLE_SEL),
		m_Sidebar(parent)
	{
		InsertColumn(0, _("Nodes"), wxLIST_FORMAT_LEFT, 180);
	}

	void OnSelect(wxListEvent& event)
	{
		m_Sidebar->SelectSplineNode(event.GetIndex());
	}

private:
	CinematicSidebar* m_Sidebar;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(NodeListCtrl, wxListCtrl)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, NodeListCtrl::OnSelect)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

class CinematicBottomBar : public wxPanel
{
public:
	CinematicBottomBar(wxWindow* parent)
		: wxPanel(parent)
	{
		// TODO: there needs to be stuff here.
	}
private:
};

//////////////////////////////////////////////////////////////////////////

CinematicSidebar::CinematicSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(sidebarContainer, bottomBarContainer),
m_SelectedTrack(-1), m_SelectedPath(-1), m_SelectedSplineNode(-1)
{
	m_TrackList = new TrackListCtrl(this);
	m_MainSizer->Add(m_TrackList, wxSizerFlags().Expand().Proportion(1));

	m_PathList = new PathListCtrl(this);
	m_MainSizer->Add(m_PathList, wxSizerFlags().Expand().Proportion(1));

	m_BottomBar = new CinematicBottomBar(bottomBarContainer);
}

void CinematicSidebar::OnFirstDisplay()
{
	qGetCinemaTracks qry;
	qry.Post();
	
	m_Tracks = *qry.tracks;

	m_TrackList->Freeze();
	for (size_t track = 0; track < m_Tracks.size(); ++track)
	{
		m_TrackList->InsertItem((long)track, wxString(m_Tracks[track].name.c_str()));
	}
	m_TrackList->Thaw();
}

void CinematicSidebar::SelectTrack(ssize_t n)
{
	if (n == -1)
	{
		m_PathList->DeleteAllItems();
	}
	else
	{
		wxCHECK_RET (n >= 0 && n < (ssize_t)m_Tracks.size(), _T("SelectTrack out of bounds"));

		m_PathList->Freeze();
		m_PathList->DeleteAllItems();

		if (n != -1)
		{
			std::vector<sCinemaPath> paths = *m_Tracks[n].paths;
			for (size_t path = 0; path < paths.size(); ++path)
			{
				m_PathList->InsertItem((long)path, wxString::Format(_("Path #%d"), path+1));
			}
		}

		m_PathList->Thaw();
	}

	m_SelectedTrack = n;
	SelectPath(-1);
}

void CinematicSidebar::SelectPath(ssize_t n)
{
	if (n == -1 || m_SelectedTrack == -1)
	{
		m_PathList->DeleteAllItems();
	}
	else
	{
		std::vector<sCinemaPath> paths = *m_Tracks[m_SelectedTrack].paths;
		wxCHECK_RET (n >= 0 && n < (ssize_t)paths.size(), _T("SelectPath out of bounds"));

		// TODO: modify the node-list (which doesn't yet exist) to contain
		// the data about the selected path
	}

	m_SelectedPath = n;
	SelectSplineNode(-1);
}

void CinematicSidebar::SelectSplineNode(ssize_t n)
{
	// TODO: modify the controls that allow modification of this node
}