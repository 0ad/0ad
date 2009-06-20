/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "ArchiveViewer.h"

#include "Misc/Version.h"

#include <set>
#include <algorithm>

#include "DatafileIO/BAR/BAR.h"
#include "DatafileIO/Stream/Stream.h"
#include "DatafileIO/Stream/wx.h"
using namespace DatafileIO;

#include "wx/dnd.h"
#include "wx/confbase.h"
#include "wx/wfstream.h"
#include "wx/tooltip.h"
#include "wx/progdlg.h"
#include "wx/regex.h"
#ifdef __WXMSW__
# include "wx/msw/registry.h"
#endif

// Possible TODO: Remove duplication of AtlasWindow code

enum {
	ID_ExtractSelected,
	ID_ExtractAll,
	ID_EnablePreview,
	ID_About
};

BEGIN_EVENT_TABLE(ArchiveViewer, wxFrame)
	EVT_MENU(wxID_OPEN, ArchiveViewer::OnOpen)
	EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, ArchiveViewer::OnMRUFile)
	EVT_MENU(wxID_EXIT, ArchiveViewer::OnQuit)
	EVT_MENU(ID_ExtractSelected, ArchiveViewer::OnExtractSelected)
	EVT_MENU(ID_ExtractAll, ArchiveViewer::OnExtractAll)
	EVT_MENU(ID_EnablePreview, ArchiveViewer::OnEnablePreview)
	EVT_MENU(ID_About, ArchiveViewer::OnAbout)
	EVT_CLOSE(ArchiveViewer::OnClose)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////


class FilterTextCtrl : public wxTextCtrl
{
public:
	FilterTextCtrl(wxWindow* parent, ArchiveViewer* archViewWin, const wxSize& size)
		: wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, size),
		m_archViewWin(archViewWin) {}
private:
	void OnChange(wxCommandEvent& WXUNUSED(event))
	{
		m_archViewWin->SetFilter(GetValue());
	}
	ArchiveViewer* m_archViewWin;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(FilterTextCtrl, wxTextCtrl)
	EVT_TEXT(wxID_ANY, FilterTextCtrl::OnChange)
END_EVENT_TABLE()

class FileListCtrl : public wxListCtrl
{
public:
	FileListCtrl(wxWindow* parent, ArchiveViewer* archViewWin)
		: wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT),
		m_archViewWin(archViewWin) {}
	void OnSelect(wxListEvent& event)
	{
		m_archViewWin->UpdatePreview(event.GetIndex());
	}
private:
	ArchiveViewer* m_archViewWin;
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(FileListCtrl, wxListCtrl)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, FileListCtrl::OnSelect)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

// Allow drag-and-drop of files onto the window, as a convenient way of opening them
//class DropTarget : public wxFileDropTarget
//{
//public:
//	DropTarget(ColourTesterImageCtrl* imgctrl)
//		: m_ImageCtrl(imgctrl)
//	{}
//
//	bool OnDropFiles(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), const wxArrayString& filenames)
//	{
//		if (filenames.GetCount() >= 1)
//			m_ImageCtrl->SetImageFile(filenames[0]);
//		return true;
//	}
//private:
//	ColourTesterImageCtrl* m_ImageCtrl;
//};


