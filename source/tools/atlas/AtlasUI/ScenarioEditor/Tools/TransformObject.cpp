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
#include "Common/MiscState.h"
#include "Common/ObjectSettings.h"
#include "GameInterface/Messages.h"

using AtlasMessage::Position;

class TransformObject : public StateDrivenTool<TransformObject>
{
	DECLARE_DYNAMIC_CLASS(TransformObject);

	int m_dx, m_dy;

public:
	TransformObject()
	{
		SetState(&Waiting);
	}

	void OnDisable()
	{
		g_SelectedObjects.clear();
		g_SelectedObjects.NotifyObservers();
		POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));
	}


	// TODO: keys to rotate/move object?

	struct sWaiting : public State
	{
		bool OnMouse(TransformObject* obj, wxMouseEvent& evt)
		{
			if (evt.LeftDown())
			{
				// New selection - never merge with movements of other objects
				ScenarioEditor::GetCommandProc().FinaliseLastCommand();

				// Select the object clicked on:

				AtlasMessage::qPickObject qry(Position(evt.GetPosition()));
				qry.Post();

				// TODO: handle multiple selections
				g_SelectedObjects.clear();

				// Check they actually clicked on a valid object
				if (AtlasMessage::ObjectIDIsValid(qry.id))
				{
					g_SelectedObjects.push_back(qry.id);
					// Remember the screen-space offset of the mouse from the
					// object's centre, so we can add that back when moving it
					// (instead of just moving the object's centre to directly
					// beneath the mouse)
					obj->m_dx = qry.offsetx;
					obj->m_dy = qry.offsety;
					SET_STATE(Dragging);
				}
				g_SelectedObjects.NotifyObservers();
				POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));
				return true;
			}
			else if ((evt.Dragging() && evt.RightIsDown()) || evt.RightDown())
			{
				// Dragging with right mouse button -> rotate objects to look
				// at mouse
				Position pos (evt.GetPosition());
				for (size_t i = 0; i < g_SelectedObjects.size(); ++i)
					POST_COMMAND(RotateObject, (g_SelectedObjects[i], true, pos, 0.f));

				return true;
			}
			else
				return false;
		}

		bool OnKey(TransformObject* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_CHAR && evt.GetKeyCode() == WXK_DELETE)
			{
				for (size_t i = 0; i < g_SelectedObjects.size(); ++i)
					POST_COMMAND(DeleteObject, (g_SelectedObjects[i]));

				g_SelectedObjects.clear();
				g_SelectedObjects.NotifyObservers();

				POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));
				return true;
			}
			else if (type == KEY_CHAR && (evt.GetKeyCode() >= '0' && evt.GetKeyCode() <= '9'))
			{
				int playerID = evt.GetKeyCode() - '0';
				obj->GetScenarioEditor().GetObjectSettings().SetPlayerID(playerID);
				obj->GetScenarioEditor().GetObjectSettings().NotifyObservers();
				return true;
			}
			else
				return false;
		}
	}
	Waiting;

	struct sDragging : public State
	{
		bool OnMouse(TransformObject* obj, wxMouseEvent& evt)
		{
			if (evt.LeftUp())
			{
				SET_STATE(Waiting);
				return true;
			}
			else if (evt.Dragging())
			{
				Position pos (evt.GetPosition() + wxPoint(obj->m_dx, obj->m_dy));
				for (size_t i = 0; i < g_SelectedObjects.size(); ++i)
					POST_COMMAND(MoveObject, (g_SelectedObjects[i], pos));
				return true;
			}
			else
				return false;
		}
	}
	Dragging;
};

IMPLEMENT_DYNAMIC_CLASS(TransformObject, StateDrivenTool<TransformObject>);
