#include "precompiled.h"

#include "QuickFileCtrl.h"

#include "General/Datafile.h"

#include "wx/filename.h"

const int verticalPadding = 2;

//////////////////////////////////////////////////////////////////////////

// Unpleasant hack: It's necessary to keep focus inside the QuickFileCtrl;
// but since that's made of lots of controls, they each need to detect focus
// changes, and compare the focus-recipient to every control inside the group,
// to tell whether focus is leaving.
//
// (This solution doesn't work very well, and I'd like to find a better
// way of doing it...)
//
// So, create some unexciting classes:

class FileCtrl_TextCtrl : public wxTextCtrl
{
public:
	FileCtrl_TextCtrl(wxWindow* pParent, wxWindowID vId, const wxString& rsValue = wxEmptyString, const wxPoint& rPos = wxDefaultPosition, const wxSize& rSize = wxDefaultSize, long lStyle = 0, const wxValidator& rValidator = wxDefaultValidator, const wxString& rsName = wxTextCtrlNameStr)
		: wxTextCtrl(pParent, vId, rsValue, rPos, rSize, lStyle, rValidator, rsName)
	{}

	void OnKillFocus(wxFocusEvent& WXUNUSED(event))
	{
		wxWindow* focused = wxWindow::FindFocus();
		QuickFileCtrl* parent = wxDynamicCast(GetParent(), QuickFileCtrl);
		wxASSERT(parent);
		if (! (focused == parent->m_TextCtrl || focused == parent->m_ButtonBrowse))
			parent->OnKillFocus();
	}

	void OnChar(wxKeyEvent& event)
	{
		QuickFileCtrl* parent = wxDynamicCast(GetParent(), QuickFileCtrl);
		wxASSERT(parent);
		if (event.GetKeyCode() == WXK_RETURN)
			parent->OnKillFocus();
		else if (event.GetKeyCode() == WXK_ESCAPE)
			parent->Destroy();
		else
			event.Skip();
	}

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(FileCtrl_TextCtrl, wxTextCtrl)
	EVT_KILL_FOCUS(FileCtrl_TextCtrl::OnKillFocus)
	EVT_CHAR(FileCtrl_TextCtrl::OnChar)
END_EVENT_TABLE()

class FileCtrl_Button : public wxButton
{
public:
	FileCtrl_Button(wxWindow* pParent, wxWindowID vId, const wxString& rsValue = wxEmptyString, const wxPoint& rPos = wxDefaultPosition, const wxSize& rSize = wxDefaultSize, long lStyle = 0, const wxValidator& rValidator = wxDefaultValidator, const wxString& rsName = wxTextCtrlNameStr)
		: wxButton(pParent, vId, rsValue, rPos, rSize, lStyle, rValidator, rsName)
	{}

	void OnKillFocus(wxFocusEvent& WXUNUSED(event))
	{
		wxWindow* focused = wxWindow::FindFocus();
		QuickFileCtrl* parent = wxDynamicCast(GetParent(), QuickFileCtrl);
		wxASSERT(parent);
		if (! (focused == parent->m_TextCtrl || focused == parent->m_ButtonBrowse))
			parent->OnKillFocus();
	}

	virtual void OnPress(wxCommandEvent& event)=0;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(FileCtrl_Button, wxButton)
	EVT_KILL_FOCUS(FileCtrl_Button::OnKillFocus)
	EVT_BUTTON(wxID_ANY, FileCtrl_Button::OnPress)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

class FileCtrl_Button_Browse : public FileCtrl_Button
{
public:
	FileCtrl_Button_Browse(wxWindow* parent, const wxPoint& pos, const wxSize& size,
		long style, const wxString& rootDir, const wxString& fileMask)
	: FileCtrl_Button(parent, wxID_ANY, _("&Browse..."), pos, size, style),
		m_RootDir(rootDir), m_FileMask(fileMask)
	{
	}

