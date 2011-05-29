/* Copyright (C) 2011 Wildfire Games.
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

#include "precompiled.h"

#include "ScenarioEditor/ScenarioEditor.h"
#include "Common/Tools.h"
#include "Common/Brushes.h"
#include "Common/MiscState.h"
#include "Common/ObjectSettings.h"
#include "GameInterface/Messages.h"

using AtlasMessage::Position;

// ObjectSidebar handles the display and animation etc of the actor;
// this tool mainly handles the user input and toggling the rendering.
// (There isn't a hugely clear separation of responsibilities.)

class ActorViewerTool : public StateDrivenTool<ActorViewerTool>
{
	DECLARE_DYNAMIC_CLASS(ActorViewerTool);

	// Camera settings
	float m_Distance;
	float m_Angle;
	float m_Elevation;

	// Mouse input state
	int m_LastX, m_LastY;
	bool m_LastIsValid;

public:
	ActorViewerTool() :
		m_Distance(20.f), m_Angle(0.f), m_Elevation((float)M_PI / 6.f),
		m_LastIsValid(false)
	{
	}

	void PostLookAt()
	{
		float offset = 0.3f; // slight fudge so we turn nicely when going over the top of the unit
		POST_MESSAGE(LookAt, (AtlasMessage::eRenderView::ACTOR,
			Position(
				m_Distance*cos(m_Elevation)*sin(m_Angle) + offset*cos(m_Angle),
				m_Distance*sin(m_Elevation),
				m_Distance*cos(m_Elevation)*cos(m_Angle) - offset*sin(m_Angle)),
			Position(0, 0, 0)));
	}

	virtual void Init(void* initData, ScenarioEditor* scenarioEditor)
	{
		StateDrivenTool<ActorViewerTool>::Init(initData, scenarioEditor);

		SetState(&Viewing);
	}

	void OnEnable()
	{
		GetScenarioEditor().GetObjectSettings().SetView(AtlasMessage::eRenderView::ACTOR);

		std::vector<AtlasMessage::ObjectID> selected;
		selected.push_back(0);
		g_SelectedObjects = selected;

		PostLookAt();

		POST_MESSAGE(RenderEnable, (AtlasMessage::eRenderView::ACTOR));
	}

	void OnDisable()
	{
		GetScenarioEditor().GetObjectSettings().SetView(AtlasMessage::eRenderView::GAME);

		std::vector<AtlasMessage::ObjectID> selected;
		g_SelectedObjects = selected;

		POST_MESSAGE(RenderEnable, (AtlasMessage::eRenderView::GAME));
	}

	struct sViewing : public State
	{
		bool OnMouse(ActorViewerTool* obj, wxMouseEvent& evt)
		{
			bool camera_changed = false;

			if (evt.GetWheelRotation())
			{
				float speed = -1.f * ScenarioEditor::GetSpeedModifier();

				obj->m_Distance += evt.GetWheelRotation() * speed / evt.GetWheelDelta();

				camera_changed = true;
			}

			if (evt.ButtonDown(wxMOUSE_BTN_LEFT) || evt.ButtonDown(wxMOUSE_BTN_RIGHT))
			{
				obj->m_LastX = evt.GetX();
				obj->m_LastY = evt.GetY();
				obj->m_LastIsValid = true;
			}
			else if (evt.Dragging()
				&& (evt.ButtonIsDown(wxMOUSE_BTN_LEFT) || evt.ButtonIsDown(wxMOUSE_BTN_RIGHT))
				&& obj->m_LastIsValid)
			{
				int dx = evt.GetX() - obj->m_LastX;
				int dy = evt.GetY() - obj->m_LastY;
				obj->m_LastX = evt.GetX();
				obj->m_LastY = evt.GetY();

				obj->m_Angle += dx * M_PI/256.f * ScenarioEditor::GetSpeedModifier();

				if (evt.ButtonIsDown(wxMOUSE_BTN_LEFT))
					obj->m_Distance += dy / 8.f * ScenarioEditor::GetSpeedModifier();
				else // evt.ButtonIsDown(wxMOUSE_BTN_RIGHT))
					obj->m_Elevation += dy * M_PI/256.f * ScenarioEditor::GetSpeedModifier();

				camera_changed = true;
			}
			else if ((evt.ButtonUp(wxMOUSE_BTN_LEFT) || evt.ButtonUp(wxMOUSE_BTN_RIGHT))
				&& !(evt.ButtonIsDown(wxMOUSE_BTN_LEFT) || evt.ButtonIsDown(wxMOUSE_BTN_RIGHT))
				)
			{
				// In some situations (e.g. double-clicking the title bar to
				// maximise the window) we get a dragging event without the matching
				// buttondown; so disallow dragging when all buttons were released since
				// the last buttondown.
				// (TODO: does this problem affect the scenario editor too?)
				obj->m_LastIsValid = false;
			}

			obj->m_Distance = std::max(obj->m_Distance, 1/64.f); // don't let people fly through the origin

			if (camera_changed)
				obj->PostLookAt();

			return true;
		}

		bool OnKey(ActorViewerTool* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_DOWN && (evt.GetKeyCode() >= '0' && evt.GetKeyCode() <= '9'))
			{
				// (TODO: this should probably be 'char' not 'down'; but we don't get
				// 'char' unless we return false from this function, in which case the
				// scenario editor intercepts some other keys for itself)
				int playerID = evt.GetKeyCode() - '0';
				obj->GetScenarioEditor().GetObjectSettings().SetPlayerID(playerID);
				obj->GetScenarioEditor().GetObjectSettings().NotifyObservers();
			}

			// Prevent keys from passing through to the scenario editor
			return true;
		}
	}
	Viewing;
};

IMPLEMENT_DYNAMIC_CLASS(ActorViewerTool, StateDrivenTool<ActorViewerTool>);
