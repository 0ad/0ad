/* Copyright (C) 2019 Wildfire Games.
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

static float g_DefaultAngle = (float)(M_PI*3.0/4.0);

class PlaceObject : public StateDrivenTool<PlaceObject>
{
	DECLARE_DYNAMIC_CLASS(PlaceObject);

	Position m_ScreenPos, m_ObjPos, m_Target;
	wxString m_ObjectID;
	unsigned int m_ActorSeed;
	int m_RotationDirection;

public:
	PlaceObject(): m_RotationDirection(0)
	{
		SetState(&Waiting);
	}

	void SendObjectMsg(bool preview)
	{
		int dragDistSq =
			  (m_ScreenPos.type1.x-m_Target.type1.x)*(m_ScreenPos.type1.x-m_Target.type1.x)
			+ (m_ScreenPos.type1.y-m_Target.type1.y)*(m_ScreenPos.type1.y-m_Target.type1.y);
		bool useTarget = (dragDistSq >= 16*16);
		if (preview)
			POST_MESSAGE(ObjectPreview, ((std::wstring)m_ObjectID.wc_str(), GetScenarioEditor().GetObjectSettings().GetSettings(), m_ObjPos, useTarget, m_Target, g_DefaultAngle, m_ActorSeed, true));
		else
		{
			POST_COMMAND(CreateObject, ((std::wstring)m_ObjectID.wc_str(), GetScenarioEditor().GetObjectSettings().GetSettings(), m_ObjPos, useTarget, m_Target, g_DefaultAngle, m_ActorSeed));
			RandomizeActorSeed();
		}
	}

	virtual void Init(void* initData, ScenarioEditor* scenarioEditor)
	{
		StateDrivenTool<PlaceObject>::Init(initData, scenarioEditor);

		wxASSERT(initData);
		wxString& id = *static_cast<wxString*>(initData);
		m_ObjectID = id;
		SendObjectMsg(true);
	}

	void OnEnable()
	{
		RandomizeActorSeed();
	}

	void OnDisable()
	{
		m_ObjectID = _T("");
		SendObjectMsg(true);
	}

	/*
		Object placement:
		* Select unit from list
		* Move mouse around screen; preview of unit follows mouse
		* Left mouse down -> remember position, fix preview to point
		* Mouse move -> if moved > [limit], rotate unit to face mouse; else default orientation
		* Left mouse release -> finalise placement of object on map

		* Page up/down -> rotate default orientation

		* Escape -> cancel placement tool

		TOOD: what happens if somebody saves while the preview is active?
	*/

	bool OnMouseOverride(wxMouseEvent& WXUNUSED(evt))
	{
		// This used to let the scroll-wheel rotate units, but that overrides
		// the camera zoom and makes navigation very awkward, so it doesn't
		// any more.
		return false;
	}

	bool OnKeyOverride(wxKeyEvent& evt, KeyEventType type)
	{
		if (type == KEY_CHAR && evt.GetKeyCode() == WXK_ESCAPE)
		{
			SetState(&Disabled);
			return true;
		}
		else if (evt.GetKeyCode() == WXK_PAGEDOWN)
		{
			if (type == KEY_DOWN)
				m_RotationDirection = 1;
			else if (type == KEY_UP)
				m_RotationDirection = 0;
			else
				return false;
			return true;
		}
		else if (evt.GetKeyCode() == WXK_PAGEUP)
		{
			if (type == KEY_DOWN)
				m_RotationDirection = -1;
			else if (type == KEY_UP)
				m_RotationDirection = 0;
			else
				return false;
			return true;
		}
		else
			return false;
	}

	void RotateTick(float dt)
	{
		if (m_RotationDirection)
		{
			float speed = M_PI/2.f * ScenarioEditor::GetSpeedModifier(); // radians per second
			g_DefaultAngle += (m_RotationDirection * dt * speed);
			SendObjectMsg(true);
		}
	}

	void RandomizeActorSeed()
	{
		m_ActorSeed = (unsigned int)floor((rand() / (float)RAND_MAX) * 65535.f);
	}

	struct sWaiting : public State
	{
		bool OnMouse(PlaceObject* obj, wxMouseEvent& evt)
		{
			if (obj->OnMouseOverride(evt))
				return true;
			else if (evt.LeftDown())
			{
				obj->m_ObjPos = obj->m_ScreenPos = obj->m_Target = Position(evt.GetPosition());
				obj->SendObjectMsg(true);
				obj->m_ObjPos = Position::Unchanged(); // make sure object is stationary even if the camera moves
				SET_STATE(Placing);
				return true;
			}
			else if (evt.Moving())
			{
				obj->m_ObjPos = obj->m_ScreenPos = obj->m_Target = Position(evt.GetPosition());
				obj->SendObjectMsg(true);
				return true;
			}
			else
				return false;
		}
		bool OnKey(PlaceObject* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_CHAR && (evt.GetKeyCode() >= '0' && evt.GetKeyCode() <= '9'))
			{
				int playerID = evt.GetKeyCode() - '0';
				obj->GetScenarioEditor().GetObjectSettings().SetPlayerID(playerID);
				obj->GetScenarioEditor().GetObjectSettings().NotifyObservers();
				obj->SendObjectMsg(true);
				return true;
			}
			else
				return obj->OnKeyOverride(evt, type);
		}
		void OnTick(PlaceObject* obj, float dt)
		{
			obj->RotateTick(dt);
		}
	}
	Waiting;

	struct sPlacing : public State
	{
		bool OnMouse(PlaceObject* obj, wxMouseEvent& evt)
		{
			if (obj->OnMouseOverride(evt))
				return true;
			else if (evt.LeftUp())
			{
				obj->m_Target = Position(evt.GetPosition());
				// Create the actual object
				obj->SendObjectMsg(false);
				// Go back to preview mode
				SET_STATE(Waiting);
				obj->m_ObjPos = obj->m_ScreenPos = obj->m_Target;
				obj->SendObjectMsg(true);
				return true;
			}
			else if (evt.Dragging())
			{
				obj->m_Target = Position(evt.GetPosition());
				obj->SendObjectMsg(true);
				return true;
			}
			else
				return false;
		}
		bool OnKey(PlaceObject* obj, wxKeyEvent& evt, KeyEventType type)
		{
			return obj->OnKeyOverride(evt, type);
		}
		void OnTick(PlaceObject* obj, float dt)
		{
			obj->RotateTick(dt);
		}
	}
	Placing;
};

IMPLEMENT_DYNAMIC_CLASS(PlaceObject, StateDrivenTool<PlaceObject>);
