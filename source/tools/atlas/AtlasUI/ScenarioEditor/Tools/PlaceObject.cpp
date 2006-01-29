#include "stdafx.h"

#include "Common/Tools.h"
#include "Common/Brushes.h"
#include "Common/MiscState.h"
#include "GameInterface/Messages.h"

using AtlasMessage::Position;

static float g_DefaultAngle = M_PI*3.0/4.0;

class PlaceObject : public StateDrivenTool<PlaceObject>
{
	DECLARE_DYNAMIC_CLASS(PlaceObject);

	Position m_ScreenPos, m_ObjPos, m_Target;
	wxString m_ObjectID;
	static int m_Player;

public:
	PlaceObject()
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
			POST_MESSAGE(ObjectPreview(m_ObjectID.c_str(), 0, m_ObjPos, useTarget, m_Target, g_DefaultAngle));
		else
			POST_COMMAND(CreateObject,(m_ObjectID.c_str(), m_Player, m_ObjPos, useTarget, m_Target, g_DefaultAngle));
	}

	virtual void Init(void* initData)
	{
		wxASSERT(initData);
		wxString& id = *static_cast<wxString*>(initData);
		m_ObjectID = id;
		SendObjectMsg(true);
	}

	void OnEnable()
	{
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

	bool OnKeyOverride(wxKeyEvent& evt, KeyEventType dir)
	{
		switch (dir)
		{
		case KEY_CHAR:
			int key = evt.GetKeyCode();
			if (key == WXK_ESCAPE)
			{
				SetState(&Disabled);
				return true;
			}
			else if(key >= '0' && key <= '8') {
				m_Player = key - '0';
			}
			break;
		}
		return false;
	}

	void RotateTick(float dt)
	{
		int dir = 0;
		if (wxGetKeyState(WXK_NEXT))  ++dir; // page-down key
		if (wxGetKeyState(WXK_PRIOR)) --dir; // page-up key
		if (dir)
		{
			float speed = M_PI/2.f; // radians per second
			if (wxGetKeyState(WXK_SHIFT) && wxGetKeyState(WXK_CONTROL))
				speed /= 64.f;
			else if (wxGetKeyState(WXK_CONTROL))
				speed /= 4.f;
			else if (wxGetKeyState(WXK_SHIFT))
				speed *= 4.f;
			g_DefaultAngle += (dir * dt * speed);
			SendObjectMsg(true);
		}
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
		bool OnKey(PlaceObject* obj, wxKeyEvent& evt, KeyEventType dir)
		{
			return obj->OnKeyOverride(evt, dir);
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
		bool OnKey(PlaceObject* obj, wxKeyEvent& evt, KeyEventType dir)
		{
			return obj->OnKeyOverride(evt, dir);
		}
		void OnTick(PlaceObject* obj, float dt)
		{
			obj->RotateTick(dt);
		}
	}
	Placing;
};

IMPLEMENT_DYNAMIC_CLASS(PlaceObject, StateDrivenTool<PlaceObject>);

int PlaceObject::m_Player = 1;
