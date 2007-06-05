#include "../Common/Sidebar.h"

struct ObjectSidebarImpl;
class ObjectSidebar : public Sidebar
{
public:
	ObjectSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer);
	~ObjectSidebar();
	void SetObjectFilter(int type);

protected:
	virtual void OnFirstDisplay();

private:
	ObjectSidebarImpl* p;
};
