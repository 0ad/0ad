//#include "wx/tglbtn.h"

#include <map>

class ITool;
class SectionLayout;
class ToolManager;

class ToolButton : public wxButton
{
public:
	ToolButton(ToolManager& toolManager, wxWindow *parent, const wxString& label, const wxString& toolName, const wxSize& size = wxDefaultSize, long style = 0);

	void SetSelectedAppearance(bool selected);

protected:
	void OnClick(wxCommandEvent& evt);

private:
	ToolManager& m_ToolManager;
	wxString m_Tool;
	bool m_Selected;

	DECLARE_EVENT_TABLE();
};

class ToolButtonBar : public wxToolBar
{
public:
	ToolButtonBar(ToolManager& toolManager, wxWindow* parent, SectionLayout* sectionLayout, int baseID);
	void AddToolButton(const wxString& shortLabel, const wxString& longLabel,
		const wxString& iconPNGFilename, const wxString& toolName, const wxString& sectionPage);

protected:
	void OnTool(wxCommandEvent& evt);

private:
	ToolManager& m_ToolManager;
	int m_Id;
	int m_Size;
	struct Button
	{
		Button() {}
		Button(const wxString& name, const wxString& sectionPage) : name(name), sectionPage(sectionPage) {}
		wxString name;
		wxString sectionPage;
	};
	std::map<int, Button> m_Buttons;
	SectionLayout* m_SectionLayout;

	DECLARE_EVENT_TABLE();
};
