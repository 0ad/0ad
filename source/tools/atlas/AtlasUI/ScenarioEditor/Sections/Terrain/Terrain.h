#include "../Common/Sidebar.h"

class TerrainSidebar : public Sidebar
{
public:
	TerrainSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

protected:
	virtual void OnFirstDisplay();
};
