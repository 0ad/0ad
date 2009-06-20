/**
 * =========================================================================
 * File        : ErrorReporter.cpp
 * Project     : 0 A.D.
 * Description : preview and send crashlogs to server.
 *
 * @author jan@wildfiregames.com, joe@wildfiregames.com
 * =========================================================================
 */

/*
 * based on dbgrptg.cpp,
 * Copyright (c) 2005 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
 * License: wxWindows license
 */

#include "precompiled.h"

#if 0

#include "wx/statline.h"
#include "wx/mimetype.h"

static const wxChar* DIALOG_TITLE = _T("WildfireGames Reporting Utility");
static const wxChar* PROBLEM_TITLE = _T("Problem Report for 0AD");
static const wxChar* DIALOG_REGRET = _T("Much to our regret, we must report the program has encountered a problem.\n");
static const wxChar* DIALOG_REPGEN = _T("A problem report has been generated in the directory\n");
static const wxChar* DIALOG_SPACER = _T("\n");
static const wxChar* DIALOG_FILES = _T("The report contains the files listed below.\nWe believe they hold information that will help us to improve the program,\nso please take a minute and send them!\n");
static const wxChar* DIALOG_PRIVACY = _T("If any of these files contain private information,\nplease uncheck them and they will be removed from the report.\n");
static const wxChar* DIALOG_RESPECT = _T("We respect your privacy so if you do not wish to send us this problem report\nwe understand and you can use the \"Cancel\" button.\n");
static const wxChar* DIALOG_APOLOGY = _T("Thank you and we're sorry for the inconvenience!\n");
static const wxChar* DIALOG_ADDL_INFO = _T("If you have any additional information pertaining to this problem report,\n please enter it here and it will be joined to it:");
// This is displayed to indicate a successful upload of the report file.
static const wxChar* DIALOG_UPLOAD = _T("Report successfully uploaded.");
// Default location for the crash files to include in the report file.
static const wxChar* LOGS_LOCATION = _T("C:\\0AD\\BINARIES\\LOGS\\");


// ----------------------------------------------------------------------------
// wxDumpPreviewDlg: simple class for showing ASCII preview of dump files
// ----------------------------------------------------------------------------

class wxDumpPreviewDlg : public wxDialog
{
public:
	wxDumpPreviewDlg(wxWindow *parent,
		const wxString& title,
		const wxString& text);

private:
	// the text we show
	wxTextCtrl *m_text;

	DECLARE_NO_COPY_CLASS(wxDumpPreviewDlg)
};

wxDumpPreviewDlg::wxDumpPreviewDlg(wxWindow *parent, const wxString& title, const wxString& text)
: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// create controls
	// ---------------

	// use wxTE_RICH2 style to avoid 64kB limit under MSW and display big files
	// faster than with wxTE_RICH
	m_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
		wxPoint(0, 0), wxDefaultSize,
		wxTE_MULTILINE |
		wxTE_READONLY |
		wxTE_NOHIDESEL |
		wxTE_RICH2);
	m_text->SetValue(text);

	// use fixed-width font
	m_text->SetFont(wxFont(12, wxFONTFAMILY_TELETYPE,
		wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	wxButton *btnClose = new wxButton(this, wxID_CANCEL, _("Close"));


	// layout them
	// -----------

	wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL),
		*sizerBtns = new wxBoxSizer(wxHORIZONTAL);

	sizerBtns->Add(btnClose, 0, 0, 1);

	sizerTop->Add(m_text, 1, wxEXPAND);
	sizerTop->Add(sizerBtns, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM | wxRIGHT, 1);

	// set the sizer &c
	// ----------------

	// make the text window bigger to show more contents of the file
	sizerTop->SetItemMinSize(m_text, 600, 300);
	SetSizer(sizerTop);

	Layout();
	Fit();

	m_text->SetFocus();
}

// ----------------------------------------------------------------------------
// wxDumpOpenExternalDlg: choose a command for opening the given file
// ----------------------------------------------------------------------------

class wxDumpOpenExternalDlg : public wxDialog
{
public:
	wxDumpOpenExternalDlg(wxWindow *parent, const wxFileName& filename);

	// return the command chosed by user to open this file
	const wxString& GetCommand() const { return m_command; }

	wxString m_command;

private:

#if wxUSE_FILEDLG
	void OnBrowse(wxCommandEvent& event);
#endif // wxUSE_FILEDLG

