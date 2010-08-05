/* Copyright (C) 2010 Wildfire Games.
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
#include "GameInterface/Messages.h"


using AtlasMessage::Position;

class SmoothElevation : public StateDrivenTool<SmoothElevation>
{
	DECLARE_DYNAMIC_CLASS(SmoothElevation);

	int m_Direction; // +1 = smoother, -1 = rougher
	Position m_Pos;

public:
	SmoothElevation()
	{
		SetState(&Waiting);
	}


	void OnEnable()
	{
		g_Brush_Elevation.MakeActive();
	}

	void OnDisable()
	{
		POST_MESSAGE(BrushPreview, (false, Position()));
	}


	struct sWaiting : public State
	{
		bool OnMouse(SmoothElevation* obj, wxMouseEvent& evt)
		{
			if (evt.LeftDown())
			{
				obj->m_Pos = Position(evt.GetPosition());
				SET_STATE(Smoothing);
				return true;
			}
			else if (evt.RightDown())
			{
				obj->m_Pos = Position(evt.GetPosition());
				SET_STATE(Roughing);
				return true;
			}
			else if (evt.Moving())
			{
				POST_MESSAGE(BrushPreview, (true, Position(evt.GetPosition())));
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	Waiting;


	struct sSmoothing_common : public State
	{
		void OnEnter(SmoothElevation* obj)
		{
			POST_MESSAGE(BrushPreview, (true, obj->m_Pos));
		}

		void OnLeave(SmoothElevation*)
		{
			ScenarioEditor::GetCommandProc().FinaliseLastCommand();
		}

		bool OnMouse(SmoothElevation* obj, wxMouseEvent& evt)
		{
			if (IsMouseUp(evt))
			{
				SET_STATE(Waiting);
				return true;
			}
			else if (evt.Dragging())
			{
				wxPoint pos = evt.GetPosition();
				obj->m_Pos = Position(pos);
				POST_MESSAGE(BrushPreview, (true, obj->m_Pos));
				return true;
			}
			else
			{
				return false;
			}
		}

		void OnTick(SmoothElevation* obj, float dt)
		{
			POST_COMMAND(SmoothElevation, (obj->m_Pos, dt*4096.f*GetDirection()*g_Brush_Elevation.GetStrength()));
			obj->m_Pos = Position::Unchanged();
		}

		virtual bool IsMouseUp(wxMouseEvent& evt) = 0;
		virtual int GetDirection() = 0;
	};

	struct sSmoothing : public sSmoothing_common
	{
		bool IsMouseUp(wxMouseEvent& evt) { return evt.LeftUp(); }
		int GetDirection() { return +1; }
	}
	Smoothing;

	struct sRoughing : public sSmoothing_common
	{
		bool IsMouseUp(wxMouseEvent& evt) { return evt.RightUp(); }
		int GetDirection() { return -1; }
	}
	Roughing;
};

IMPLEMENT_DYNAMIC_CLASS(SmoothElevation, StateDrivenTool<SmoothElevation>);
