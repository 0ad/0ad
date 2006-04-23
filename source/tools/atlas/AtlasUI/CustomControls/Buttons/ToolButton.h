//#include "wx/tglbtn.h"

class ITool;

class ToolButton : public wxButton
{
public:
	ToolButton(wxWindow *parent, const wxString& label, const wxString& toolName, const wxSize& size = wxDefaultSize, long style = 0);

	void SetSelectedAppearance(bool selected);

protected:
	void OnClick(wxCommandEvent& evt);

private:
	wxString m_Tool;
	bool m_Selected;

	DECLARE_EVENT_TABLE();
};

class ToolButtonBar : public wxToolBar
{
public:
	ToolButtonBar(wxWindow* parent, int baseID);
	void AddToolButton(const wxString& shortLabel, const wxString& longLabel, const wxString& iconPNGFilename, const wxString& toolName);

protected:
	void OnTool(wxCommandEvent& evt);

private:
	int m_Id;
	int m_Size;
	std::map<int, wxString> m_Buttons;

	DECLARE_EVENT_TABLE();
};