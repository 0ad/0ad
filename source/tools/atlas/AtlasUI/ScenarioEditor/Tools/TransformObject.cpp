/* Copyright (C) 2014 Wildfire Games.
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
#include <wx/clipbrd.h>
#include <wx/xml/xml.h>
#include <wx/sstream.h>
#include <wx/version.h>

using AtlasMessage::Position;

class TransformObject : public StateDrivenTool<TransformObject>
{
	DECLARE_DYNAMIC_CLASS(TransformObject);

	int m_dx, m_dy;
	AtlasMessage::ObjectID m_lastSelected;
	wxPoint m_startPoint;
	Position m_entPosition;

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

	void OnPasteStart()
	{
		//PASTE
		wxString entities;
		if (wxTheClipboard->Open())
		{
			if (wxTheClipboard->IsSupported(wxDF_TEXT))
			{
				wxTextDataObject data;
				wxTheClipboard->GetData(data);
				entities = data.GetText();
			}
			wxTheClipboard->Close();
		}
		else
		{
			//TODO: Show something when the clipboard couldnt open
		}

		//First do we need to check if it is a correct xml string
		wxInputStream* is = new wxStringInputStream(entities);
		wxXmlDocument doc;
		if (!doc.Load(*is))
			return;

		// Entities, Entity(1.*)
		wxXmlNode* root = doc.GetRoot();
		if (root->GetName() != wxT("Entities"))
			return;

		//	Template, position,orientation
		const wxXmlNode* child = root->GetChildren();

		while (child)
		{
			if (child->GetName() != wxT("Entity"))
				return;

			//TODO Validate all attributes
			child = child->GetNext();
		}

		//Update selectedObjects??
		g_SelectedObjects.clear();
		POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));

		// is the source code get here now you can add the objects to scene(preview)
		// store id to move 
		child = root->GetChildren();

		while (child)
		{
			wxString templateName;
			Position entityPos;
			long playerId = 0;
			double orientation = 0;

			const wxXmlNode* xmlData = child->GetChildren();

			while (xmlData)
			{
				if (xmlData->GetName() == wxT("Template"))
					templateName = xmlData->GetNodeContent();
				else if (xmlData->GetName() == wxT("Position"))
				{
					wxString x, z;
#if wxCHECK_VERSION(3, 0, 0)
					xmlData->GetAttribute(wxT("x"), &x);
					xmlData->GetAttribute(wxT("z"), &z);
#else
					xmlData->GetPropVal(wxT("x"), &x);
					xmlData->GetPropVal(wxT("z"), &z);
#endif

					double aux, aux2;
					x.ToDouble(&aux);
					z.ToDouble(&aux2);

					entityPos = Position(aux, 0, aux2);
				}
				else if (xmlData->GetName() == wxT("Orientation"))
				{
					wxString y;
#if wxCHECK_VERSION(3, 0, 0)
					xmlData->GetAttribute(wxT("y"), &y);
#else
					xmlData->GetPropVal(wxT("y"), &y);
#endif
					y.ToDouble(&orientation);
				}
				else if (xmlData->GetName() == wxT("Player"))
				{
					wxString x;
					x = xmlData->GetNodeContent();
					x.ToLong(&playerId);
				}

				xmlData = xmlData->GetNext();
			}

			//Update current Ownership
			this->GetScenarioEditor().GetObjectSettings().SetPlayerID(playerId);
			this->GetScenarioEditor().GetObjectSettings().NotifyObservers();

			POST_MESSAGE(ObjectPreview, ((std::wstring)templateName.c_str(), GetScenarioEditor().GetObjectSettings().GetSettings(), entityPos, false, Position(), orientation, 0, false));

			child = child->GetNext();
		}

		//Set state paste for preview the new objects
		this->SetState(&Pasting);

		//Update the objects to current mouse position
		OnMovingPaste();
	}

	void OnMovingPaste()
	{
		//Move the preview(s) object(s)
		POST_MESSAGE(MoveObjectPreview, ((m_entPosition)));
	}

	void OnPasteEnd(bool canceled)
	{
		if (canceled)
			//delete previews objects
			POST_MESSAGE(ObjectPreview, (_T(""), GetScenarioEditor().GetObjectSettings().GetSettings(), Position(), false, Position(), 0, 0, true));
		else
		{
			//Create new Objects and delete preview objects
			POST_MESSAGE(ObjectPreviewToEntity, ());

			AtlasMessage::qGetCurrentSelection currentSelection;
			currentSelection.Post();

			g_SelectedObjects = *currentSelection.ids;
		}


		//when all is done set default state
		this->SetState(&Waiting);
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
				Position pos(evt.GetPosition());
				for (size_t i = 0; i < g_SelectedObjects.size(); ++i)
					POST_COMMAND(RotateObject, (g_SelectedObjects[i], true, pos, 0.f));

				return true;
			}
			else if (evt.Moving())
			{
				//Save position for smooth paste position
				obj->m_entPosition = Position(evt.GetPosition());

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
			else if (evt.GetModifiers() == wxMOD_CONTROL)
			{
				if (evt.GetKeyCode() == 'C')
				{
					if (!g_SelectedObjects.empty())
					{
						//COPY current selections
						AtlasMessage::qGetObjectMapSettings info(g_SelectedObjects);
						info.Post();

						//In xmldata is the configuration
						//now send to clipboard
						if (wxTheClipboard->Open())
						{
							wxString text(info.xmldata.c_str());
							wxTheClipboard->SetData(new wxTextDataObject(text));
							wxTheClipboard->Close();
						}
						else
						{
							//TODO: Say something about couldnt open clipboard
						}
						return true;
					}
				}
				else if (evt.GetKeyCode() == 'V')
				{
					obj->OnPasteStart();
					return true;
				}
			}

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
				Position pos(evt.GetPosition() + wxPoint(obj->m_dx, obj->m_dy));

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
						std::vector<AtlasMessage::ObjectID>::iterator it = std::find(g_SelectedObjects.begin(), g_SelectedObjects.end(), ids[i]);
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
						std::vector<AtlasMessage::ObjectID>::iterator it = std::find(g_SelectedObjects.begin(), g_SelectedObjects.end(), ids[i]);
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

	struct sPasting : public State
	{
		bool OnMouse(TransformObject* obj, wxMouseEvent& evt)
		{
			if (evt.Moving())
			{
				//Move the object
				obj->m_entPosition = Position(evt.GetPosition());
				obj->OnMovingPaste();
				return true;
			}
			else if (evt.LeftDown())
			{
				//Place the object and update 
				obj->OnPasteEnd(false);
				return true;
			}
			else
				return false;
		}
		bool OnKey(TransformObject* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_CHAR && evt.GetKeyCode() == WXK_ESCAPE)
			{
				obj->OnPasteEnd(true);
				return true;
			}
			else
				return false;
		}
	}
	Pasting;
};

IMPLEMENT_DYNAMIC_CLASS(TransformObject, StateDrivenTool<TransformObject>);
