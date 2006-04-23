#include "../Common/Sidebar.h"

class TerrainSidebar : public Sidebar
{
public:
	TerrainSidebar(wxWindow* parent);
	wxWindow* GetBottomBar(wxWindow* parent);

private:
	wxWindow* m_BottomBar;
};

class TerrainBottomBar : public wxPanel
{
public:
	TerrainBottomBar(wxWindow* parent);
private:
};
