// $Id: fontselect.h,v 1.5 2004/06/19 13:46:11 philip Exp $

#include "wx/dialog.h"
#include "wx/filedlg.h"

#ifdef _WIN32
class FontSelectorDialog : public wxDialog
#else
class FontSelectorDialog : public wxFileDialog
#endif
{
public:
	FontSelectorDialog(wxWindow* parent);
	~FontSelectorDialog();

	wxString FontName;
	wxString FontFilename;

#ifdef _WIN32
	void OnFontSelect(wxCommandEvent& event);
	void OnOK(wxCommandEvent& event);
//#else
//	void OnInit(wxInitDialogEvent& event);
#endif

	static void DefaultFonts(wxString& Name0, wxString& Filename0, wxString& Name1, wxString& Filename1);

private:

#ifdef _WIN32
	wxArrayString FontNames;
	wxCharBuffer** FontFilenames;
#endif

	DECLARE_EVENT_TABLE()
};