	DECLARE_EVENT_TABLE()
	DECLARE_NO_COPY_CLASS(wxDumpOpenExternalDlg)
};

BEGIN_EVENT_TABLE(wxDumpOpenExternalDlg, wxDialog)

#if wxUSE_FILEDLG
EVT_BUTTON(wxID_MORE, wxDumpOpenExternalDlg::OnBrowse)
#endif

END_EVENT_TABLE()


wxDumpOpenExternalDlg::wxDumpOpenExternalDlg(wxWindow *parent, const wxFileName& filename)
: wxDialog(parent, wxID_ANY,
wxString::Format
(
	_("Open file \"%s\""),
	filename.GetFullPath().c_str()
)
)
{
	// create controls
	// ---------------

	wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);
	sizerTop->Add(new wxStaticText(this, wxID_ANY,
		wxString::Format
		(
		_("Enter command to open file \"%s\":"),
		filename.GetFullName().c_str()
		)),
		wxSizerFlags().Border());

	wxSizer *sizerH = new wxBoxSizer(wxHORIZONTAL);

	wxTextCtrl *command = new wxTextCtrl
		(
		this,
		wxID_ANY,
		wxEmptyString,
		wxDefaultPosition,
		wxSize(250, wxDefaultCoord),
		0
#if wxUSE_VALIDATORS
		,wxTextValidator(wxFILTER_NONE, &m_command)
#endif
		);
	sizerH->Add(command,
		wxSizerFlags(1).Align(wxALIGN_CENTER_VERTICAL));

#if wxUSE_FILEDLG

	wxButton *browse = new wxButton(this, wxID_MORE, wxT(">>"),
		wxDefaultPosition, wxDefaultSize,
		wxBU_EXACTFIT);
	sizerH->Add(browse,
		wxSizerFlags(0).Align(wxALIGN_CENTER_VERTICAL). Border(wxLEFT));

#endif // wxUSE_FILEDLG

	sizerTop->Add(sizerH, wxSizerFlags(0).Expand().Border());

	sizerTop->Add(new wxStaticLine(this), wxSizerFlags().Expand().Border());

	sizerTop->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL),
		wxSizerFlags().Align(wxALIGN_RIGHT).Border());

	// set the sizer &c
	// ----------------

	SetSizer(sizerTop);

	Layout();
	Fit();

	command->SetFocus();
}

#if wxUSE_FILEDLG

void wxDumpOpenExternalDlg::OnBrowse(wxCommandEvent& )
{
	wxFileName fname(m_command);
	wxFileDialog dlg(this,
		wxFileSelectorPromptStr,
		fname.GetPathWithSep(),
		fname.GetFullName()
#ifdef __WXMSW__
		, _("Executable files (*.exe)|*.exe|All files (*.*)|*.*||")
#endif // __WXMSW__
		);
	if ( dlg.ShowModal() == wxID_OK )
	{
		m_command = dlg.GetPath();
		TransferDataToWindow();
	}
}

#endif // wxUSE_FILEDLG

// ----------------------------------------------------------------------------
// wxDebugReportDialog: class showing debug report to the user
// ----------------------------------------------------------------------------

class wxDebugReportDialog : public wxDialog
{
public:
	wxDebugReportDialog(wxDebugReport& dbgrpt);

	virtual bool TransferDataToWindow();
	virtual bool TransferDataFromWindow();

private:
	void OnView(wxCommandEvent& );
	void OnViewUpdate(wxUpdateUIEvent& );
	void OnOpen(wxCommandEvent& );


	// small helper: add wxEXPAND and wxALL flags
	static wxSizerFlags SizerFlags(int proportion)
	{
		return wxSizerFlags(proportion).Expand().Border();
	}


	wxDebugReport& m_dbgrpt;

	wxCheckListBox *m_checklst;
	wxTextCtrl *m_notes;

	wxArrayString m_files;

	DECLARE_EVENT_TABLE()
	DECLARE_NO_COPY_CLASS(wxDebugReportDialog)
};

// ============================================================================
// wxDebugReportDialog implementation
// ============================================================================

BEGIN_EVENT_TABLE(wxDebugReportDialog, wxDialog)
EVT_BUTTON(wxID_VIEW_DETAILS, wxDebugReportDialog::OnView)
EVT_UPDATE_UI(wxID_VIEW_DETAILS, wxDebugReportDialog::OnViewUpdate)
EVT_BUTTON(wxID_OPEN, wxDebugReportDialog::OnOpen)
EVT_UPDATE_UI(wxID_OPEN, wxDebugReportDialog::OnViewUpdate)
END_EVENT_TABLE()


