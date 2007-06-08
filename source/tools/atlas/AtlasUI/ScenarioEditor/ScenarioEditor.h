#ifndef INCLUDED_SCENARIOEDITOR
#define INCLUDED_SCENARIOEDITOR

#include "General/AtlasWindowCommandProc.h"
#include "CustomControls/FileHistory/FileHistory.h"
#include "SectionLayout.h"

class ScriptInterface;

class ScenarioEditor : public wxFrame
{
public:
	ScenarioEditor(wxWindow* parent, ScriptInterface& scriptInterface);
	void OnClose(wxCloseEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnIdle(wxIdleEvent& event);
	
// 	void OnNew(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnSaveAs(wxCommandEvent& event);
	void OnMRUFile(wxCommandEvent& event);

	void OnQuit(wxCommandEvent& event);
	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);

	void OnWireframe(wxCommandEvent& event);
	void OnMessageTrace(wxCommandEvent& event);
	void OnScreenshot(wxCommandEvent& event);
	void OnMediaPlayer(wxCommandEvent& event);
	void OnJavaScript(wxCommandEvent& event);

	void OpenFile(const wxString& name);

	static AtlasWindowCommandProc& GetCommandProc();

	static float GetSpeedModifier();

	ScriptInterface& GetScriptInterface() const { return m_ScriptInterface; }

private:
	ScriptInterface& m_ScriptInterface;

	wxTimer m_Timer;

	SectionLayout m_SectionLayout;

	void SetOpenFilename(const wxString& filename);
	wxString m_OpenFilename;
	FileHistory m_FileHistory;

	DECLARE_EVENT_TABLE();
};

#endif // INCLUDED_SCENARIOEDITOR
