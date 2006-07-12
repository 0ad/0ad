#include "../Common/Sidebar.h"

#include "General/Observable.h"

class VariableListBox;

class EnvironmentSidebar : public Sidebar
{
public:
	EnvironmentSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

protected:
	virtual void OnFirstDisplay();

private:
	VariableListBox* m_SkyList;
	ObservableConnection m_Conn;
};
