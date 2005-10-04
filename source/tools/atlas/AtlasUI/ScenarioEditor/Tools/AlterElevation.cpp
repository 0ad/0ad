#include "stdafx.h"

#include "Common/Tools.h"

#include "GameInterface/Messages.h"

using AtlasMessage::Position;

class AlterElevation : public StateDrivenTool<AlterElevation>
{
private:

	int m_Direction; // +1 = raise, -1 = lower
	Position m_Pos;

public:
	AlterElevation()
	{
		SetState(&Waiting);
	}


	void OnEnable(AlterElevation*)
	{
		int w = 2, h = 3;
		float* data = new float[w*h];
		for (int i = 0; i < w*h; ++i) data[i] = 1.f;
		POST_COMMAND(SetBrush(w, h, data));
	}

	void OnDisable(AlterElevation*)
	{
		POST_COMMAND(BrushPreview(false, Position()));
	}


	struct sWaiting : public State
	{
		bool OnMouse(AlterElevation* obj, wxMouseEvent& evt)
		{
			if (evt.LeftDown())
			{
				obj->m_Pos = Position(evt.GetPosition());
				SET_STATE(Raising);
				return true;
			}
			else if (evt.RightDown())
			{
				obj->m_Pos = Position(evt.GetPosition());
				SET_STATE(Lowering);
				return true;
			}
			else if (evt.Moving())
			{
				POST_COMMAND(BrushPreview(true, Position(evt.GetPosition())));
			}
			return false;
		}
	}
	Waiting;


	struct sAltering_common : public State
	{
		void OnLeave(AlterElevation*)
		{
			ScenarioEditor::GetCommandProc().FinaliseLastCommand();
		}

		bool OnMouse(AlterElevation* obj, wxMouseEvent& evt)
		{
			if (IsMouseUp(evt))
			{
				SET_STATE(Waiting);
				return true;
			}
			else if (evt.Dragging())
			{
				obj->m_Pos = Position(evt.GetPosition());
				POST_COMMAND(BrushPreview(true, obj->m_Pos));
				return true;
			}
			else
			{
				return false;
			}
		}

		void OnTick(AlterElevation* obj, float dt)
		{
			ADD_WORLDCOMMAND(AlterElevation, (obj->m_Pos, dt*4096.f*GetDirection()));
		}

		virtual bool IsMouseUp(wxMouseEvent& evt) = 0;
		virtual int GetDirection() = 0;
	};

	struct sRaising : public sAltering_common
	{
		bool IsMouseUp(wxMouseEvent& evt) { return evt.LeftUp(); }
		int GetDirection() { return +1; }
	}
	Raising;

	struct sLowering : public sAltering_common
	{
		bool IsMouseUp(wxMouseEvent& evt) { return evt.RightUp(); }
		int GetDirection() { return -1; }
	}
	Lowering;
};

DECLARE_TOOL(AlterElevation);

