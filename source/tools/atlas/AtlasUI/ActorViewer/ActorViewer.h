/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Windows/AtlasWindow.h"

#include "GameInterface/Messages.h"
#include "General/Observable.h"
#include "ScenarioEditor/Tools/Common/ObjectSettings.h"

#include "wx/treectrl.h"

class wxTreeCtrl;
class wxTreeEvent;
class ScriptInterface;

class ActorViewer : public wxFrame
{
public:
	ActorViewer(wxWindow* parent, ScriptInterface& scriptInterface);

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

	ScriptInterface& m_ScriptInterface;

	Observable<std::vector<AtlasMessage::ObjectID> > m_ObjectSelection;
	ObjectSettings m_ObjectSettings;
	wxColour m_BackgroundColour;
	bool m_ToggledWireframe, m_ToggledWalking, m_ToggledGround, m_ToggledShadows, m_ToggledStats;

	Observable<AtlasMessage::sEnvironmentSettings> m_EnvironmentSettings;
	ObservableScopedConnection m_EnvConn;

	DECLARE_EVENT_TABLE();
};
