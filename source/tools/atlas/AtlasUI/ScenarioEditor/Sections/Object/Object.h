#include "../Common/Sidebar.h"

class ObjectSidebar : public Sidebar
{
public:
	ObjectSidebar(wxWindow* parent);
	wxWindow* GetBottomBar(wxWindow* parent);

protected:
	void OnFirstDisplay();

private:
	wxWindow* m_BottomBar;
	wxListBox* m_ObjectListBox;
};

class ObjectBottomBar : public wxPanel
{
public:
	ObjectBottomBar(wxWindow* parent);
private:
};
