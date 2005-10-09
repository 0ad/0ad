#ifndef SCENARIOEDITOR_H__
#define SCENARIOEDITOR_H__

#include "General/AtlasWindowCommandProc.h"

class ScenarioEditor : public wxFrame
{
public:
	ScenarioEditor(wxWindow* parent);
	void OnClose(wxCloseEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnIdle(wxIdleEvent& event);
	
	void OnQuit(wxCommandEvent& event);
	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);

	void OnWireframe(wxCommandEvent& event);
	void OnMessageTrace(wxCommandEvent& event);

	static AtlasWindowCommandProc& GetCommandProc();

private:
	wxTimer m_Timer;

	DECLARE_EVENT_TABLE();
};

#endif // SCENARIOEDITOR_H__
