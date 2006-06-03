#include "../Common/Sidebar.h"

class EnvironmentSidebar : public Sidebar
{
public:
	EnvironmentSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

protected:
	virtual void OnFirstDisplay();
};