// ----------------------------------------------------------------------------
// construction
// ----------------------------------------------------------------------------
/**
 * wxDebugReportDialog: This contains modifications to implement changes to
 * the content of the dialog.
 *
 * @param wxDebugReport & dbgrpt reference to compressed report file.
 **/
wxDebugReportDialog::wxDebugReportDialog(wxDebugReport& dbgrpt)
: wxDialog(NULL, wxID_ANY,
		   wxString(DIALOG_TITLE),
		   wxDefaultPosition,
		   wxDefaultSize,
		   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
		   m_dbgrpt(dbgrpt)
{
	// upper part of the dialog: explanatory message
	wxString msg;
	msg << DIALOG_REGRET
		<< DIALOG_REPGEN
		<< _T("\"") << dbgrpt.GetDirectory() << _T("\"\n")
		<< DIALOG_SPACER
		<< DIALOG_FILES
		<< DIALOG_PRIVACY
		<< DIALOG_RESPECT
		<< DIALOG_SPACER
		<< DIALOG_APOLOGY
		;

	const wxSizerFlags flagsFixed(SizerFlags(0));
	const wxSizerFlags flagsExpand(SizerFlags(1));
	const wxSizerFlags flagsExpand2(SizerFlags(2));

	wxSizer *sizerPreview = new wxStaticBoxSizer(wxVERTICAL, this, PROBLEM_TITLE);
	sizerPreview->Add(CreateTextSizer(msg), SizerFlags(0).Centre());

	// ... and the list of files in this debug report with buttons to view them
	wxSizer *sizerFileBtns = new wxBoxSizer(wxVERTICAL);
	sizerFileBtns->AddStretchSpacer(1);
	sizerFileBtns->Add(new wxButton(this, wxID_VIEW_DETAILS, _T("&View...")),
		wxSizerFlags().Border(wxBOTTOM));
	sizerFileBtns->Add(new wxButton(this, wxID_OPEN, _T("&Open...")),
		wxSizerFlags().Border(wxTOP));
	sizerFileBtns->AddStretchSpacer(1);

	m_checklst = new wxCheckListBox(this, wxID_ANY);

	wxSizer *sizerFiles = new wxBoxSizer(wxHORIZONTAL);
	sizerFiles->Add(m_checklst, flagsExpand);
	sizerFiles->Add(sizerFileBtns, flagsFixed);

	sizerPreview->Add(sizerFiles, flagsExpand2);


	// lower part of the dialog: notes field
	wxSizer *sizerNotes = new wxStaticBoxSizer(wxVERTICAL, this, _("&Notes:"));

	msg = DIALOG_ADDL_INFO;

	m_notes = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE);

	sizerNotes->Add(CreateTextSizer(msg), flagsFixed);
	sizerNotes->Add(m_notes, flagsExpand);


	wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);
	sizerTop->Add(sizerPreview, flagsExpand2);
	sizerTop->AddSpacer(5);
	sizerTop->Add(sizerNotes, flagsExpand);
	sizerTop->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), flagsFixed);

	SetSizerAndFit(sizerTop);
	Layout();
	CentreOnScreen();
}

// ----------------------------------------------------------------------------
// data exchange
// ----------------------------------------------------------------------------

bool wxDebugReportDialog::TransferDataToWindow()
{
	// all files are included in the report by default
	const size_t count = m_dbgrpt.GetFilesCount();
	for ( size_t n = 0; n < count; n++ )
	{
		wxString name, desc;
		if ( m_dbgrpt.GetFile(n, &name, &desc) )
		{
			m_checklst->Append(name + _T(" (") + desc + _T(')'));
			m_checklst->Check(n);

			m_files.Add(name);
		}
	}

	return true;
}

