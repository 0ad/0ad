/*
IGUIScrollBar
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"

using namespace std;

//-------------------------------------------------------------------
//	IGUIScrollBar
//-------------------------------------------------------------------
IGUIScrollBar::IGUIScrollBar() : m_pStyle(NULL), m_pGUI(NULL),
								 m_X(300), m_Y(300),
								 m_ScrollRange(1), m_ScrollSpace(0), // MaxPos: not 0, due to division.
								 m_Length(200), m_Width(20), 
								 m_BarSize(0.5), m_Pos(0),
								 m_UseEdgeButtons(true),
								 m_ButtonPlusPressed(false),
								 m_ButtonMinusPressed(false),
								 m_ButtonPlusHovered(false),
								 m_ButtonMinusHovered(false),
								 m_BarHovered(false),
								 m_BarPressed(false)
{
}

IGUIScrollBar::~IGUIScrollBar()
{
}

void IGUIScrollBar::SetupBarSize()
{
	m_BarSize = min((float)m_ScrollSpace/(float)m_ScrollRange, 1.f);
}

const SGUIScrollBarStyle *IGUIScrollBar::GetStyle() const
{
	if (!m_pHostObject)
		return NULL;

    return m_pHostObject->GetScrollBarStyle(m_ScrollBarStyle);
}

CGUI *IGUIScrollBar::GetGUI() const 
{ 
	if (!m_pHostObject)
		return NULL;

	return m_pHostObject->GetGUI(); 
}

void IGUIScrollBar::UpdatePosBoundaries()
{
	if (m_Pos < 0 ||
		m_ScrollRange < m_ScrollSpace) // <= scrolling not applicable
		m_Pos = 0;
	else
	if (m_Pos > m_ScrollRange - m_ScrollSpace)
		m_Pos = m_ScrollRange - m_ScrollSpace;
}

void IGUIScrollBar::HandleMessage(const SGUIMessage &Message)
{
	switch (Message.type)
	{
	case GUIM_MOUSE_MOTION:
		{
			// TODO Gee: Optimizations needed!

			CPos mouse = m_pHostObject->GetMousePos();

			// If bar is being dragged
			if (m_BarPressed)
			{
				SetPosFromMousePos(mouse);
				UpdatePosBoundaries();
			}

			CRect bar_rect = GetBarRect();
			// check if components are being hovered
			m_BarHovered = bar_rect.PointInside(mouse);

			m_ButtonMinusHovered = HoveringButtonMinus(m_pHostObject->GetMousePos());
			m_ButtonPlusHovered =  HoveringButtonPlus(m_pHostObject->GetMousePos());

			if (!m_ButtonMinusHovered)
				m_ButtonMinusPressed = false;

			if (!m_ButtonPlusHovered)
				m_ButtonPlusPressed = false;
		} break;

	case GUIM_MOUSE_PRESS_LEFT:
		{
			if (!m_pHostObject)
				break;

			CPos mouse = m_pHostObject->GetMousePos();

			// if bar is pressed
			if (GetBarRect().PointInside(mouse))
			{
				m_BarPressed = true;
				m_BarPressedAtPos = mouse;
				m_PosWhenPressed = (float)m_Pos;
			}
			else
			// if button-minus is pressed
			if (m_ButtonMinusHovered)
			{
				m_ButtonMinusPressed = true;
				ScrollMinus();
			}
			else
			// if button-plus is pressed
			if (m_ButtonPlusHovered)
			{
				m_ButtonPlusPressed = true;
				ScrollPlus();
			}
			else
			// Pressing the background of the bar, to scroll
			//  notice the if-sentence alone does not admit that,
			//  it must be after the above if/elses
			{
				if (GetOuterRect().PointInside(mouse))
				{
					// Scroll plus or minus a lot, this might change, it doesn't
					//  have to be fancy though.
					if (mouse.y < GetBarRect().top)
						ScrollMinusPlenty();
					else
						ScrollPlusPlenty();

					// Simulate mouse movement to see if bar now is hovered
					HandleMessage(SGUIMessage(GUIM_MOUSE_MOTION));
				}
			}
		} break;

	case GUIM_MOUSE_RELEASE_LEFT:
		m_ButtonMinusPressed = false;
		m_ButtonPlusPressed = false;
		break;

	default:
		break;
	}
}
