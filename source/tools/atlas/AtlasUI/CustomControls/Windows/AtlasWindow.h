#include "AtlasWindowCommandProc.h"

#include "IAtlasSerialiser.h"

#include "wx/filename.h"
#include "wx/docview.h"

class AtObj;

class AtlasWindow : public wxFrame, public IAtlasSerialiser
{
	friend class AtlasWindowCommandProc;

	DECLARE_CLASS(AtlasWindow);

public:
	AtlasWindow(wxWindow* parent, const wxString& title, const wxSize& size);

	void OnNew(wxCommandEvent& event);
//	void OnImport(wxCommandEvent& event);
//	void OnExport(wxCommandEvent& event);
	// TODO: import/export vs open/save/saveas - how should it decide which to do?
	void OnOpen(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnSaveAs(wxCommandEvent& event);

	void OnQuit(wxCommandEvent& event);

	void OnMRUFile(wxCommandEvent& event);

	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);

	void OnClose(wxCloseEvent& event);

protected:
	void SetCurrentFilename(wxFileName filename = wxString());
	wxFileName GetCurrentFilename() { return m_CurrentFilename; }

	bool SaveChanges(bool forceSaveAs);
public:
	bool OpenFile(wxString filename);

private:
	AtlasWindowCommandProc m_CommandProc;

	wxMenuItem* m_menuItem_Save;

	wxFileName m_CurrentFilename;
	wxString m_WindowTitle;

	wxFileHistory m_FileHistory;

	DECLARE_EVENT_TABLE();
};
