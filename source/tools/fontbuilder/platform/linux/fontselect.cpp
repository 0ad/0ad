// $Id: fontselect.cpp,v 1.3 2004/06/19 12:56:09 philip Exp $

/************

  This attempt at completely separating the platform-dependent
  parts failed. Need to think about it more carefully in the
  future.

  ************/

#include "stdafx.h"

#include "../fontselect.h"

BEGIN_EVENT_TABLE(FontSelectorDialog, wxFileDialog)
//	EVT_INIT_DIALOG(FontSelectorDialog::OnInit)
END_EVENT_TABLE()

FontSelectorDialog::FontSelectorDialog(wxWindow* parent)
//: wxFileDialog(parent, wxT("Font selector"), wxEmptyString, wxEmptyString, wxOPEN | wxHIDE_READONLY)
: wxFileDialog(parent, wxT("Select a font file"), wxEmptyString, wxEmptyString, wxT("TTF files (*.ttf)|*.ttf|All files (*.*)|*.*"), wxOPEN | wxHIDE_READONLY)
{
}

/*
void FontSelectorDialog::OnDestroy(wxInitDialogEvent& WXUNUSED(event))
{
	wxFileDialog Dlg (this, wxT("Select a font file"), wxEmptyString, wxEmptyString, wxT("TTF files (*.ttf)|*.ttf|All files (*.*)|*.*"), wxOPEN | wxHIDE_READONLY);
	if (Dlg.ShowModal() == wxID_OK)
	{
		FontName = Dlg.GetFilename();
		FontFilename = Dlg.GetPath();
		EndModal(wxID_OK);
	}
	else
	{
		EndModal(wxID_CANCEL);
	}
}
*/

FontSelectorDialog::~FontSelectorDialog()
{
	FontName = GetFilename();
	FontFilename = GetPath();
}

void FontSelectorDialog::DefaultFonts(wxString& Name0, wxString& Filename0, wxString& Name1, wxString& Filename1)
{
	Name0 = wxT("Please select");
	Filename0 = wxT("");
	Name1 = wxT("Please select");
	Filename1 = wxT("");
}
