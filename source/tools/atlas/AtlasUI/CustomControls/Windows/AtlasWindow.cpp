/* Copyright (C) 2011 Wildfire Games.
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

#include "AtlasWindow.h"

#include "AtlasObject/AtlasObject.h"
#include "General/AtlasWindowCommand.h"
#include "General/Datafile.h"

#include "wx/artprov.h"
#include "wx/config.h"
#include "wx/file.h"

//////////////////////////////////////////////////////////////////////////

class SaveOnExitDialog : public wxDialog
{
public:
	SaveOnExitDialog(wxWindow* parent, bool allowCancel)
		: wxDialog(parent, wxID_ANY, (wxString) _("Save changes?"))
	{
		wxBitmap bitmap = wxArtProvider::GetIcon(wxART_WARNING, wxART_MESSAGE_BOX);
		wxBoxSizer* topsizer = new wxBoxSizer(wxHORIZONTAL);
		topsizer->Add(new wxStaticBitmap(this, wxID_ANY, bitmap),
						wxSizerFlags().Centre()/*.Border(wxALL, 14)*/);

		topsizer->Add(new wxStaticText(this, wxID_ANY, _("Would you like to save your changes to the current document?")),
						wxSizerFlags().Centre().Border(wxLEFT, 10));

		wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
		buttons->AddButton(new wxButton(this, wxID_SAVE, _("&Save changes"))); // use _SAVE (instead of _YES) so that nice things happen on Macs
		buttons->AddButton(new wxButton(this, wxID_NO, _("&Discard changes")));
		if (allowCancel)
			buttons->AddButton(new wxButton(this, wxID_CANCEL, _("&Cancel")));
		buttons->Realize();

		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(topsizer, wxSizerFlags().Proportion(1).Centre().Border(wxLEFT|wxRIGHT|wxTOP, 10));
		sizer->Add(buttons, wxSizerFlags().Centre().Border(wxALL, 10));

		SetSizer(sizer);
		sizer->SetSizeHints(this);
	}

	void OnSave(wxCommandEvent& WXUNUSED(event)) { EndModal(wxID_SAVE); }
	void OnNo(wxCommandEvent& WXUNUSED(event)) { EndModal(wxID_NO); }

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(SaveOnExitDialog, wxDialog)
	EVT_BUTTON(wxID_SAVE, SaveOnExitDialog::OnSave)
	EVT_BUTTON(wxID_NO, SaveOnExitDialog::OnNo)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(AtlasWindow, wxFrame);

BEGIN_EVENT_TABLE(AtlasWindow, wxFrame)
	EVT_MENU(wxID_NEW, AtlasWindow::OnNew)
//	EVT_MENU(ID_Import, AtlasWindow::OnImport)
//	EVT_MENU(ID_Export, AtlasWindow::OnExport)
	EVT_MENU(wxID_OPEN, AtlasWindow::OnOpen)
	EVT_MENU(wxID_SAVE, AtlasWindow::OnSave)
	EVT_MENU(wxID_SAVEAS, AtlasWindow::OnSaveAs)
	EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, AtlasWindow::OnMRUFile)
	EVT_MENU(wxID_EXIT,  AtlasWindow::OnQuit)

	EVT_MENU(wxID_UNDO, AtlasWindow::OnUndo)
	EVT_MENU(wxID_REDO, AtlasWindow::OnRedo)

	EVT_CLOSE(AtlasWindow::OnClose)
END_EVENT_TABLE()

AtlasWindow::AtlasWindow(wxWindow* parent, const wxString& title, const wxSize& size)
	: wxFrame(parent, wxID_ANY, _T(""), wxDefaultPosition, size),
	m_WindowTitle(title), m_FileHistory(title)
{

	m_MenuBar = new wxMenuBar;
	SetMenuBar(m_MenuBar);

	wxMenu *menuFile = new wxMenu;
	m_MenuBar->Append(menuFile, _("&File"));
	{
		menuFile->Append(wxID_NEW, _("&New\tCtrl+N"));
//		menuFile->Append(ID_Import, _("&Import..."));
//		menuFile->Append(ID_Export, _("&Export..."));
		menuFile->Append(wxID_OPEN, _("&Open...\tCtrl+O"));
		menuFile->Append(wxID_SAVE, _("&Save\tCtrl+S"));
		menuFile->Append(wxID_SAVEAS, _("Save &As..."));
		menuFile->AppendSeparator();//-----------
		menuFile->Append(wxID_EXIT,   _("E&xit"));
		m_FileHistory.UseMenu(menuFile);//-------
		m_FileHistory.AddFilesToMenu();
	}

	m_menuItem_Save = menuFile->FindItem(wxID_SAVE); // remember this item, to let it be greyed out
	wxASSERT(m_menuItem_Save);

	wxMenu *menuEdit = new wxMenu;
	m_MenuBar->Append(menuEdit, _("&Edit"));
	{
		menuEdit->Append(wxID_UNDO, _("&Undo"));
		menuEdit->Append(wxID_REDO, _("&Redo"));
	}

	m_CommandProc.SetEditMenu(menuEdit);
	m_CommandProc.Initialize();

	m_FileHistory.Load(*wxConfigBase::Get());

	CreateStatusBar();

	SetCurrentFilename();
}

