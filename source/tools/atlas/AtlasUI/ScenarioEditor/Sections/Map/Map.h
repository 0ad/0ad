#include "../Common/Sidebar.h"

class MapSidebar : public Sidebar
{
public:
	MapSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

private:
	void GenerateMap(wxCommandEvent& event);
	void GenerateRMS(wxCommandEvent& event);

	wxTextCtrl* m_RMSText;

	void OnSimPlay(wxCommandEvent& event);
	void OnSimPause(wxCommandEvent& event);
	void OnSimReset(wxCommandEvent& event);
	void UpdateSimButtons();

	int m_SimState;

	DECLARE_EVENT_TABLE();
};
