#include "../Common/Sidebar.h"

class TerrainSidebar : public Sidebar
{
public:
	TerrainSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

protected:
	virtual void OnFirstDisplay();
};
