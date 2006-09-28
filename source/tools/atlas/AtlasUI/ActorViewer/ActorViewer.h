#include "Windows/AtlasWindow.h"

#include "GameInterface/Messages.h"
#include "General/Observable.h"
#include "ScenarioEditor/Tools/Common/ObjectSettings.h"

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
	void OnToggleButton(wxCommandEvent& event);
	void OnBackgroundButton(wxCommandEvent& event);

	void OnActorEdited();
	ObservableScopedConnections m_ActorConns;

	wxTreeCtrl* m_TreeCtrl;
	wxComboBox* m_AnimationBox;
	wxString m_CurrentActor;
	float m_CurrentSpeed;

	Observable<std::vector<AtlasMessage::ObjectID> > m_ObjectSelection;
	Observable<ObjectSettings> m_ObjectSettings;
	wxColour m_BackgroundColour;
	bool m_ToggledWireframe, m_ToggledWalking, m_ToggledGround, m_ToggledShadows, m_ToggledStats;

	Observable<AtlasMessage::sEnvironmentSettings> m_EnvironmentSettings;
	ObservableScopedConnection m_EnvConn;

	DECLARE_EVENT_TABLE();
};
