/*
IGUIScrollBar
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx.h"
#include "GUI.h"

using namespace std;

//-------------------------------------------------------------------
//	IGUIScrollBar
//-------------------------------------------------------------------
IGUIScrollBar::IGUIScrollBar() : m_pStyle(NULL), m_X(300), m_Y(300), m_Length(200), m_Width(20), m_BarSize(0.5), m_Pos(0.5)
{
}

IGUIScrollBar::~IGUIScrollBar()
{
}

const SGUIScrollBarStyle & IGUIScrollBar::GetStyle() const
{
	if (!m_pHostObject)
		return SGUIScrollBarStyle();

    return m_pHostObject->GetScrollBarStyle(m_ScrollBarStyle);
}

void IGUIScrollBar::UpdatePosBoundaries()
{
	if (m_Pos > 1.f)
		m_Pos = 1.f;
	else
	if (m_Pos < 0.f)
		m_Pos = 0.f;
}

bool IGUIScrollBar::HandleMessage(const SGUIMessage &Message)
{
	switch (Message.type)
	{
	case GUIM_MOUSE_MOTION:
		if (m_BarPressed)
		{
			SetPosFromMousePos(m_pHostObject->GetMouseX(), m_pHostObject->GetMouseY());
			UpdatePosBoundaries();
		}
		break;

	case GUIM_MOUSE_PRESS_LEFT:
		{
			if (!m_pHostObject)
				break;

			int mouse_x = m_pHostObject->GetMouseX(),
				mouse_y = m_pHostObject->GetMouseY();

			// if bar is pressed
			if	   (mouse_x >= GetBarRect().left &&
					mouse_x <= GetBarRect().right &&
					mouse_y >= GetBarRect().top &&
					mouse_y <= GetBarRect().bottom)
			{
				m_BarPressed = true;
				m_BarPressedAtX = mouse_x;
				m_BarPressedAtY = mouse_y;
				m_PosWhenPressed = m_Pos;
			}
			else
			// if button-minus is pressed
			if (HoveringButtonMinus(mouse_x, mouse_y))
			{
				ScrollMinus();
			}
			else
			// if button-plus is pressed
			if (HoveringButtonPlus(mouse_x, mouse_y))
			{
				ScrollPlus();
			}
		}
		break;

	default:
		return false;
	}

	return true;
}
