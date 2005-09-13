#include "stdafx.h"

#include "Common/Tools.h"

#include "GameInterface/Messages.h"

using AtlasMessage::Position;

class AlterElevation : public ITool
{
public:
	AlterElevation()
		: m_Direction(0)
	{
	}

	void OnMouse(wxMouseEvent& evt)
	{
		if (evt.LeftDown())
		{
			ScenarioEditor::GetCommandProc().FinaliseLastCommand();
			m_Direction = +1;
			m_Pos = Position(evt.GetPosition());
		}
		else if (evt.RightDown())
		{
			ScenarioEditor::GetCommandProc().FinaliseLastCommand();
			m_Direction = -1;
			m_Pos = Position(evt.GetPosition());
		}
		else if (evt.LeftUp() || evt.RightUp())
		{
			ScenarioEditor::GetCommandProc().FinaliseLastCommand();
			m_Direction = 0;
		}
		else if (evt.Dragging())
		{
			m_Pos = Position(evt.GetPosition());
		}
		else
		{
			evt.Skip();
		}
	}

	void OnKey(wxKeyEvent& evt, int WXUNUSED(dir))
	{
		evt.Skip();
	}

	void OnTick(float dt)
	{
		if (m_Direction)
		{
			// TODO: If the mouse hasn't been moved in this stroke, use the
			// same tile position as last time (else it's annoying when digging
			// deep holes or building tall hills.)
			ADD_WORLDCOMMAND(AlterElevation, (m_Pos, dt*4096.f*m_Direction));
		}
	}

private:

	int m_Direction; // +1 = raise, -1 = lower, 0 = inactive
	Position m_Pos;
};

DECLARE_TOOL(AlterElevation);
