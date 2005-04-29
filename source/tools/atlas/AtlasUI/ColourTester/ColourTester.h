class ColourTesterImageCtrl;

class ColourTester : public wxFrame
{
public:
	ColourTester(wxWindow* parent);

private:
	ColourTesterImageCtrl* m_ImageCtrl;

	DECLARE_EVENT_TABLE();
};
