#ifndef SIDEBAR_H__
#define SIDEBAR_H__

class Sidebar : public wxPanel
{
	DECLARE_DYNAMIC_CLASS(Sidebar);

public:
	Sidebar() {}
	Sidebar(wxWindow* parent);

	void OnSwitchTo();

	virtual wxWindow* GetBottomBar(wxWindow* parent);
		// called whenever the bottom bar is made visible; should usually be
		// lazily constructed, then cached forever (to maximise responsiveness)

protected:
	wxSizer* m_MainSizer; // vertical box sizer, used by most sidebars

	virtual void OnFirstDisplay() {}
		// should be overridden when sidebars need to do expensive construction,
		// so it can be delayed until it is required

private:
	bool m_AlreadyDisplayed;
};

#endif // SIDEBAR_H__
