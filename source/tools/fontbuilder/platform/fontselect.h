// $Id: fontselect.h,v 1.2 2004/06/18 22:05:58 philip Exp $

class FontSelectorDialog : public wxDialog
{
public:
	FontSelectorDialog(wxWindow* parent);
#ifdef _WIN32
	~FontSelectorDialog();
#endif

	wxString FontName;
	wxString FontFilename;

#ifdef _WIN32
	void OnFontSelect(wxCommandEvent& event);
	void OnOK(wxCommandEvent& event);
#endif

	static void DefaultFonts(wxString& Name0, wxString& Filename0, wxString& Name1, wxString& Filename1);

private:

#ifdef _WIN32
	wxArrayString FontNames;
	wxCharBuffer** FontFilenames;
#endif

	DECLARE_EVENT_TABLE()
};