	void OnPress(wxCommandEvent& WXUNUSED(event))
	{
		QuickFileCtrl* parent = wxDynamicCast(GetParent(), QuickFileCtrl);
		wxASSERT(parent);

		wxString defaultDir, defaultFile;

		wxFileName oldFilename (parent->m_TextCtrl->GetValue()); // TODO: use wxPATH_UNIX?
		if (oldFilename.IsOk())
		{
			// XXX: this assumes the file was in the 'public' mod, which is
			// often untrue and will quite annoy users
			wxFileName path (_T("mods/public/") + m_RootDir);
			wxASSERT(path.IsOk());
			path.MakeAbsolute(Datafile::GetDataDirectory());
			wxASSERT(path.IsOk());

			oldFilename.MakeAbsolute(path.GetPath());
			defaultDir = oldFilename.GetPath();
			defaultFile = oldFilename.GetFullName();
		}
		else
		{
			defaultDir = *parent->m_RememberedDir;
			defaultFile = wxEmptyString;
		}

		wxFileDialog dlg(this, _("Choose a file"), defaultDir, defaultFile, m_FileMask, wxOPEN);

		parent->m_DisableKillFocus = true;
		int ret = dlg.ShowModal();
		parent->m_DisableKillFocus = false;

		if (ret != wxID_OK)
			return;

		wxFileName filename (dlg.GetPath());
		*parent->m_RememberedDir = filename.GetPath();

		// The file should be in ".../mods/*/$m_RootDir/..." for some unknown '*'
		// So assume it is, and just find the substring after the m_RootDir
		// (XXX: This is pretty bogus because of case-insensitive filenames etc)
		wxString filenameStr (filename.GetFullPath(wxPATH_UNIX));
		int idx = filenameStr.Find(m_RootDir);
		if (idx < 0)
		{
			wxLogError(_T("Selected file (%s) must be in a \"%s\" directory"), filenameStr.c_str(), m_RootDir.c_str());
			return;
		}
		wxString filenameRel (filenameStr.Mid(idx + m_RootDir.Len()));
		parent->m_TextCtrl->SetValue(filenameRel);
	}

private:
	wxString m_RootDir;
	wxString m_FileMask;
};

//////////////////////////////////////////////////////////////////////////


IMPLEMENT_DYNAMIC_CLASS(QuickFileCtrl, wxPanel);

QuickFileCtrl::QuickFileCtrl(wxWindow* parent,
							 wxRect& location,
							 const wxString& rootDir,
							 const wxString& fileMask,
							 wxString& rememberedDir,
							 const wxValidator& validator)
 : wxPanel(parent, wxID_ANY,
	location.GetPosition()-wxPoint(0,verticalPadding), wxDefaultSize,
	wxNO_BORDER),
	m_DisableKillFocus(false), m_RememberedDir(&rememberedDir)
{
	wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);

	m_TextCtrl = new FileCtrl_TextCtrl(this, wxID_ANY, wxEmptyString,
		wxDefaultPosition,
		location.GetSize()+wxSize(0,verticalPadding*2),
		wxSUNKEN_BORDER,
		validator);

	m_ButtonBrowse = new FileCtrl_Button_Browse(this,
		// Position the button just below the text control
		wxPoint(0, location.GetSize().GetHeight()+verticalPadding),
		wxDefaultSize, wxBU_EXACTFIT,
		rootDir, fileMask);

	s->Add(m_TextCtrl);
	s->Add(m_ButtonBrowse);
	SetSizer(s);
	s->SetSizeHints(this);

	m_DisableKillFocus = true; // yucky
	m_TextCtrl->GetValidator()->TransferToWindow();
	m_TextCtrl->SetFocus();
	m_TextCtrl->SetSelection(-1, -1);
	m_DisableKillFocus = false;
	m_TextCtrl->SetFocus();
}

void QuickFileCtrl::OnKillFocus()
{
	// (This is always called explicitly, never by the wx event system)

	// Another unpleasant hack: TransferFromWindow happens to shift focus,
	// which ends up with OnKillFocus being called again, so it ends up
	// trying to destroy the window twice. So, only allow that to happen once:
	if (m_DisableKillFocus) return;
	m_DisableKillFocus = true;

	m_TextCtrl->GetValidator()->TransferFromWindow();
	Destroy();
}
