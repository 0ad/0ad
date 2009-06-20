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

#include "precompiled.h"

#include "ScenarioEditor/ScenarioEditor.h"
#include "Common/Tools.h"
#include "Common/Brushes.h"
#include "GameInterface/Messages.h"

using AtlasMessage::Position;

class FlattenElevation : public StateDrivenTool<FlattenElevation>
{
	DECLARE_DYNAMIC_CLASS(FlattenElevation);

	Position m_Pos;

public:
	FlattenElevation()
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
		bool OnMouse(FlattenElevation* obj, wxMouseEvent& evt)
		{
			if (evt.LeftDown())
			{
				obj->m_Pos = Position(evt.GetPosition());
				SET_STATE(Flattening);
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


	struct sFlattening : public State
	{
		void OnEnter(FlattenElevation* obj)
		{
			POST_MESSAGE(BrushPreview, (true, obj->m_Pos));
		}

		void OnLeave(FlattenElevation*)
		{
			ScenarioEditor::GetCommandProc().FinaliseLastCommand();
		}

		bool OnMouse(FlattenElevation* obj, wxMouseEvent& evt)
		{
			if (evt.LeftUp())
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

		void OnTick(FlattenElevation* obj, float dt)
		{
			POST_COMMAND(FlattenElevation, (obj->m_Pos, dt*4096.f*g_Brush_Elevation.GetStrength()));
			obj->m_Pos = Position::Unchanged();
		}
	}
	Flattening;
};

IMPLEMENT_DYNAMIC_CLASS(FlattenElevation, StateDrivenTool<FlattenElevation>);
