#include "../Common/Sidebar.h"

#include "General/Observable.h"

class VariableListBox;

class EnvironmentSidebar : public Sidebar
{
public:
	EnvironmentSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

protected:
	virtual void OnFirstDisplay();

private:
	VariableListBox* m_SkyList;
	ObservableScopedConnection m_Conn;
};
