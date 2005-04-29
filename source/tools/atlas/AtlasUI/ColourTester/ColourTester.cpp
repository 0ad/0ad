#include "stdafx.h"

#include "ColourTester.h"

#include "ColourTesterImageCtrl.h"
#include "ColourTesterColourCtrl.h"
#include "ColourTesterFileCtrl.h"

//#include "AtlasObject/AtlasObject.h"
//#include "Datafile.h"

//#include "wx/file.h"

BEGIN_EVENT_TABLE(ColourTester, wxFrame)
//EVT_MENU(ID_Custom1, OnCreateEntity)
END_EVENT_TABLE()

ColourTester::ColourTester(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, _("Colour Tester"), wxDefaultPosition, wxSize(800, 700))
{
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
}
