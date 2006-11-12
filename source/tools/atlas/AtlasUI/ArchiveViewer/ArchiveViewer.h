#include "FileHistory/FileHistory.h"
#include "SnapSplitterWindow/SnapSplitterWindow.h"
#include "FilePreviewer.h"

#include "wx/filename.h"
#include "wx/listctrl.h"

#include <vector>

namespace DatafileIO
{
	class BARReader;
	struct BAREntry;
	class SeekableInputStream;
}

class ArchiveViewer : public wxFrame
{
public:
	ArchiveViewer(wxWindow* parent);
	~ArchiveViewer();

	void OnOpen(wxCommandEvent& event);
	void OnMRUFile(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);
	void OnExtractSelected(wxCommandEvent& event);
	void OnExtractAll(wxCommandEvent& event);
	void OnEnablePreview(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

	void OnClose(wxCloseEvent& event);

	void SetFilter(const wxString& filter);
	void UpdatePreview(long index = -1);

private:
	void OpenFile(const wxString& filename);
	void UpdateFileList(); // must be called after opening a new file
	void RedrawFileList(bool keepSelection);

	void ExtractFiles(bool onlySelected);

	void SetCurrentFilename(wxFileName filename = wxString());
	wxFileName GetCurrentFilename() { return m_CurrentFilename; }

	wxString GetDefaultOpenDirectory();

	wxFileName m_CurrentFilename;
	wxString m_WindowTitle;

	wxString m_Filter;
	bool m_PreviewEnabled;
	FilePreviewer* m_PreviewWindow;
	SnapSplitterWindow* m_Splitter;

	wxString m_LastExtractDir;

//	wxStatusBar* m_StatusBar;

	wxListCtrl* m_FileListCtrl;
	DatafileIO::SeekableInputStream* m_BARStream;
	DatafileIO::BARReader* m_BARReader;

	struct FileData {
		wxString NameLower; // for filtering
		wxString Name;
		wxString Size;
		wxString Date;
	};
	std::vector<FileData> m_CachedFileData;

	FileHistory m_FileHistory;

	DECLARE_EVENT_TABLE();
};
