#include "wx/panel.h"

class QuickFileCtrl : public wxPanel
{
	DECLARE_DYNAMIC_CLASS(QuickFileCtrl);

public:
	QuickFileCtrl() {};
	QuickFileCtrl(wxWindow* parent, wxRect& location,
					const wxString& rootDir, const wxString& fileMask,
					wxString& rememberedDir,
					const wxValidator& validator = wxDefaultValidator);

	void OnKillFocus();

//private: // (or *theoretically* private)
	wxTextCtrl* m_TextCtrl;
	wxButton* m_ButtonBrowse;
	bool m_DisableKillFocus;

	wxString* m_RememberedDir; // can't be wxString&, because DYNAMIC_CLASSes need default constructors, and there's no suitable string to store in here...
};
