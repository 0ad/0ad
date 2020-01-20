/* Copyright (C) 2020 Wildfire Games.
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

#include "SnapSplitterWindow.h"

#include <wx/confbase.h>

BEGIN_EVENT_TABLE(SnapSplitterWindow, wxSplitterWindow)
	EVT_SPLITTER_SASH_POS_CHANGING(wxID_ANY, SnapSplitterWindow::OnSashPosChanging)
	EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, SnapSplitterWindow::OnSashPosChanged)
	EVT_SPLITTER_DCLICK(wxID_ANY, SnapSplitterWindow::OnDoubleClick)
END_EVENT_TABLE()

SnapSplitterWindow::SnapSplitterWindow(wxWindow* parent, long style, const wxString& configPath)
	: wxSplitterWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
					   style | wxSP_LIVE_UPDATE),
					   m_SnapTolerance(16),
					   m_ConfigPath(configPath)
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

bool SnapSplitterWindow::SplitVertically(wxWindow *window1, wxWindow *window2, int sashPosition)
{
	if (sashPosition == 0)
		sashPosition = m_DefaultSashPosition;

	LoadSashPositionIfSaved(&sashPosition);

	return wxSplitterWindow::SplitVertically(window1, window2, sashPosition);
}

bool SnapSplitterWindow::SplitHorizontally(wxWindow *window1, wxWindow *window2, int sashPosition)
{
	if (sashPosition == 0)
		sashPosition = m_DefaultSashPosition;

	LoadSashPositionIfSaved(&sashPosition);

	return wxSplitterWindow::SplitHorizontally(window1, window2, sashPosition);
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

void SnapSplitterWindow::OnSashPosChanged(wxSplitterEvent& WXUNUSED(evt))
{
	SaveSashPositionIfChanged();
}

void SnapSplitterWindow::OnDoubleClick(wxSplitterEvent& WXUNUSED(evt))
{
	int defaultPos = ConvertSashPosition(m_DefaultSashPosition);
	SetSashPosition(defaultPos);
}

bool SnapSplitterWindow::LoadSashPositionIfSaved(int* sashPosition)
{
	wxASSERT(sashPosition);
	// We use the same config path for both horizontal and vertical splits,
	// because we can't have both at the same moment.
	wxConfigBase* cfg = wxConfigBase::Get(false);
	// wxConfigBase::Read doesn't modify the value if there was an error.
	return cfg && cfg->Read(m_ConfigPath + _T("SashPosition"), sashPosition);
}

void SnapSplitterWindow::SaveSashPositionIfChanged()
{
	wxConfigBase* cfg = wxConfigBase::Get(false);
	if (!cfg)
		return;
	cfg->Write(m_ConfigPath + _T("SashPosition"), GetSashPosition());
}