ArchiveViewer::ArchiveViewer(wxWindow* parent)
: wxFrame(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(600, 371))
, m_FileHistory(_T("Archive Viewer"))
, m_WindowTitle(wxString::Format(_("%s - Archive Viewer"), g_ProgramNameVersion.c_str()))
, m_BARStream(NULL), m_BARReader(NULL), m_PreviewEnabled(false)
{
	SetIcon(wxIcon(_T("ICON_ArchiveViewer")));

	wxToolTip::Enable(true);

	wxMenuBar* menuBar = new wxMenuBar;
	SetMenuBar(menuBar);

	wxMenu *menuFile = new wxMenu;
	menuBar->Append(menuFile, _("&File"));
	{
		menuFile->Append(wxID_OPEN, _("&Open..."));
		menuFile->AppendSeparator();//-----------
		menuFile->Append(wxID_EXIT,   _("E&xit"));
		m_FileHistory.UseMenu(menuFile);//-------
		m_FileHistory.AddFilesToMenu();
	}

	wxMenu *menuView = new wxMenu;
	menuBar->Append(menuView, _("&View"));
	{
		menuView->AppendCheckItem(ID_EnablePreview, _("&Preview"));
	}

	wxMenu *menuExtract = new wxMenu;
	menuBar->Append(menuExtract, _("&Extract"));
	{
		menuExtract->Append(ID_ExtractAll, _("&All files..."));
		menuExtract->Append(ID_ExtractSelected, _("&Selected..."));
	}

	wxMenu *menuHelp = new wxMenu;
	menuBar->Append(menuHelp, _("&Help"));
	{
		menuHelp->Append(ID_About, _("&About"));
	}


	m_FileHistory.Load(*wxConfigBase::Get());



	m_Splitter = new SnapSplitterWindow(this, wxSP_NOBORDER | wxSP_3DSASH);

	wxPanel* leftPanel = new wxPanel(m_Splitter);

	//////////////////////////////////////////////////////////////////////////
	// Set up sizers:

	wxBoxSizer* vertSizer = new wxBoxSizer(wxVERTICAL);
	leftPanel->SetSizer(vertSizer);

	wxBoxSizer* filterSizer = new wxBoxSizer(wxHORIZONTAL);

	//////////////////////////////////////////////////////////////////////////
	// Add things to sizers:

	m_FileListCtrl = new FileListCtrl(leftPanel, this);
	m_FileListCtrl->InsertColumn(0, _("Filename"), wxLIST_FORMAT_LEFT, 300);
	m_FileListCtrl->InsertColumn(1, _("File size (bytes)"), wxLIST_FORMAT_RIGHT, 100);
	m_FileListCtrl->InsertColumn(2, _("Last modified date"), wxLIST_FORMAT_LEFT, 150);
	vertSizer->Add(m_FileListCtrl, wxSizerFlags().Expand().Proportion(1).Border(wxALL, 5));

	vertSizer->Add(filterSizer,
		wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT|wxBOTTOM, 5));


	m_PreviewWindow = new FilePreviewer(m_Splitter);
	m_PreviewWindow->Hide();

	m_Splitter->Initialize(leftPanel);

	filterSizer->Add(new wxStaticText(leftPanel, wxID_ANY, _("&Search for file:")),
		wxSizerFlags().Centre().Border(wxALL, 5));

	FilterTextCtrl* filterTextCtrl = new FilterTextCtrl(leftPanel, this, wxSize(150, -1));
	filterTextCtrl->SetToolTip(_("You may use * to match any sequence characters, and ? to match a single character"));
	filterSizer->Add(filterTextCtrl, wxSizerFlags().Border(wxALL, 5));

	//////////////////////////////////////////////////////////////////////////

	SetCurrentFilename();
}

void ArchiveViewer::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(wxString::Format(_("%s - created by Philip Taylor (philip@wildfiregames.com / philip@zaynar.demon.co.uk)"), g_ProgramNameVersion.c_str()),
		_("About"), wxOK|wxCENTRE|wxICON_INFORMATION);
}

ArchiveViewer::~ArchiveViewer()
{
	delete m_BARStream;
	delete m_BARReader;
}

void ArchiveViewer::OnClose(wxCloseEvent& event)
{
	event.Skip();
	m_FileHistory.Save(*wxConfigBase::Get());
}

//struct compare_bar {
//	bool operator()(const BAREntry& x, const BAREntry& y) { return (x.filename < y.filename); }
//};

