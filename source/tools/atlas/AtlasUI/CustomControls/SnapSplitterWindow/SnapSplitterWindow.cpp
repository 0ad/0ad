#include "stdafx.h"

#include "SnapSplitterWindow.h"

BEGIN_EVENT_TABLE(SnapSplitterWindow, wxSplitterWindow)
	EVT_SPLITTER_SASH_POS_CHANGING(wxID_ANY, SnapSplitterWindow::OnSashPosChanging)
	EVT_SPLITTER_DCLICK(wxID_ANY, SnapSplitterWindow::OnDoubleClick)
END_EVENT_TABLE()

SnapSplitterWindow::SnapSplitterWindow(wxWindow* parent, long style)
	: wxSplitterWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
					   style | wxSP_LIVE_UPDATE),
					   m_SnapTolerance(16)
{
	// Set min size, to disable unsplitting
	SetMinimumPaneSize(32);
}

void SnapSplitterWindow::SetDefaultSashPosition(int sashPosition)
{
	m_DefaultSashPosition = sashPosition;

	// Set gravity so that the unspecified-size window is resized
	if (sashPosition < 0)
		SetSashGravity(1.0);
	else if (sashPosition == 0)
		SetSashGravity(0.5);
	else
		SetSashGravity(0.0);
}

bool SnapSplitterWindow::SplitVertically(wxWindow *window1, wxWindow *window2)
{
	return wxSplitterWindow::SplitVertically(window1, window2, m_DefaultSashPosition);
}

bool SnapSplitterWindow::SplitHorizontally(wxWindow *window1, wxWindow *window2)
{
	return wxSplitterWindow::SplitHorizontally(window1, window2, m_DefaultSashPosition);
}

void SnapSplitterWindow::OnSashPosChanging(wxSplitterEvent& evt)
{
	int defaultPos = ConvertSashPosition(m_DefaultSashPosition);

	if (evt.GetSashPosition() >= defaultPos-m_SnapTolerance &&
		evt.GetSashPosition() <= defaultPos+m_SnapTolerance)
	{
		evt.SetSashPosition(defaultPos);
	}
}

void SnapSplitterWindow::OnDoubleClick(wxSplitterEvent& WXUNUSED(evt))
{
	int defaultPos = ConvertSashPosition(m_DefaultSashPosition);
	SetSashPosition(defaultPos);
}
