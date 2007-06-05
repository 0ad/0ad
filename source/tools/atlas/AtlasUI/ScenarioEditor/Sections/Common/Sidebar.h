#ifndef INCLUDED_SIDEBAR
#define INCLUDED_SIDEBAR

class ScenarioEditor;

class Sidebar : public wxPanel
{
public:
	Sidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

	void OnSwitchAway();
	void OnSwitchTo();

	wxWindow* GetBottomBar() { return m_BottomBar; }

protected:
	ScenarioEditor& m_ScenarioEditor;

	wxSizer* m_MainSizer; // vertical box sizer, used by most sidebars

	wxWindow* m_BottomBar; // window that goes at the bottom of the screen; may be NULL

	virtual void OnFirstDisplay() {}
		// should be overridden when sidebars need to do expensive construction,
		// so it can be delayed until it is required;

private:
	bool m_AlreadyDisplayed;
};

#endif // INCLUDED_SIDEBAR
