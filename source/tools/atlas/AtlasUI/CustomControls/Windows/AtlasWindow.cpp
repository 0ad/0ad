#include "stdafx.h"

#include "AtlasWindow.h"

#include "AtlasObject/AtlasObject.h"
#include "AtlasWindowCommand.h"

#include "wx/artprov.h"
#include "wx/config.h"

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
		buttons->Finalise();

		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(topsizer, wxSizerFlags().Proportion(1).Centre().Border(wxLEFT|wxRIGHT|wxTOP, 10));
		sizer->Add(buttons, wxSizerFlags().Centre().Border(wxALL, 10));

		SetSizer(sizer);
		sizer->SetSizeHints(this);
	}

	void OnSave(wxCommandEvent& WXUNUSED(event)) { EndDialog(wxID_SAVE); }
	void OnNo(wxCommandEvent& WXUNUSED(event)) { EndDialog(wxID_NO); }

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(SaveOnExitDialog, wxDialog)
	EVT_BUTTON(wxID_SAVE, SaveOnExitDialog::OnSave)
	EVT_BUTTON(wxID_NO, SaveOnExitDialog::OnNo)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(AtlasWindow, wxFrame);

enum
{
	ID_Quit = 1,
	ID_New,
//	ID_Import,
//	ID_Export,
	ID_Open,
	ID_Save,
	ID_SaveAs
};

BEGIN_EVENT_TABLE(AtlasWindow, wxFrame)
	EVT_MENU(ID_New, AtlasWindow::OnNew)
//	EVT_MENU(ID_Import, AtlasWindow::OnImport)
//	EVT_MENU(ID_Export, AtlasWindow::OnExport)
	EVT_MENU(ID_Open, AtlasWindow::OnOpen)
	EVT_MENU(ID_Save, AtlasWindow::OnSave)
	EVT_MENU(ID_SaveAs, AtlasWindow::OnSaveAs)
	EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, AtlasWindow::OnMRUFile)
	EVT_MENU(ID_Quit,  AtlasWindow::OnQuit)

	EVT_MENU(wxID_UNDO, AtlasWindow::OnUndo)
	EVT_MENU(wxID_REDO, AtlasWindow::OnRedo)

	EVT_CLOSE(AtlasWindow::OnClose)
END_EVENT_TABLE()

AtlasWindow::AtlasWindow(wxWindow* parent, const wxString& title, const wxSize& size)
	: wxFrame(parent, wxID_ANY, _T(""), wxDefaultPosition, size),
	m_WindowTitle(title), m_FileHistory(9)
{

	wxMenuBar *menuBar = new wxMenuBar;
	SetMenuBar(menuBar);

	wxMenu *menuFile = new wxMenu;
	menuBar->Append(menuFile, _("&File"));
	{
		menuFile->Append(ID_New, _("&New"));
//		menuFile->Append(ID_Import, _("&Import..."));
//		menuFile->Append(ID_Export, _("&Export..."));
		menuFile->Append(ID_Open, _("&Open..."));
		menuFile->Append(ID_Save, _("&Save"));
		menuFile->Append(ID_SaveAs, _("Save &As..."));
		menuFile->AppendSeparator();//-----------
		menuFile->Append(ID_Quit,   _("E&xit"));
		m_FileHistory.UseMenu(menuFile);//-------
		m_FileHistory.AddFilesToMenu();
	}

	m_menuItem_Save = menuFile->FindItem(ID_Save); // to let it be greyed out
	wxASSERT(m_menuItem_Save);

	wxMenu *menuEdit = new wxMenu;
	menuBar->Append(menuEdit, _("&Edit"));
	{
		menuEdit->Append(wxID_UNDO, _("&Undo"));
		menuEdit->Append(wxID_REDO, _("&Redo"));
	}

	m_CommandProc.SetEditMenu(menuEdit);
	m_CommandProc.Initialize();

	m_FileHistory.Load(*wxConfigBase::Get());

	CreateStatusBar();
	//SetStatusText(_("Welcome to wxWidgets!"));

	SetCurrentFilename();
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
	wxFileDialog dlg (this, _("Select XML file to open"), _T(""), _T(""), _("XML files (*.xml)|*.xml|All files (*.*)|*.*"), wxOPEN);

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
			_("XML files (*.xml)|*.xml|All files (*.*)|*.*"), wxSAVE | wxOVERWRITE_PROMPT);

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

	AtlasObject::SaveToXML(file, GetCurrentFilename().GetFullPath());

	return true;
}

bool AtlasWindow::OpenFile(wxString filename)
{
	AtObj file (AtlasObject::LoadFromXML(filename));
	// TODO: Make sure it succeeded.

	AtlasWindowCommandProc* commandProc = AtlasWindowCommandProc::GetFromParentFrame(this);
	commandProc->Submit(new AtlasCommand_Begin(_("Open file"), this));
	ImportData(file);
	commandProc->Submit(new AtlasCommand_End());

	m_FileHistory.AddFileToHistory(filename);
	SetCurrentFilename(filename);

	return true;
}
