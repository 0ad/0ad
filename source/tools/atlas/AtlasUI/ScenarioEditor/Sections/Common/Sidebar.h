#ifndef INCLUDED_SIDEBAR
#define INCLUDED_SIDEBAR

class Sidebar : public wxPanel
{
	DECLARE_DYNAMIC_CLASS(Sidebar);

public:
	Sidebar() {}
	Sidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

	void OnSwitchAway();
	void OnSwitchTo();

	wxWindow* GetBottomBar() { return m_BottomBar; }

protected:
	wxSizer* m_MainSizer; // vertical box sizer, used by most sidebars

	wxWindow* m_BottomBar; // window that goes at the bottom of the screen; may be NULL

	virtual void OnFirstDisplay() {}
		// should be overridden when sidebars need to do expensive construction,
		// so it can be delayed until it is required;

private:
	bool m_AlreadyDisplayed;
};

#endif // INCLUDED_SIDEBAR
