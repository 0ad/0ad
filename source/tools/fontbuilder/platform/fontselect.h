// $Id: fontselect.h,v 1.1 2004/06/17 19:32:04 philip Exp $

class FontSelectorDialog : public wxDialog
{
public:
	FontSelectorDialog(wxWindow* parent);
	~FontSelectorDialog();

	wxString FontName;
	wxString FontFilename;

	void OnFontSelect(wxCommandEvent& event);
	void OnOK(wxCommandEvent& event);

	static void DefaultFonts(wxString& Name0, wxString& Filename0, wxString& Name1, wxString& Filename1);

private:

	wxArrayString FontNames;
	wxCharBuffer** FontFilenames;

	DECLARE_EVENT_TABLE()
};

