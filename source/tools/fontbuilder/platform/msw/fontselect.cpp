// $Id: fontselect.cpp,v 1.1 2004/06/17 19:32:04 philip Exp $

#include "stdafx.h"

#include "../fontselect.h"

#include "wx/msw/registry.h"

enum
{
	ID_FontList = wxID_HIGHEST+1,
	ID_FontPreview
};

BEGIN_EVENT_TABLE(FontSelectorDialog, wxDialog)
	EVT_LISTBOX(ID_FontList, FontSelectorDialog::OnFontSelect)
	EVT_BUTTON(wxID_OK, FontSelectorDialog::OnOK)
END_EVENT_TABLE()


FontSelectorDialog::FontSelectorDialog(wxWindow* parent)
	: wxDialog(parent, -1, wxString(wxT("Font selector")), wxDefaultPosition, wxSize(400, 200), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);

	wxString PreviewString;
	PreviewString += wxT("Aa");
	PreviewString += (wchar_t)0x0105; // LATIN SMALL LETTER A WITH OGONEK
	PreviewString += wxT("\nBb");
	PreviewString += (wchar_t)0x00DF; // LATIN SMALL LETTER SHARP S
	PreviewString += wxT("\nCc");
	PreviewString += (wchar_t)0x00E7; // LATIN SMALL LETTER C WITH CEDILLA
	PreviewString += (wchar_t)0x033F; // COMBINING DOUBLE OVERLINE
	PreviewString += wxT("\n");
	PreviewString += (wchar_t)0xFB4E; // HEBREW LETTER PE WITH RAFE
	PreviewString += (wchar_t)0xFB6B; // ARABIC LETTER VEH FINAL FORM
	PreviewString += (wchar_t)0xF915; // CJK COMPATIBILITY IDEOGRAPH-F915
	PreviewString += (wchar_t)0x3342; // SQUARE HOON

	wxBoxSizer* TopSizer = new wxBoxSizer(wxHORIZONTAL);
	wxListBox* FontListBox = new wxListBox(this, ID_FontList, wxDefaultPosition, wxSize(200, 100));
	TopSizer->Add(FontListBox, 0, wxGROW);
	TopSizer->Add(new wxStaticText(this, ID_FontPreview, PreviewString), 1, wxGROW);
	MainSizer->Add(TopSizer, 1);

	wxBoxSizer* ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	ButtonSizer->Add(new wxButton(this, wxID_OK, wxT("OK")), 0, wxALL, 10);
	ButtonSizer->Add(new wxButton(this, wxID_CANCEL, wxT("Cancel")), 0, wxALL, 10);
	MainSizer->Add(ButtonSizer, 0, wxALIGN_CENTER);

	SetSizer(MainSizer);

	// Get a list of all the fonts installed on the system

	wxRegKey FontKey (wxT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"));

	// Get the list of keys (= font names)
	wxString ValueName;
	long ValueIndex;
	FontKey.GetFirstValue(ValueName, ValueIndex);
	do {
		if (! ValueName.IsEmpty())
			FontNames.Add(ValueName);
	} while (FontKey.GetNextValue(ValueName, ValueIndex));

	FontNames.Sort();

	// Get all the filenames, and store in the listbox's client data

	FontFilenames = new wxCharBuffer*[FontNames.Count()];

	for (size_t i = 0; i < FontNames.Count(); ++i)
	{
		wxString t;
		FontKey.QueryValue(FontNames[i], t);
		wxCharBuffer* b = new wxCharBuffer(t.mb_str());
		FontFilenames[i] = b;
	}

	FontListBox->Set(FontNames, (void**)FontFilenames);
}

FontSelectorDialog::~FontSelectorDialog()
{
	for (size_t i = 0; i < FontNames.Count(); ++i)
		delete FontFilenames[i];
	delete[] FontFilenames;
}


void FontSelectorDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
	// Find where the fonts are on the disk
	wxRegKey FontPathKey (wxT("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"));
	wxString FontPath;
	FontPathKey.QueryValue(wxT("Fonts"), FontPath);

	wxListBox* FontList = (wxListBox*)wxWindow::FindWindowById(ID_FontList);

	int Selection = FontList->GetSelection();
	FontFilename = FontPath + wxT("\\");
	FontFilename += wxString::FromAscii( ((wxCharBuffer*) FontList->GetClientData(Selection))->data() );

	FontName = FontList->GetStringSelection();

	EndModal(wxID_OK);
}


void FontSelectorDialog::OnFontSelect(wxCommandEvent& event)
{
	wxString FontName = event.GetString();

	// Translate "Arial (TrueType)" into "Arial", etc
	wxRegEx ThingyRemove;
	ThingyRemove.Compile(wxT(" \\(.+\\)$"));
	ThingyRemove.Replace(&FontName, wxT(""));

	// Set the preview box to use that font
	wxFont PreviewFont (18, wxDEFAULT, wxNORMAL, wxNORMAL, false, FontName, wxFONTENCODING_SYSTEM);
	wxStaticText* PreviewText = (wxStaticText*)wxWindow::FindWindowById(ID_FontPreview);
	PreviewText->SetFont(PreviewFont);
}

void FontSelectorDialog::DefaultFonts(wxString& Name0, wxString& Filename0, wxString& Name1, wxString& Filename1)
{
	// Find where the fonts are on the disk
	wxRegKey FontPathKey (wxT("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"));
	wxString FontPath;
	FontPathKey.QueryValue(wxT("Fonts"), FontPath);

	Name0 = wxT("Verdana (TrueType)");
	Filename0 = FontPath + wxT("\\verdana.ttf");
	Name1 = wxT("Arial Unicode MS (TrueType)");
	Filename1 = FontPath + wxT("\\arialuni.ttf");
}
