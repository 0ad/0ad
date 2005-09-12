#include "stdafx.h"

#include "SnapSplitterWindow.h"

BEGIN_EVENT_TABLE(SnapSplitterWindow, wxSplitterWindow)
	EVT_SPLITTER_SASH_POS_CHANGING(wxID_ANY, SnapSplitterWindow::OnSashPosChanging)
END_EVENT_TABLE()

SnapSplitterWindow::SnapSplitterWindow(wxWindow* parent, long style)
	: wxSplitterWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
					   style | wxSP_LIVE_UPDATE),
					   m_SnapTolerance(16)
{
	// Set min size, to disable unsplitting
	SetMinimumPaneSize(32);

}

bool SnapSplitterWindow::SplitVertically(wxWindow *window1, wxWindow *window2, int sashPosition)
{
	m_DefaultSashPosition = sashPosition;
	return DoSplit(wxSPLIT_VERTICAL, window1, window2, sashPosition);
}
bool SnapSplitterWindow::SplitHorizontally(wxWindow *window1, wxWindow *window2, int sashPosition)
{
	m_DefaultSashPosition = sashPosition;
	return DoSplit(wxSPLIT_HORIZONTAL, window1, window2, sashPosition);
}

void SnapSplitterWindow::OnSashPosChanging(wxSplitterEvent& evt)
{
	if (evt.GetSashPosition() >= m_DefaultSashPosition-m_SnapTolerance &&
		evt.GetSashPosition() <= m_DefaultSashPosition+m_SnapTolerance)
	{
		evt.SetSashPosition(m_DefaultSashPosition);
	}
}
