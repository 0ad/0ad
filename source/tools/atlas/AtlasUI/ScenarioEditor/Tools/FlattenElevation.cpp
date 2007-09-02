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
