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
#include "GameInterface/Messages.h"

using AtlasMessage::Position;

class PaintTerrain : public StateDrivenTool<PaintTerrain>
{
	DECLARE_DYNAMIC_CLASS(PaintTerrain);

	Position m_Pos;

	// Brush for eyedropper preview
	//	(it's confusing if we use the arbitrarily sized paint brush)
	Brush m_EyedropperBrush;

	static const wxKeyCode EYEDROPPER_HOTKEY = WXK_SHIFT;

public:
	PaintTerrain()
	{
		SetState(&Waiting);

		m_EyedropperBrush.SetSquare(2);
	}


	void OnEnable()
	{
		// TODO: multiple independent brushes?
		g_Brush_Elevation.MakeActive();
	}

	void OnDisable()
	{
		POST_MESSAGE(BrushPreview, (false, Position()));
	}

	struct sWaiting : public State
	{
		bool OnKey(PaintTerrain* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_DOWN && evt.GetKeyCode() == EYEDROPPER_HOTKEY)
			{
				SET_STATE(Eyedropper);
				return true;
			}
			else
			{
				return false;
			}
		}

		bool OnMouse(PaintTerrain* obj, wxMouseEvent& evt)
		{
			if (evt.LeftDown())
			{
				obj->m_Pos = Position(evt.GetPosition());
				SET_STATE(PaintingHigh);
				return true;
			}
			else if (evt.RightDown())
			{
				obj->m_Pos = Position(evt.GetPosition());
				SET_STATE(PaintingLow);
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


	struct sPainting_common : public State
	{
		void OnEnter(PaintTerrain* obj)
		{
			Paint(obj);
		}

		void OnLeave(PaintTerrain*)
		{
			ScenarioEditor::GetCommandProc().FinaliseLastCommand();
		}

		bool OnMouse(PaintTerrain* obj, wxMouseEvent& evt)
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
				Paint(obj);
				return true;
			}
			else
			{
				return false;
			}
		}

		void Paint(PaintTerrain* obj)
		{
			POST_MESSAGE(BrushPreview, (true, obj->m_Pos));
			POST_COMMAND(PaintTerrain, (obj->m_Pos, (std::wstring)g_SelectedTexture.wc_str(), GetPriority()));
		}

		virtual bool IsMouseUp(wxMouseEvent& evt) = 0;
		virtual int GetPriority() = 0;
	};

	struct sPaintingHigh : public sPainting_common
	{
		bool IsMouseUp(wxMouseEvent& evt) { return evt.LeftUp(); }
		int GetPriority() { return AtlasMessage::ePaintTerrainPriority::HIGH; }
	}
	PaintingHigh;

	struct sPaintingLow : public sPainting_common
	{
		bool IsMouseUp(wxMouseEvent& evt) { return evt.RightUp(); }
		int GetPriority() { return AtlasMessage::ePaintTerrainPriority::LOW; }
	}
	PaintingLow;

	struct sEyedropper : public State
	{
		void OnEnter(PaintTerrain* obj)
		{
			obj->m_EyedropperBrush.MakeActive();
		}

		void OnLeave(PaintTerrain* WXUNUSED(obj))
		{
			g_Brush_Elevation.MakeActive();
		}

		bool OnKey(PaintTerrain* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_UP && evt.GetKeyCode() == EYEDROPPER_HOTKEY)
			{
				SET_STATE(Waiting);
				return true;
			}
			else
			{
				return false;
			}
		}

		bool OnMouse(PaintTerrain* WXUNUSED(obj), wxMouseEvent& evt)
		{
			if (evt.ButtonDown() || evt.Dragging())
			{
				POST_MESSAGE(BrushPreview, (true, evt.GetPosition()));
				AtlasMessage::qGetTerrainTexture qry(evt.GetPosition());
				qry.Post();

				g_SelectedTexture = wxString(qry.texture.c_str());
				g_SelectedTexture.NotifyObservers();
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
	Eyedropper;
};

IMPLEMENT_DYNAMIC_CLASS(PaintTerrain, StateDrivenTool<PaintTerrain>);
