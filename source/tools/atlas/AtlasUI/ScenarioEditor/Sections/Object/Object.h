#include "../Common/Sidebar.h"

struct ObjectSidebarImpl;
class ObjectSidebar : public Sidebar
{
public:
	ObjectSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer);
	~ObjectSidebar();
	void SetObjectFilter(int type);

protected:
	void OnFirstDisplay();

private:
	ObjectSidebarImpl* p;
};

class ObjectBottomBar : public wxPanel
{
public:
	ObjectBottomBar(wxWindow* parent);
private:
};
