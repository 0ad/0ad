#include "../Common/Sidebar.h"

class MapSidebar : public Sidebar
{
public:
	MapSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

private:
	void GenerateMap(wxCommandEvent& WXUNUSED(event));
	void GenerateRMS(wxCommandEvent& WXUNUSED(event));

	wxTextCtrl* m_RMSText;

	DECLARE_EVENT_TABLE();
};
