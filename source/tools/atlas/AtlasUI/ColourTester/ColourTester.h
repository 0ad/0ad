class ColourTesterImageCtrl;

DECLARE_EVENT_TYPE(wxEVT_MY_IMAGE_CHANGED, -1)

class ColourTester : public wxFrame
{
public:
	ColourTester(wxWindow* parent);

private:
	void OnImageChanged(wxCommandEvent& event);

	ColourTesterImageCtrl* m_ImageCtrl;

	wxStatusBar* m_StatusBar;

	DECLARE_EVENT_TABLE();
};
