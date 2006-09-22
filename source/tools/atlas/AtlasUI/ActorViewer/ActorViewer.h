#include "Windows/AtlasWindow.h"

#include "GameInterface/Messages.h"
#include "General/Observable.h"

class wxTreeCtrl;

class ActorViewer : public wxFrame
{
public:
	ActorViewer(wxWindow* parent);

private:
	void SetActorView(bool flushCache = false);
	void OnClose(wxCloseEvent& event);
	void OnTreeSelection(wxTreeEvent& event);
	void OnAnimationSelection(wxCommandEvent& event);
	void OnSpeedButton(wxCommandEvent& event);
	void OnEditButton(wxCommandEvent& event);

	void OnActorEdited();
	ObservableScopedConnections m_ActorConns;

	wxTreeCtrl* m_TreeCtrl;
	wxComboBox* m_AnimationBox;
	wxString m_CurrentActor;
	float m_CurrentSpeed;

	Observable<AtlasMessage::sEnvironmentSettings> m_EnvironmentSettings;
	ObservableScopedConnection m_EnvConn;

	DECLARE_EVENT_TABLE();
};