void AtlasWindow::AddCustomMenu(wxMenu* menu, const wxString& title)
{
	m_MenuBar->Append(menu, title);
}

void AtlasWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close();
}

void AtlasWindow::OnClose(wxCloseEvent& event)
{
	SaveOnExitDialog dialog(this, event.CanVeto());
	int ret = dialog.ShowModal();

	if (ret == wxID_SAVE)
	{
		if (SaveChanges(false))
			event.Skip(); // save succeeded; exit
		else
			event.Veto(); // save failed; don't exit
	}
	else if (ret == wxID_NO)
	{
		// discard changes
		event.Skip();
	}
	else
	{
		assert(ret == wxID_CANCEL);
		event.Veto();
	}

	if (event.GetSkipped())
		m_FileHistory.Save(*wxConfigBase::Get());
}

void AtlasWindow::OnUndo(wxCommandEvent& WXUNUSED(event))
{
	m_CommandProc.Undo();
}

void AtlasWindow::OnRedo(wxCommandEvent& WXUNUSED(event))
{
	m_CommandProc.Redo();
}


//void AtlasWindow::OnImport(wxCommandEvent& WXUNUSED(event))
//{
//	...
//}
//
//void AtlasWindow::OnExport(wxCommandEvent& WXUNUSED(event))
//{
//	...
//}

void AtlasWindow::OnNew(wxCommandEvent& WXUNUSED(event))
{
	AtObj blank;

	AtlasWindowCommandProc* commandProc = AtlasWindowCommandProc::GetFromParentFrame(this);
	commandProc->Submit(new AtlasCommand_Begin(_("New file"), this));
	ImportData(blank);
	commandProc->Submit(new AtlasCommand_End());

	SetCurrentFilename();
}

void AtlasWindow::OnOpen(wxCommandEvent& WXUNUSED(event))
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

	wxFileDialog dlg (this, _("Select XML file to open"), path, name, _("XML files (*.xml)|*.xml|All files (*.*)|*.*"), wxFD_OPEN);

	if (dlg.ShowModal() != wxID_OK)
		return;

	OpenFile(dlg.GetPath());
}

void AtlasWindow::OnSave(wxCommandEvent& WXUNUSED(event))
{
	SaveChanges(false);
}

void AtlasWindow::OnSaveAs(wxCommandEvent& WXUNUSED(event))
{
	SaveChanges(true);
}

void AtlasWindow::OnMRUFile(wxCommandEvent& event)
{
	wxString file (m_FileHistory.GetHistoryFile(event.GetId() - wxID_FILE1));
	if (file.Length())
		OpenFile(file);
}


void AtlasWindow::SetCurrentFilename(wxFileName filename)
{
	m_CurrentFilename = filename;

	if (filename.IsOk())
		SetTitle(m_WindowTitle + _T(" - ") + filename.GetName());
	else
		SetTitle(m_WindowTitle + _T(" - ") + _("Unnamed file"));

	if (m_menuItem_Save)
		m_menuItem_Save->Enable(filename.IsOk());
}


bool AtlasWindow::SaveChanges(bool forceSaveAs)
{
	if (forceSaveAs || !GetCurrentFilename().IsOk())
	{
		wxFileDialog dlg (this, _("Select XML file to save as"),
			GetCurrentFilename().GetPath(), GetCurrentFilename().GetFullName(),
			//_T(""), _T(""),
			_("XML files (*.xml)|*.xml|All files (*.*)|*.*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

		if (dlg.ShowModal() != wxID_OK)
			return false;

		wxString filename(dlg.GetPath());

		m_FileHistory.AddFileToHistory(filename);
		SetCurrentFilename(filename);
	}

	if (! GetCurrentFilename().IsOk())
	{
		wxLogError(_T("Invalid 'save as' filename"));
		return false;
	}

	AtObj file (ExportData());
	// TODO: Make sure it succeeded. Back up .xml file in case it didn't.

	std::string xml = AtlasObject::SaveToXML(file);
	wxCHECK(!xml.empty(), false);

	wxFile outfile (GetCurrentFilename().GetFullPath(), wxFile::write);
	outfile.Write(xml.c_str(), xml.length());
	outfile.Close();

	sig_FileSaved();

	return true;
}

bool AtlasWindow::OpenFile(const wxString& filename)
{
	std::string xml;
	wxCHECK(Datafile::SlurpFile(filename, xml), false);
	AtObj file (AtlasObject::LoadFromXML(xml));
	// TODO: Make sure it succeeded.

	AtlasWindowCommandProc* commandProc = AtlasWindowCommandProc::GetFromParentFrame(this);
	commandProc->Submit(new AtlasCommand_Begin(_("Open file"), this));
	ImportData(file);
	commandProc->Submit(new AtlasCommand_End());

	m_FileHistory.AddFileToHistory(filename);
	SetCurrentFilename(filename);

	return true;
}
