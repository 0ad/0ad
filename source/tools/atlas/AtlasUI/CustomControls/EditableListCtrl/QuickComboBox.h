class QuickComboBox : public wxComboBox
{
public:
	QuickComboBox(wxWindow* parent, wxRect& location, const wxArrayString& choices, const wxValidator& validator = wxDefaultValidator);

	void OnKillFocus(wxFocusEvent& event);
	void OnChar(wxKeyEvent& event);

private:
	DECLARE_EVENT_TABLE();
};
