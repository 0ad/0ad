// $Id: fontselect.cpp,v 1.1 2004/06/18 22:05:58 philip Exp $

#include "stdafx.h"

#include "../fontselect.h"

FontSelectorDialog::FontSelectorDialog(wxWindow* parent)
: wxDialog(parent, -1, wxString(wxT("Font selector")), wxDefaultPosition, wxSize(400, 200), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	wxFileDialog Dlg (this, wxT("Select a font file"), wxEmptyString, wxEmptyString, wxT("TTF files (*.ttf)|*.ttf|All files (*.*)|*.*"), wxOPEN | wxHIDE_READONLY)
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

void FontSelectorDialog::DefaultFonts(wxString& Name0, wxString& Filename0, wxString& Name1, wxString& Filename1)
{
	Name0 = wxT("Please select");
	Filename0 = wxT("");
	Name1 = wxT("Please select");
	Filename1 = wxT("");
}
