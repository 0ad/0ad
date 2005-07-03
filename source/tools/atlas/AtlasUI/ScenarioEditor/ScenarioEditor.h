class ScenarioEditor : public wxFrame
{
public:
	ScenarioEditor();
	void OnClose(wxCloseEvent& evt);
	void OnTimer(wxTimerEvent& evt);

private:
	wxTimer m_Timer;

	DECLARE_EVENT_TABLE();
};
