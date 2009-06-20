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

#include "ColourTester.h"

#include "ColourTesterImageCtrl.h"
#include "ColourTesterColourCtrl.h"
#include "ColourTesterFileCtrl.h"

#include "wx/dnd.h"

DEFINE_EVENT_TYPE(wxEVT_MY_IMAGE_CHANGED)

BEGIN_EVENT_TABLE(ColourTester, wxFrame)
	EVT_COMMAND(wxID_ANY, wxEVT_MY_IMAGE_CHANGED, ColourTester::OnImageChanged)
END_EVENT_TABLE()


// Allow drag-and-drop of files onto the window, as a convenient way of opening them
class DropTarget : public wxFileDropTarget
{
public:
	DropTarget(ColourTesterImageCtrl* imgctrl)
		: m_ImageCtrl(imgctrl)
	{}

	bool OnDropFiles(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), const wxArrayString& filenames)
	{
		if (filenames.GetCount() >= 1)
			m_ImageCtrl->SetImageFile(filenames[0]);
		return true;
	}
private:
	ColourTesterImageCtrl* m_ImageCtrl;
};


ColourTester::ColourTester(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, _("Colour Tester"), wxDefaultPosition, wxSize(800, 700))
{
	SetIcon(wxIcon(_T("ICON_ColourTester")));

	wxPanel* mainPanel = new wxPanel(this);

	//////////////////////////////////////////////////////////////////////////
	// Set up sizers:

	wxBoxSizer* vertSizer = new wxBoxSizer(wxVERTICAL);
	mainPanel->SetSizer(vertSizer);

	wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);
	vertSizer->Add(topSizer,
		wxSizerFlags().Proportion(1).Expand().Border(wxLEFT|wxRIGHT, 5));

	wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
	vertSizer->Add(bottomSizer,
		wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, 5));

	//////////////////////////////////////////////////////////////////////////
	// Add things to sizers:

	m_ImageCtrl = new ColourTesterImageCtrl(mainPanel);

	topSizer->Add(new ColourTesterFileCtrl(mainPanel, wxSize(200,200), m_ImageCtrl),
		wxSizerFlags().Expand().Border(wxALL, 10));

	topSizer->Add(m_ImageCtrl,
		wxSizerFlags().Proportion(1).Expand().Border(wxALL, 10));

	bottomSizer->Add((wxPanel*)new ColourTesterColourCtrl(mainPanel, m_ImageCtrl),
		wxSizerFlags().Border(wxALL, 10));

	m_StatusBar = new wxStatusBar(mainPanel, wxID_ANY);
	m_StatusBar->SetFieldsCount(2);
	int statusWidths[] = { -2, -1 };
	m_StatusBar->SetStatusWidths(2, statusWidths);
	vertSizer->Add(m_StatusBar,
		wxSizerFlags().Expand());

	//////////////////////////////////////////////////////////////////////////
	
	SetDropTarget(new DropTarget(m_ImageCtrl));

}

void ColourTester::OnImageChanged(wxCommandEvent& event)
{
	ColourTesterImageCtrl* imgCtrl = wxDynamicCast(event.GetEventObject(), ColourTesterImageCtrl);
	wxCHECK(imgCtrl, );
	m_StatusBar->SetStatusText(event.GetString(), 0);
	m_StatusBar->SetStatusText(imgCtrl->GetImageFiletype(), 1);
}
