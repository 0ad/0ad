//#include "wx/tglbtn.h"

class ITool;

class ToolButton : public wxButton
{
public:
	static ToolButton* g_Current;

	ToolButton(wxWindow *parent,
	           const wxString& label,
			   const wxString& toolName,
	           const wxSize& size = wxDefaultSize,
	           long style = 0)
		: wxButton(parent, wxID_ANY, label, wxDefaultPosition, size, style),
		m_Tool(toolName)
	{
		SetSelectedAppearance(false);
	}

protected:
	void OnClick(wxCommandEvent& evt);

	void SetSelectedAppearance(bool selected);

private:
	wxString m_Tool;

	DECLARE_EVENT_TABLE();
};
