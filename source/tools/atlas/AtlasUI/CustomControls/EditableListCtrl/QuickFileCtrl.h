class QuickFileCtrl : public wxPanel
{
	DECLARE_DYNAMIC_CLASS(QuickFileCtrl);

public:
	QuickFileCtrl() {};
	QuickFileCtrl(wxWindow* parent, wxRect& location,
					const wxString& rootDir, const wxString& fileMask,
					const wxValidator& validator = wxDefaultValidator);

	void OnKillFocus();

//private: // (or *theoretically* private)
	wxTextCtrl* m_TextCtrl;
	wxButton* m_ButtonBrowse;
	bool m_DisableKillFocus;
};
