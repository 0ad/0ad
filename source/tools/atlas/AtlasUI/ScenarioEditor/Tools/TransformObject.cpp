/* Copyright (C) 2012 Wildfire Games.
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
	AtlasMessage::ObjectID m_lastSelected;
	wxPoint m_startPoint;

	// TODO: If we don't plan to change hotkeys, just replace with evt.ShiftDown(), etc.
	static const wxKeyCode SELECTION_ADD_HOTKEY = WXK_SHIFT;
	static const wxKeyCode SELECTION_REMOVE_HOTKEY = WXK_CONTROL;	// COMMAND on Macs
	static const wxKeyCode SELECTION_ACTORS_HOTKEY = WXK_ALT;

public:
	TransformObject() : m_lastSelected(0)
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
			if (evt.LeftDClick() && AtlasMessage::ObjectIDIsValid(obj->m_lastSelected))
			{
				SET_STATE(SelectSimilar);
				return true;
			}
			else if (evt.LeftDown())
			{
				bool selectionAdd = wxGetKeyState(SELECTION_ADD_HOTKEY);
				bool selectionRemove = wxGetKeyState(SELECTION_REMOVE_HOTKEY);
				bool selectionActors = wxGetKeyState(SELECTION_ACTORS_HOTKEY);

				// New selection - never merge with movements of other objects
				ScenarioEditor::GetCommandProc().FinaliseLastCommand();

				// Select the object clicked on:

				AtlasMessage::qPickObject qry(Position(evt.GetPosition()), selectionActors);
				qry.Post();

				// Check they actually clicked on a valid object
				if (AtlasMessage::ObjectIDIsValid(qry.id))
				{
					std::vector<AtlasMessage::ObjectID>::iterator it = std::find(g_SelectedObjects.begin(), g_SelectedObjects.end(), qry.id);
					bool objectIsSelected = (it != g_SelectedObjects.end());

					if (selectionRemove)
					{
						// Remove from selection
						if (objectIsSelected)
							g_SelectedObjects.erase(it);
					}
					else if (!objectIsSelected)
					{
						// Add to selection
						if (!selectionAdd)
							g_SelectedObjects.clear();

						g_SelectedObjects.push_back(qry.id);
					}

					obj->m_lastSelected = qry.id;

					// If we're selecting the whole group
					if (!selectionAdd && !selectionRemove && !g_SelectedObjects.empty())
					{
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
				}
				else
				{
					// Bandboxing
					obj->m_lastSelected = 0;
					obj->m_startPoint = evt.GetPosition();
					SET_STATE(Bandboxing);
				}

				return true;
			}
			else if (g_SelectedObjects.size() == 1 && ((evt.Dragging() && evt.RightIsDown()) || evt.RightDown()))
			{
				// TODO: Rotation of selections with multiple objects?

				// Dragging with right mouse button -> rotate objects to look
				// at mouse
				Position pos (evt.GetPosition());
				for (size_t i = 0; i < g_SelectedObjects.size(); ++i)
					POST_COMMAND(RotateObject, (g_SelectedObjects[i], true, pos, 0.f));

				return true;
			}
			else if (evt.Moving())
			{
				// Prevent certain events from reaching game UI in this mode
				//	to prevent selection ring confusion
				return true;
			}
			else
				return false;
		}

		bool OnKey(TransformObject* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_CHAR && evt.GetKeyCode() == WXK_DELETE)
			{
				POST_COMMAND(DeleteObjects, (g_SelectedObjects));

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

				POST_COMMAND(MoveObjects, (g_SelectedObjects, obj->m_lastSelected, pos));
				return true;
			}
			else
				return false;
		}

		bool OnKey(TransformObject* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_UP && evt.GetKeyCode() == WXK_ESCAPE)
			{
				// Cancel move action
				ScenarioEditor::GetCommandProc().FinaliseLastCommand();
				ScenarioEditor::GetCommandProc().Undo();
				SET_STATE(Waiting);

				return true;
			}
			else
				return false;
		}
	}
	Dragging;

	struct sBandboxing : public State
	{
		bool OnMouse(TransformObject* obj, wxMouseEvent& evt)
		{
			if (evt.LeftIsDown() && evt.Dragging())
			{
				// Update bandbox overlay
				POST_MESSAGE(SetBandbox, (true, obj->m_startPoint.x, obj->m_startPoint.y, evt.GetPosition().x, evt.GetPosition().y));
				return true;
			}
			else if (evt.LeftUp())
			{
				bool selectionAdd = wxGetKeyState(SELECTION_ADD_HOTKEY);
				bool selectionRemove = wxGetKeyState(SELECTION_REMOVE_HOTKEY);
				bool selectionActors = wxGetKeyState(SELECTION_ACTORS_HOTKEY);

				// Now we have both corners of the box, pick objects
				AtlasMessage::qPickObjectsInRect qry(Position(obj->m_startPoint), Position(evt.GetPosition()), selectionActors);
				qry.Post();

				std::vector<AtlasMessage::ObjectID> ids = *qry.ids;

				if (!selectionAdd && !selectionRemove)
				{
					// Just copy new selections (clears list if no selections)
					g_SelectedObjects = ids;
				}
				else
				{
					for (size_t i = 0; i < ids.size(); ++i)
					{
						std::vector<AtlasMessage::ObjectID>::iterator it = 	std::find(g_SelectedObjects.begin(), g_SelectedObjects.end(), ids[i]);
						bool objectIsSelected = (it != g_SelectedObjects.end());
						if (selectionRemove)
						{
							// Remove from selection
							if (objectIsSelected)
								g_SelectedObjects.erase(it);
						}
						else if (!objectIsSelected)
						{
							// Add to selection
							g_SelectedObjects.push_back(ids[i]);
						}
					}
				}

				POST_MESSAGE(SetBandbox, (false, 0, 0, 0, 0));

				g_SelectedObjects.NotifyObservers();
				POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));

				SET_STATE(Waiting);
				return true;
			}
			else
				return false;
		}

		bool OnKey(TransformObject* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_UP && evt.GetKeyCode() == WXK_ESCAPE)
			{
				// Clear bandbox and return to waiting state
				POST_MESSAGE(SetBandbox, (false, 0, 0, 0, 0));
				SET_STATE(Waiting);
				return true;
			}
			else
				return false;
		}
	}
	Bandboxing;

	struct sSelectSimilar : public State
	{
		bool OnMouse(TransformObject* obj, wxMouseEvent& evt)
		{
			if (evt.LeftUp())
			{
				bool selectionAdd = wxGetKeyState(SELECTION_ADD_HOTKEY);
				bool selectionRemove = wxGetKeyState(SELECTION_REMOVE_HOTKEY);

				// Select similar objects
				AtlasMessage::qPickSimilarObjects qry(obj->m_lastSelected);
				qry.Post();

				std::vector<AtlasMessage::ObjectID> ids = *qry.ids;
				
				if (!selectionAdd && !selectionRemove)
				{
					// Just copy new selections (clears list if no selections)
					g_SelectedObjects = ids;
				}
				else
				{
					for (size_t i = 0; i < ids.size(); ++i)
					{
						std::vector<AtlasMessage::ObjectID>::iterator it = 	std::find(g_SelectedObjects.begin(), g_SelectedObjects.end(), ids[i]);
						bool objectIsSelected = (it != g_SelectedObjects.end());
						if (selectionRemove)
						{
							// Remove from selection
							if (objectIsSelected)
								g_SelectedObjects.erase(it);
						}
						else if (!objectIsSelected)
						{
							// Add to selection
							g_SelectedObjects.push_back(ids[i]);
						}
					}
				}

				g_SelectedObjects.NotifyObservers();
				POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));

				SET_STATE(Waiting);
				return true;
			}
			else
				return false;
		}
	}
	SelectSimilar;
};

IMPLEMENT_DYNAMIC_CLASS(TransformObject, StateDrivenTool<TransformObject>);
