#include "AtlasWindowCommandProc.h"

#include "IAtlasExporter.h"

#include "wx/frame.h"

class AtObj;

class AtlasWindow : public wxFrame, public IAtlasExporter
{
	friend class AtlasWindowCommandProc;

	DECLARE_CLASS(AtlasWindow);

public:
	AtlasWindow(wxWindow* parent, const wxString& title, const wxSize& size);

	void OnQuit(wxCommandEvent& event);

	void OnImport(wxCommandEvent& event);
	void OnExport(wxCommandEvent& event);

	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);

protected:
	// Call with the name of the currently opened file, or with the
	// empty string for new unnamed documents
	void SetDisplayedFilename(wxString filename);

private:
	AtlasWindowCommandProc m_CommandProc;

	wxString m_DisplayedFilename;
	wxString m_WindowTitle;

	DECLARE_EVENT_TABLE();
};