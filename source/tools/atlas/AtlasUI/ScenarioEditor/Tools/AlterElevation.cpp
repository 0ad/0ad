#include "stdafx.h"

#include "Common/Tools.h"

#include "GameInterface/Messages.h"

using AtlasMessage::Position;

class AlterElevation : public ITool
{
public:
	AlterElevation()
		: m_IsActive(false)
	{
	}

	void OnMouse(wxMouseEvent& evt)
	{
		if (evt.LeftDown())
		{
			m_IsActive = true;
			m_Pos = Position(evt.GetPosition());
		}
		else if (evt.LeftUp())
		{
			m_IsActive = false;
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

	void OnKey(wxKeyEvent& evt, int dir)
	{
		evt.Skip();
	}

	void OnTick(float dt)
	{
		if (m_IsActive)
		{
			ADD_WORLDCOMMAND(AlterElevation, (m_Pos, dt*4.096f));
		}
	}

private:

	bool m_IsActive;
	Position m_Pos;
};

DECLARE_TOOL(AlterElevation);
