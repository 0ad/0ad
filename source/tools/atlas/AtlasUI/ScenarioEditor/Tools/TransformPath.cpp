/* Copyright (C) 2017 Wildfire Games.
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

#include "Common/Tools.h"
#include "Common/Brushes.h"
#include "Common/MiscState.h"
#include "Common/ObjectSettings.h"
#include "GameInterface/Messages.h"
#include "ScenarioEditor/ScenarioEditor.h"

#include <wx/clipbrd.h>
#include <wx/sstream.h>
#include <wx/version.h>
#include <wx/xml/xml.h>


using AtlasMessage::Position;

class TransformPath : public StateDrivenTool<TransformPath>
{
	DECLARE_DYNAMIC_CLASS(TransformPath);

	wxPoint m_StartPoint;
	AtlasMessage::sCinemaPathNode node;
	int axis;

public:
	TransformPath()
	{
		SetState(&Waiting);
	}

	void OnDisable()
	{
		POST_MESSAGE(ClearPathNodePreview, );
	}

	struct sWaiting : public State
	{
		bool OnMouse(TransformPath* obj, wxMouseEvent& evt)
		{
			if (evt.LeftDown())
			{
				ScenarioEditor::GetCommandProc().FinaliseLastCommand();

				AtlasMessage::qPickPathNode query(Position(evt.GetPosition()));
				query.Post();

				obj->node = query.node;
				if (obj->node.index != AtlasMessage::AXIS_INVALID)
					SET_STATE(WaitingAxis);
				return true;
			}
			else
				return false;
		}
	}
	Waiting;
	
	struct sWaitingAxis : public State
	{
		bool OnMouse(TransformPath* obj, wxMouseEvent& evt)
		{
			if (evt.LeftDown())
			{
				AtlasMessage::qPickAxis query(obj->node, Position(evt.GetPosition()));
				query.Post();

				obj->axis = query.axis;
				if (obj->axis != AtlasMessage::AXIS_INVALID)
				{
					obj->m_StartPoint = evt.GetPosition();
					SET_STATE(Dragging);
				}
				return true;
			}
			else if (evt.LeftUp())
			{
				if (obj->axis != AtlasMessage::AXIS_INVALID)
					return false;

				AtlasMessage::qPickPathNode query(Position(evt.GetPosition()));
				query.Post();

				obj->node = query.node;
				if (obj->node.index == -1)
					SET_STATE(Waiting);
				return true;
			}
			else
				return false;
		}

		bool OnKey(TransformPath* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type != KEY_UP)
				return false;
			switch (evt.GetKeyCode())
			{
			case WXK_INSERT:
				POST_COMMAND(AddPathNode, (obj->node));
				return true;
			case WXK_DELETE:
				POST_COMMAND(DeletePathNode, (obj->node));
				obj->node.index = -1;
				return true;
			case WXK_ESCAPE:
				POST_MESSAGE(ClearPathNodePreview, );
				SET_STATE(Waiting);
				return true;
			default:
				return false;
			}
		}
	}
	WaitingAxis;

	struct sDragging : public State
	{
		bool OnMouse(TransformPath* obj, wxMouseEvent& evt)
		{
			if (evt.LeftUp())
			{
				obj->axis = AtlasMessage::AXIS_INVALID;
				SET_STATE(WaitingAxis);
				return true;
			}
			else if (evt.Dragging())
			{
				POST_COMMAND(MovePathNode, (obj->node, obj->axis, Position(obj->m_StartPoint), Position(evt.GetPosition())));
				obj->m_StartPoint = evt.GetPosition();
				return true;
			}
			else
				return false;
		}

		bool OnKey(TransformPath* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type != KEY_UP)
				return false;
			if (evt.GetKeyCode() == WXK_ESCAPE)
			{
				POST_MESSAGE(ClearPathNodePreview, );
				SET_STATE(Waiting);
				return true;
			}
			else
				return false;
		}
	}
	Dragging;
};

IMPLEMENT_DYNAMIC_CLASS(TransformPath, StateDrivenTool<TransformPath>);