bool wxDebugReportDialog::TransferDataFromWindow()
{
	// any unchecked files should be removed from the report
	const size_t count = m_checklst->GetCount();
	for ( size_t n = 0; n < count; n++ )
	{
		if ( !m_checklst->IsChecked(n) )
		{
			m_dbgrpt.RemoveFile(m_files[n]);
		}
	}

	// if the user entered any notes, add them to the report
	const wxString notes = m_notes->GetValue();
	if ( !notes.empty() )
	{
		// for now filename fixed, could make it configurable in the future...
		m_dbgrpt.AddText(_T("notes.txt"), notes, _T("user notes"));
	}

	return true;
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

void wxDebugReportDialog::OnView(wxCommandEvent& )
{
	const int sel = m_checklst->GetSelection();
	wxCHECK_RET( sel != wxNOT_FOUND, _T("invalid selection in OnView()") );

	wxFileName fn(m_dbgrpt.GetDirectory(), m_files[sel]);
	wxString str;

	wxFFile file(fn.GetFullPath());
	if ( file.IsOpened() && file.ReadAll(&str) )
	{
		wxDumpPreviewDlg dlg(this, m_files[sel], str);
		dlg.ShowModal();
	}
}

void wxDebugReportDialog::OnOpen(wxCommandEvent& )
{
	const int sel = m_checklst->GetSelection();
	wxCHECK_RET( sel != wxNOT_FOUND, _T("invalid selection in OnOpen()") );

	wxFileName fn(m_dbgrpt.GetDirectory(), m_files[sel]);

	// try to get the command to open this kind of files ourselves
	wxString command;
#if wxUSE_MIMETYPE
	wxFileType *
		ft = wxTheMimeTypesManager->GetFileTypeFromExtension(fn.GetExt());
	if ( ft )
	{
		command = ft->GetOpenCommand(fn.GetFullPath());
		delete ft;
	}
#endif // wxUSE_MIMETYPE

	// if we couldn't, ask the user
	if ( command.empty() )
	{
		wxDumpOpenExternalDlg dlg(this, fn);
		if ( dlg.ShowModal() == wxID_OK )
		{
			// get the command chosen by the user and append file name to it

			// if we don't have place marker for file name in the command...
			wxString cmd = dlg.GetCommand();
			if ( !cmd.empty() )
			{
#if wxUSE_MIMETYPE
				if ( cmd.find(_T('%')) != wxString::npos )
				{
					command = wxFileType::ExpandCommand(cmd, fn.GetFullPath());
				}
				else // no %s nor %1
#endif // wxUSE_MIMETYPE
				{
					// append the file name to the end
					command << cmd << _T(" \"") << fn.GetFullPath() << _T('"');
				}
			}
		}
	}

	if ( !command.empty() )
		::wxExecute(command);
}

void wxDebugReportDialog::OnViewUpdate(wxUpdateUIEvent& event)
{
	int sel = m_checklst->GetSelection();
	if (sel >= 0)
	{
		wxFileName fn(m_dbgrpt.GetDirectory(), m_files[sel]);
		event.Enable(fn.FileExists());
	}
	else
		event.Enable(false);
}


// ============================================================================
// wxDebugReportPreviewStd implementation
// ============================================================================

bool wxDebugReportPreviewStd::Show(wxDebugReport& dbgrpt) const
{
	if ( !dbgrpt.GetFilesCount() )
		return false;

	wxDebugReportDialog dlg(dbgrpt);

#ifdef __WXMSW__
	// before entering the event loop (from ShowModal()), block the event
	// handling for all other windows as this could result in more crashes
	wxEventLoop::SetCriticalWindow(&dlg);
#endif // __WXMSW__

	return dlg.ShowModal() == wxID_OK && dbgrpt.GetFilesCount() != 0;
}












// ----------------------------------------------------------------------------
// custom debug reporting class
// ----------------------------------------------------------------------------

// this is your custom debug reporter: it will use curl program (which should
// be available) to upload the crash report to the given URL (which should be
// set up by you)
class MyDebugReport : public wxDebugReportUpload
{
public:
	MyDebugReport()
	: wxDebugReportUpload(_T("http://your.url.here/"), _T("report:file"), _T("action"))
	{
		//This would be the place to put the compressed file in a custom directory
		//but since the directory member is private, it would require pulling in yet
		//another wxWidget module to modify and I decided against it and take the
		//time and date based random location generator. (JAC - 4/16/07)
	}

protected:
	// this is called with the contents of the server response: you will
	// probably want to parse the XML document in OnServerReply() instead of
	// just dumping it as I do
	virtual bool OnServerReply(const wxArrayString& reply)
	{
		if ( reply.IsEmpty() )
		{
			wxLogError(_T("Didn't receive the expected server reply."));
			return false;
		}

		wxString s(_T("Server replied:\n"));

		const size_t count = reply.GetCount();
		for ( size_t n = 0; n < count; n++ )
		{
			s << _T('\t') << reply[n] << _T('\n');
		}

		wxLogMessage(_T("%s"), s.c_str());

		return true;
	}
	/**
	 * DoProcess: This is called by the wxWidgets Dialog when OK is clicked.
	 *				Since it overrides a virtual method we must make sure
	 *				that DoProcess() of the parent class is called.
	 *
	 * @return bool true if upload was successful, false otherwise.
	 **/
	virtual bool DoProcess()
	{
		//Call the DoProcess of the parent class and make sure it is successful.
		if ( !wxDebugReportCompress::DoProcess() )
			return false;

		//Shell execute the curl command with the appropriate arguments
		//for the custom application of this class:
		//
		//C:\curl-7.16.0\curl -v -F upfile=@"<the full path to the report file>" --trace trace.txt <the URL of the accepting FORM>
		//
		//The -v and the --trace trace.txt arguments are for debugging and are not required.
		wxArrayString output, errors;
		int rc = wxExecute(wxString::Format
			(
			_T("%s -v -F %s=@\"%s\" --trace trace.txt %s"),
			_T("C:\\curl-7.16.0\\curl"),
			_T("upfile"),
			GetCompressedFileName().c_str(),
			_T("http://kaxa.findhere.org/0ad/upload.php")
			),
			output,
			errors);
		//Check the result for errors and log them with wxWidgets.
		if ( rc == -1 )
		{
			wxLogError(_("Failed to execute curl, please install it in PATH."));
		}
		else if ( rc != 0 )
		{
			const size_t count = errors.GetCount();
			if ( count )
			{
				for ( size_t n = 0; n < count; n++ )
				{
					wxLogWarning(_T("%s"), errors[n].c_str());
				}
			}

			wxLogError(_("Failed to upload the debug report (error code %d)."), rc);
		}
		else // rc == 0
		{
			//this causes a crash and I have no incentive to debug it
			//if ( OnServerReply(output) )
			return true;
		}

		return false;
	}
};

// another possibility would be to build email library from contrib and use
// this class, after uncommenting it:
#if 0

#include "wx/net/email.h"

class MyDebugReport : public wxDebugReportCompress
{
public:
	virtual bool DoProcess()
	{
		if ( !wxDebugReportCompress::DoProcess() )
			return false;
		wxMailMessage msg(GetReportName() + _T(" crash report"),
						  _T("vadim@wxwindows.org"),
						  wxEmptyString, // mail body
						  wxEmptyString, // from address
						  GetCompressedFileName(),
						  _T("crashreport.zip"));

		return wxEmail::Send(msg);
	}
};

#endif // 0

// ----------------------------------------------------------------------------
// application class
// ----------------------------------------------------------------------------

bool m_uploadReport = true;

//m_generateReport = false;	//Flag to report back to wxWidgets that indicates
//whether the DoProcess() method should be called.
//GenerateReport(false);		//Create the compressed report file.
//The argument is not used in this version but
//is meant to indicate whether crash files already
//existed and the calling method did not create new ones.
//false means the crash files were recently generated...
//true means they already existed.

// this is where we really generate the debug report
//void GenerateReport(bool CrashFilesExist);


void GenerateReport()
{
	wxDebugReportCompress *report = m_uploadReport ? new MyDebugReport
												   : new wxDebugReportCompress;
	wxString fn1, fn2, fn3;
	fn1 = _T(LOGS_LOCATION);
	fn1 += _T("crashlog.dmp");
	fn2 = _T(LOGS_LOCATION);
	fn2 += _T("crashlog.txt");

	if(wxFileExists(fn1))
		report->AddFile(fn1, _T("memory dump"));
	else
		wxLogError(_T("crashlog.dmp not found!"));
	if(wxFileExists(fn2))
		report->AddFile(fn2, _T("debug information"));
	else
		wxLogError(_T("crashlog.txt not found!"));

	//Then call the built in wxWidgets dialog which is modified to be
	//customizable for each individual project and can be found in
	// *****dbgrptg.cpp*****
	bool m_generateReport = wxDebugReportPreviewStd().Show(*report);
	if ( m_generateReport )
	{	//User clicked OK
		if ( report->Process() )
		{
			if ( m_uploadReport )
			{
				wxLogMessage(DIALOG_UPLOAD);
			}
			else
			{
				wxLogMessage(_T("Report generated in \"%s\"."),
							 report->GetCompressedFileName().c_str());
				report->Reset();
			}
		}
		// remove all crashlog files whether or not upload was successful!!
		wxRemove(fn1);
		wxRemove(fn2);
	}
	//delete the compressed report file always!!
	delete report;
}

#endif
