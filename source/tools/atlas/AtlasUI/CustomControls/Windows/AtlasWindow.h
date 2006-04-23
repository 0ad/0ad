#ifndef ATLASWINDOW_H__
#define ATLASWINDOW_H__

#include "General/AtlasWindowCommandProc.h"

#include "General/IAtlasSerialiser.h"
#include "FileHistory/FileHistory.h"

#include "wx/filename.h"

class AtObj;

class AtlasWindow : public wxFrame, public IAtlasSerialiser
{
	friend class AtlasWindowCommandProc;

	DECLARE_CLASS(AtlasWindow);

public:

	enum
	{
		ID_Quit = 1,
		ID_New,
		//	ID_Import,
		//	ID_Export,
		ID_Open,
		ID_Save,
		ID_SaveAs,

		// First available ID for custom window-specific menu items
		ID_Custom
	};

	AtlasWindow(wxWindow* parent, const wxString& title, const wxSize& size);

private:

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

	void AddCustomMenu(wxMenu* menu, const wxString& title);

	virtual wxString GetDefaultOpenDirectory() = 0;

	bool SaveChanges(bool forceSaveAs);
public:
	bool OpenFile(wxString filename);

private:
	AtlasWindowCommandProc m_CommandProc;

	wxMenuItem* m_menuItem_Save;
	wxMenuBar* m_MenuBar;

	wxFileName m_CurrentFilename;
	wxString m_WindowTitle;

	FileHistory m_FileHistory;

	DECLARE_EVENT_TABLE();
};

#endif // ATLASWINDOW_H__
