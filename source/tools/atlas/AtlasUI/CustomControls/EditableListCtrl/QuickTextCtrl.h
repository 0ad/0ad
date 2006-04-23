class QuickTextCtrl : public wxTextCtrl
{
public:
	QuickTextCtrl(wxWindow* parent, wxRect& location, const wxValidator& validator = wxDefaultValidator);

	void OnKillFocus(wxFocusEvent& event);
	void OnChar(wxKeyEvent& event);

private:
	DECLARE_EVENT_TABLE();
};
