#include "stdafx.h"

#include "Common/Tools.h"

#include "GameInterface/Messages.h"

using AtlasMessage::Position;

class AlterElevation : public StateDrivenTool<AlterElevation>
{
public:
	AlterElevation()
	{
		SetState(&Waiting);
	}

private:

	int m_Direction; // +1 = raise, -1 = lower
	Position m_Pos;

protected:

	struct Waiting : public State
	{
		bool mouse(AlterElevation* obj, wxMouseEvent& evt)
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
			else
			{
				return false;
			}
		}
	}
	Waiting;


	struct Altering_common : public State
	{
		void leave(AlterElevation*)
		{
			ScenarioEditor::GetCommandProc().FinaliseLastCommand();
		}

		bool mouse(AlterElevation* obj, wxMouseEvent& evt)
		{
			if (isMouseUp(evt))
			{
				SET_STATE(Waiting);
				return true;
			}
			else if (evt.Dragging())
			{
				obj->m_Pos = Position(evt.GetPosition());
				return true;
			}
			else
			{
				return false;
			}
		}

		void tick(AlterElevation* obj, float dt)
		{
			ADD_WORLDCOMMAND(AlterElevation, (obj->m_Pos, dt*4096.f*getDirection()));
		}

		virtual bool isMouseUp(wxMouseEvent& evt) = 0;
		virtual int getDirection() = 0;
	};

	struct Raising : public Altering_common
	{
		bool isMouseUp(wxMouseEvent& evt) { return evt.LeftUp(); }
		int getDirection() { return +1; }
	}
	Raising;

	struct Lowering : public Altering_common
	{
		bool isMouseUp(wxMouseEvent& evt) { return evt.RightUp(); }
		int getDirection() { return -1; }
	}
	Lowering;
};

DECLARE_TOOL(AlterElevation);

