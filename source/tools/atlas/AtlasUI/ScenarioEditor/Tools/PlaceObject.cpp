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
	wxString m_ObjectName;

public:
	PlaceObject()
	{
		SetState(&Waiting);
	}

	void SendPreviewCommand()
	{
		int dragDistSq =
			  (m_ScreenPos.type1.x-m_Target.type1.x)*(m_ScreenPos.type1.x-m_Target.type1.x)
			+ (m_ScreenPos.type1.y-m_Target.type1.y)*(m_ScreenPos.type1.y-m_Target.type1.y);
		bool useTarget = (dragDistSq >= 8*8);
		POST_MESSAGE(EntityPreview(m_ObjectName.c_str(), m_ObjPos, useTarget, m_Target, g_DefaultAngle));
	}

	virtual void Init(void* initData)
	{
		wxASSERT(initData);
		wxString& name = *static_cast<wxString*>(initData);
		m_ObjectName = name;
		SendPreviewCommand();
	}

	void OnEnable()
	{
	}

	void OnDisable()
	{
		m_ObjectName = _T("");
		SendPreviewCommand();
	}

	/*
	Object placement:
	* Select unit from list
	* Move mouse around screen; preview of unit follows mouse
	* Left mouse down -> remember position, fix preview to point
	* Mouse move -> if moved > 8px, rotate unit to face mouse; else default orientation
	* Left mouse release -> finalise placement of object on map

	* Scroll wheel -> rotate default orientation

	* Escape -> cancel placement tool

	TOOD: what happens if somebody saves while the preview is active?
	*/

	bool OnMouseOverride(wxMouseEvent& evt)
	{
		if (evt.GetWheelRotation())
		{
			float speed = M_PI/36.f;
			if (wxGetKeyState(WXK_SHIFT) && wxGetKeyState(WXK_CONTROL))
				speed /= 64.f;
			else if (wxGetKeyState(WXK_CONTROL))
				speed /= 4.f;
			else if (wxGetKeyState(WXK_SHIFT))
				speed *= 4.f;
			g_DefaultAngle += (evt.GetWheelRotation() * speed / evt.GetWheelDelta());
			SendPreviewCommand();
			return true;
		}
		else
			return false;
	}

	bool OnKeyOverride(wxKeyEvent& evt, KeyEventType dir)
	{
		if (dir == KEY_CHAR && evt.GetKeyCode() == WXK_ESCAPE)
		{
			SetState(&Disabled);
			return true;
		}
		// TODO: arrow keys for rotation?
		else
			return false;
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
				obj->SendPreviewCommand();
				obj->m_ObjPos = Position::Unchanged(); // make sure object is stationary even if the camera moves
				SET_STATE(Placing);
				return true;
			}
			else if (evt.Moving())
			{
				obj->m_ObjPos = obj->m_ScreenPos = obj->m_Target = Position(evt.GetPosition());
				obj->SendPreviewCommand();
				return true;
			}
			else
				return false;
		}
		bool OnKey(PlaceObject* obj, wxKeyEvent& evt, KeyEventType dir)
		{
			return obj->OnKeyOverride(evt, dir);
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
				// TODO: createobject command, so you can actually place an object
				SET_STATE(Waiting);
				obj->m_ObjPos = obj->m_ScreenPos = obj->m_Target;
				obj->SendPreviewCommand();
				return true;
			}
			else if (evt.Moving())
			{
				obj->m_Target = Position(evt.GetPosition());
				obj->SendPreviewCommand();
				return true;
			}
			else
				return false;
		}
		bool OnKey(PlaceObject* obj, wxKeyEvent& evt, KeyEventType dir)
		{
			return obj->OnKeyOverride(evt, dir);
		}
	}
	Placing;
};

IMPLEMENT_DYNAMIC_CLASS(PlaceObject, StateDrivenTool<PlaceObject>);
