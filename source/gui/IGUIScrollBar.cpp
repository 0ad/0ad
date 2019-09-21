/* Copyright (C) 2019 Wildfire Games.
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

#include "IGUIScrollBar.h"

#include "gui/IGUIScrollBarOwner.h"
#include "gui/CGUI.h"
#include "maths/MathUtil.h"

IGUIScrollBar::IGUIScrollBar(CGUI& pGUI)
	: m_pGUI(pGUI),
	 m_pStyle(nullptr),
	 m_X(300.f), m_Y(300.f),
	 m_ScrollRange(1.f), m_ScrollSpace(0.f), // MaxPos: not 0, due to division.
	 m_Length(200.f), m_Width(20.f),
	 m_BarSize(0.f), m_Pos(0.f),
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
	if (!GetStyle())
		return;

	float min = GetStyle()->m_MinimumBarSize;
	float max = GetStyle()->m_MaximumBarSize;
	float length = m_Length;

	// Check for edge buttons
	if (GetStyle()->m_UseEdgeButtons)
		length -= GetStyle()->m_Width * 2.f;

	// Check min and max are valid
	if (min > length)
		min = 0.f;
	if (max < min)
		max = length;

	// Clamp size to not exceed a minimum or maximum.
	m_BarSize = Clamp(length * std::min(m_ScrollSpace / m_ScrollRange, 1.f), min, max);
}

const SGUIScrollBarStyle* IGUIScrollBar::GetStyle() const
{
	if (!m_pHostObject)
		return NULL;

	return m_pHostObject->GetScrollBarStyle(m_ScrollBarStyle);
}

void IGUIScrollBar::UpdatePosBoundaries()
{
	if (m_Pos < 0.f ||
		m_ScrollRange < m_ScrollSpace) // <= scrolling not applicable
		m_Pos = 0.f;
	else if (m_Pos > GetMaxPos())
		m_Pos = GetMaxPos();
}

void IGUIScrollBar::HandleMessage(SGUIMessage& Message)
{
	switch (Message.type)
	{
	case GUIM_MOUSE_MOTION:
	{
		// TODO Gee: Optimizations needed!

		const CPos& mouse = m_pGUI.GetMousePos();

		// If bar is being dragged
		if (m_BarPressed)
		{
			SetPosFromMousePos(mouse);
			UpdatePosBoundaries();
		}

		// check if components are being hovered
		m_BarHovered = GetBarRect().PointInside(mouse);
		m_ButtonMinusHovered = HoveringButtonMinus(mouse);
		m_ButtonPlusHovered =  HoveringButtonPlus(mouse);

		if (!m_ButtonMinusHovered)
			m_ButtonMinusPressed = false;

		if (!m_ButtonPlusHovered)
			m_ButtonPlusPressed = false;
		break;
	}

	case GUIM_MOUSE_PRESS_LEFT:
	{
		if (!m_pHostObject)
			break;

		const CPos& mouse = m_pGUI.GetMousePos();

		// if bar is pressed
		if (GetBarRect().PointInside(mouse))
		{
			m_BarPressed = true;
			m_BarPressedAtPos = mouse;
			m_PosWhenPressed = m_Pos;
		}
		// if button-minus is pressed
		else if (m_ButtonMinusHovered)
		{
			m_ButtonMinusPressed = true;
			ScrollMinus();
		}
		// if button-plus is pressed
		else if (m_ButtonPlusHovered)
		{
			m_ButtonPlusPressed = true;
			ScrollPlus();
		}
		// Pressing the background of the bar, to scroll
		//  notice the if-sentence alone does not admit that,
		//  it must be after the above if/elses
		else
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
		break;
	}

	case GUIM_MOUSE_RELEASE_LEFT:
		m_ButtonMinusPressed = false;
		m_ButtonPlusPressed = false;
		break;

	case GUIM_MOUSE_WHEEL_UP:
	{
		ScrollMinus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		SGUIMessage msg(GUIM_MOUSE_MOTION);
		HandleMessage(msg);
		break;
	}

	case GUIM_MOUSE_WHEEL_DOWN:
	{
		ScrollPlus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		SGUIMessage msg(GUIM_MOUSE_MOTION);
		HandleMessage(msg);
		break;
	}

	default:
		break;
	}
}
