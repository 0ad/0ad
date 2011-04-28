/* Copyright (C) 2009 Wildfire Games.
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

/*
IGUIScrollBar
*/

#include "precompiled.h"
#include "GUI.h"


//-------------------------------------------------------------------
//	IGUIScrollBar
//-------------------------------------------------------------------
IGUIScrollBar::IGUIScrollBar() : m_pStyle(NULL), m_pGUI(NULL),
								 m_X(300.f), m_Y(300.f),
								 m_ScrollRange(1.f), m_ScrollSpace(0.f), // MaxPos: not 0, due to division.
								 m_Length(200.f), m_Width(20.f), 
								 m_BarSize(0.5f), m_Pos(0.f),
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
	m_BarSize = std::min((float)m_ScrollSpace/(float)m_ScrollRange, 1.f);
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
	if (m_Pos < 0.f ||
		m_ScrollRange < m_ScrollSpace) // <= scrolling not applicable
		m_Pos = 0.f;
	else
	if (m_Pos > m_ScrollRange - m_ScrollSpace)
		m_Pos = m_ScrollRange - m_ScrollSpace;
}

void IGUIScrollBar::HandleMessage(SGUIMessage &Message)
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
				m_PosWhenPressed = m_Pos;
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
					SGUIMessage msg(GUIM_MOUSE_MOTION);
					HandleMessage(msg);
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
