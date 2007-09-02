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

public:
	PaintTerrain()
	{
		SetState(&Waiting);
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
			POST_COMMAND(PaintTerrain, (obj->m_Pos, g_SelectedTexture.c_str(), GetPriority()));
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
};

IMPLEMENT_DYNAMIC_CLASS(PaintTerrain, StateDrivenTool<PaintTerrain>);