void ArchiveViewer::RedrawFileList(bool keepSelection)
{
	if (!m_BARReader || !m_BARStream)
		// No BAR file loaded
		return;

	std::vector<bool> filesSelected;
	filesSelected.resize(m_CachedFileData.size(), false);

	// If desired, remember a list of selected and focussed items
	int focussed = -1;
	if (keepSelection)
	{
		// Iterate through selected items
		long item = m_FileListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		while (item != -1)
		{
			filesSelected[ m_FileListCtrl->GetItemData(item) ] = true;
			item = m_FileListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		}

		// Remember single focussed item
		item = m_FileListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
		if (item != -1)
			focussed = m_FileListCtrl->GetItemData(item);
	}

	wxString filter;
	enum { NONE, SUBSTRING, WILDCARD } filterType = NONE; // attempt to use most efficient matching system
	if (m_Filter.Len())
	{
		if (m_Filter.Contains(_T("*")) || m_Filter.Contains(_T("?")))
		{
			filterType = WILDCARD;
			filter = _T("*") + m_Filter.Lower() + _T("*"); // match any substring, and case insensitive
		}
		else
		{
			filterType = SUBSTRING;
			filter = m_Filter.Lower();
		}
	}

	m_FileListCtrl->Freeze();
	m_FileListCtrl->DeleteAllItems();

	for (size_t i = 0; i < m_CachedFileData.size(); ++i)
	{
		if (   (filterType == SUBSTRING && !m_CachedFileData[i].NameLower.Contains(filter))
			|| (filterType == WILDCARD && !m_CachedFileData[i].NameLower.Matches(filter)))
			continue;

		wxListItem info;
		info.SetText(m_CachedFileData[i].Name);
		info.SetData((long)i);
		info.SetId((long)i);
		if (keepSelection)
		{
			int state = (filesSelected[i] ? wxLIST_STATE_SELECTED : 0) | (focussed == (int)i ? wxLIST_STATE_FOCUSED : 0);
			if (state)
			{
				info.SetState(state);
				info.SetStateMask(wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
			}
		}
		long idx = m_FileListCtrl->InsertItem(info);
		wxASSERT(idx != -1);
		m_FileListCtrl->SetItem(idx, 1, m_CachedFileData[i].Size);
		m_FileListCtrl->SetItem(idx, 2, m_CachedFileData[i].Date);
	}

	m_FileListCtrl->Thaw();
}


void ArchiveViewer::UpdateFileList()
{
	if (!m_BARReader || !m_BARStream)
		// No BAR file loaded
		return;

	wxRegEx commas (_T("(\\d+)(\\d{3})"), wxRE_ADVANCED); // for comma-ising filesizes

	// Get list of all files
	//	std::vector<BAREntry> files = m_BARReader->GetFileList(); // non-constref copy, so we can sort it
	//	std::sort(files.begin(), files.end(), compare_bar());
	const std::vector<BAREntry>& files = m_BARReader->GetFileList();

	m_CachedFileData.clear();
	m_CachedFileData.resize(files.size());

	for (size_t i = 0; i < files.size(); ++i)
	{
		m_CachedFileData[i].Name = wxString( utf16tow(files[i].filename).c_str() );
		m_CachedFileData[i].NameLower = m_CachedFileData[i].Name.Lower();

		m_CachedFileData[i].Size = wxString::Format(_T("%d"), files[i].filesize);
		while (commas.Replace(&m_CachedFileData[i].Size, _T("\\1,\\2"))) ;

		if (files[i].modified.year == 0)
			m_CachedFileData[i].Date = _("Unknown");
		else
		{
			wxDateTime date (files[i].modified.day, (wxDateTime::Month)(wxDateTime::Jan + files[i].modified.month-1), files[i].modified.year,
				files[i].modified.hour, files[i].modified.minute, files[i].modified.second, files[i].modified.msecond);
			m_CachedFileData[i].Date = wxString::Format(
				_T("%s %s.%03d"),
				date.FormatISODate().c_str(),
				date.FormatISOTime().c_str(),
				date.GetMillisecond());
		}
	}
}

void ArchiveViewer::OpenFile(const wxString& filename)
{
	wxFFileInputStream* fileStream = new wxFFileInputStream(filename);
	if (! fileStream->Ok())
	{
		wxLogError(_("Failed to open file"));
		delete fileStream;
		return;
	}

	SeekableInputStream* stream = new SeekableInputStreamFromWx(fileStream);
	BARReader* reader = new BARReader(*stream);
	if (! reader->Initialise())
	{
		wxLogError(_("Failed to read BAR file"));
		delete reader;
		delete stream;
		return;
	}

	// Delete old data, replace with new data
	delete m_BARStream;
	delete m_BARReader;
	m_BARStream = stream;
	m_BARReader = reader;

	UpdateFileList();
	RedrawFileList(false);

	m_FileHistory.AddFileToHistory(filename);
	SetCurrentFilename(filename);
}





void ArchiveViewer::OnExtractSelected(wxCommandEvent& WXUNUSED(event))
{
	ExtractFiles(true);
}

void ArchiveViewer::OnExtractAll(wxCommandEvent& WXUNUSED(event))
{
	ExtractFiles(false);
}

void ArchiveViewer::ExtractFiles(bool onlySelected)
{
	long item = -1;
	
	if (onlySelected)
	{
		// Find the first selected item, and make sure there is actually a selection
		item = m_FileListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

		if (item == -1)
		{
			wxMessageBox(_("Please select one or more files to extract."), _("No files selected"), wxICON_INFORMATION);
			return;
		}
	}

	// Work out where to extract the files to:

	// Assume same as last place, else default (from registry), else cwd
	wxString dir = m_LastExtractDir;
	if (! dir.Len())
	{
		dir = GetDefaultOpenDirectory();
		if (! dir.Len())
			dir = wxFileName::GetCwd();
	}

	// Let user choose final location
	wxDirDialog dlg(this, wxDirSelectorPromptStr, dir);
	if (dlg.ShowModal() != wxID_OK)
		return;

	// Remember the selection
	dir = dlg.GetPath();
	dir += wxFileName::GetPathSeparator();
	m_LastExtractDir = dir;
	wxFileName rootDir (dir);


	// Construct a list of items to extract:
	const std::vector<BAREntry>& files = m_BARReader->GetFileList();
	std::vector<int> selection;
	if (onlySelected)
	{
		while (item != -1)
		{
			selection.push_back(m_FileListCtrl->GetItemData(item)); // item was set at top of function
			item = m_FileListCtrl->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		}
	}
	else
	{
		// Extract all files
		for (int i = 0; i < (int)files.size(); ++i)
			selection.push_back(i);
	}

	bool aborted = false;

	{
		// Create the required directory structure:

		// Extract the list of all directories:
		std::set<std::wstring> dirs;
		for (size_t sel = 0; sel < selection.size(); ++sel)
		{
			const BAREntry& file = files[selection[sel]];
			wxString filename = wxString( utf16tow(file.filename).c_str() );
			int lastSlash = filename.Find(_T('\\'), true);
			if (lastSlash != -1)
				dirs.insert(filename.Mid(0, lastSlash+1).c_str());
		}

		// Construct the directory tree:
		for (std::set<std::wstring>::iterator it = dirs.begin(); it != dirs.end(); ++it)
		{
			// Append name-in-archive to target root directory, one dir
			// at a time, calling mkdir at each step if necessary
			wxFileName fullFilePath (rootDir);
			wxFileName filePath (it->c_str(), wxPATH_WIN);
			const wxArrayString& filePathDirs = filePath.GetDirs();
			for (size_t i = 0; i < filePathDirs.Count(); ++i)
			{
				fullFilePath.AppendDir(filePathDirs[i]);
				if (! wxDirExists(fullFilePath.GetPath()))
					wxMkdir(fullFilePath.GetPath());
			}
		}

		// Create progress display
		wxProgressDialog progress (_("Extracting files"), _("Please wait"), (int)selection.size(), this,
			wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME | wxPD_REMAINING_TIME | wxPD_SMOOTH);

		// Repeat for each selection file
		for (size_t sel = 0; sel < selection.size(); ++sel)
		{
			const BAREntry& file = files[selection[sel]];

			if (! progress.Update((int)sel, wxString::Format(_("Extracting %s"), file.filename.c_str())))
			{
				aborted = true;
				break;
			}

			// Append name-in-archive to target root directory, one dir
			// at a time, calling mkdir at each step if necessary
			wxFileName filePath (wxString( utf16tow(file.filename).c_str() ), wxPATH_WIN);
			filePath.MakeAbsolute(rootDir.GetPath());

			// Output to disk
			wxFFileOutputStream outStream(filePath.GetFullPath());
			if (! outStream.Ok())
				wxLogError(wxString::Format(_("Error opening output file %s"), filePath.GetFullPath().c_str()));
			else
			{
				SeekableOutputStreamFromWx str (outStream);
				m_BARReader->TransferFile(file, str);
				outStream.Close();

				// Set the file's timestamps to match the one specified in the archive
				if (file.modified.year != 0)
				{
					wxDateTime date (file.modified.day, (wxDateTime::Month)(wxDateTime::Jan + file.modified.month-1), file.modified.year,
						file.modified.hour, file.modified.minute, file.modified.second, file.modified.msecond);
					filePath.SetTimes(NULL, &date, &date);
				}
			}
		}
	}

	if (aborted)
		wxMessageBox(_("Extraction aborted."), _("Stopped"));
	else
		wxMessageBox(_("Extraction complete."), _("Finished"));
}


void ArchiveViewer::OnEnablePreview(wxCommandEvent& event)
{
	m_PreviewEnabled = event.IsChecked();
	
	if (m_PreviewEnabled)
	{
		// Make sure the window's not far too small
		if (GetSize().GetWidth() < 700)
			SetSize(-1, -1, 900, -1); // nobody is still using 800x600, are they?

		m_Splitter->SetDefaultSashPosition(600);
		m_Splitter->SplitVertically(m_Splitter->GetWindow1(), m_PreviewWindow);
	}
	else
		m_Splitter->Unsplit();

	UpdatePreview();
}

void ArchiveViewer::UpdatePreview(long item)
{
	if (! m_PreviewEnabled)
		return;

	// If the user opens a large archive file, enables the preview, selects the
	// bottom file, and presses shift+home, it attempts to redraw the preview
	// for every single file in sequence. That takes forever; so provide a way
	// to abort the preview update.
	if (wxGetKeyState(WXK_ESCAPE))
		return;

	// If none is specified, preview the first selected item
	if (item == -1)
		item = m_FileListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

	if (item == -1) // none selected
		return;

	const BAREntry& file = m_BARReader->GetFileList()[ m_FileListCtrl->GetItemData(item) ];
	SeekableInputStream* str = m_BARReader->GetFile(file);
	m_PreviewWindow->PreviewFile(wxString(utf16tow(file.filename).c_str()), *str);
	delete str;
}



void ArchiveViewer::OnOpen(wxCommandEvent& WXUNUSED(event))
{
	wxString path, name;
	if (GetCurrentFilename().IsOk())
	{
		path = GetCurrentFilename().GetPath();
		name = GetCurrentFilename().GetFullName();
	}
	else
	{
		path = GetDefaultOpenDirectory();
	}

	wxFileDialog dlg (this, _("Select archive file to open"), path, name, _("BAR files (*.bar)|*.bar|All files (*.*)|*.*"), wxOPEN);

	if (dlg.ShowModal() != wxID_OK)
		return;

	OpenFile(dlg.GetPath());
}

void ArchiveViewer::OnMRUFile(wxCommandEvent& event)
{
	wxString file (m_FileHistory.GetHistoryFile(event.GetId() - wxID_FILE1));
	if (file.Len())
		OpenFile(file);
}

void ArchiveViewer::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close();
}

void ArchiveViewer::SetCurrentFilename(wxFileName filename)
{
	m_CurrentFilename = filename;

	if (filename.IsOk())
		SetTitle(m_WindowTitle + _T(" - ") + filename.GetFullName());
	else
		SetTitle(m_WindowTitle);
}

wxString ArchiveViewer::GetDefaultOpenDirectory()
{
// TODO
/*
	wxRegKey key (_T("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft Games\\Age of Empires 3\\1.0"));
	if (key.Exists())
	{
		wxString dir;

		// The path might be the default value under the AppPath key, or it
		// might be the AppPath value; so check both.
		wxRegKey subkey (key, _T("AppPath"));
		if (subkey.Exists())
		{
			if (subkey.QueryValue(_T(""), dir)) // default value
				return dir;
		}
		else
		{
			if (key.HasValue(_T("AppPath")) && key.QueryValue(_T("AppPath"), dir))
				return dir;
		}
	}
*/

	return _T("");
}

void ArchiveViewer::SetFilter(const wxString& filter)
{
	m_Filter = filter;
	RedrawFileList(true);
}
