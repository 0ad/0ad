#include "stdafx.h"

#include "AtlasWindow.h"

#include "AtlasObject/AtlasObject.h"

#include "wx/intl.h"
#include "wx/menu.h"

IMPLEMENT_CLASS(AtlasWindow, wxFrame);

enum
{
	ID_Quit = 1,
	ID_Import,
	ID_Export
};

BEGIN_EVENT_TABLE(AtlasWindow, wxFrame)
	EVT_MENU(ID_Quit,  AtlasWindow::OnQuit)
	EVT_MENU(ID_Import, AtlasWindow::OnImport)
	EVT_MENU(ID_Export, AtlasWindow::OnExport)
	EVT_MENU(wxID_UNDO, AtlasWindow::OnUndo)
	EVT_MENU(wxID_REDO, AtlasWindow::OnRedo)
END_EVENT_TABLE()

AtlasWindow::AtlasWindow(wxWindow* parent, const wxString& title, const wxSize& size)
	: wxFrame(parent, wxID_ANY, _T(""), wxDefaultPosition, size)
	, m_WindowTitle(title)
{

	wxMenuBar *menuBar = new wxMenuBar;
	SetMenuBar(menuBar);

	wxMenu *menuFile = new wxMenu;
	menuBar->Append(menuFile, _("&File"));
	{
		menuFile->Append(ID_Import, _("&Import..."));
		menuFile->Append(ID_Export, _("&Export..."));
		menuFile->AppendSeparator();//-----------
		menuFile->Append(ID_Quit,   _("E&xit"));
	}

	wxMenu *menuEdit = new wxMenu;
	menuBar->Append(menuEdit, _("&Edit"));
	{
		menuEdit->Append(wxID_UNDO, _("&Undo"));
		menuEdit->Append(wxID_REDO, _("&Redo"));
	}

	m_CommandProc.SetEditMenu(menuEdit);
	m_CommandProc.Initialize();

	CreateStatusBar();
	//SetStatusText(_("Welcome to wxWidgets!"));

	SetDisplayedFilename(_T(""));
}

void AtlasWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close();
}

void AtlasWindow::OnUndo(wxCommandEvent& WXUNUSED(event))
{
	m_CommandProc.Undo();
}

void AtlasWindow::OnRedo(wxCommandEvent& WXUNUSED(event))
{
	m_CommandProc.Redo();
}


void AtlasWindow::OnImport(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog dlg (this, _T("Select XML file to import"), _T(""), _T(""), _T("XML files (*.xml)|*.xml"), wxOPEN);

	if (dlg.ShowModal() != wxID_OK)
		return;

	wxString filename = dlg.GetPath();

	AtObj file (AtlasObject::LoadFromXML(filename.c_str()));

	Import(file);

	SetDisplayedFilename(dlg.GetFilename());
}

void AtlasWindow::OnExport(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog dlg (this, _T("Select XML file to export"), _T(""), _T(""), _T("XML files (*.xml)|*.xml"), wxSAVE | wxOVERWRITE_PROMPT);

	if (dlg.ShowModal() != wxID_OK)
		return;

	wxString filename = dlg.GetPath();

	AtObj file (Export());

	AtlasObject::SaveToXML(file, filename.c_str());

	SetDisplayedFilename(dlg.GetFilename());
}


void AtlasWindow::SetDisplayedFilename(wxString filename)
{
	if (filename == _T(""))
		m_DisplayedFilename = _("Unnamed file");
	else
		m_DisplayedFilename = filename;

	SetTitle(m_WindowTitle + _T(" - ") + m_DisplayedFilename);
}
