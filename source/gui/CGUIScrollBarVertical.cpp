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

#include "CGUIScrollBarVertical.h"

#include "GUI.h"

#include "ps/CLogger.h"


CGUIScrollBarVertical::CGUIScrollBarVertical(CGUI& pGUI)
 : IGUIScrollBar(pGUI)
{
}

CGUIScrollBarVertical::~CGUIScrollBarVertical()
{
}

void CGUIScrollBarVertical::SetPosFromMousePos(const CPos& mouse)
{
	if (!GetStyle())
		return;

	/**
	 * Calculate the position for the top of the item being scrolled
	 */
	float emptyBackground = m_Length - m_BarSize;

	if (GetStyle()->m_UseEdgeButtons)
		emptyBackground -= GetStyle()->m_Width * 2;

	m_Pos = m_PosWhenPressed + GetMaxPos() * (mouse.y - m_BarPressedAtPos.y) / emptyBackground;
}

void CGUIScrollBarVertical::Draw()
{
	if (!GetStyle())
	{
		LOGWARNING("Attempt to draw scrollbar without a style.");
		return;
	}

	if (IsVisible())
	{
		CRect outline = GetOuterRect();

		m_pGUI.DrawSprite(
			GetStyle()->m_SpriteBackVertical,
			0,
			m_Z+0.1f,
			CRect(
				outline.left,
				outline.top + (GetStyle()->m_UseEdgeButtons ? GetStyle()->m_Width : 0),
				outline.right,
				outline.bottom - (GetStyle()->m_UseEdgeButtons ? GetStyle()->m_Width : 0)
			)
		);

		if (GetStyle()->m_UseEdgeButtons)
		{
			const CGUISpriteInstance* button_top;
			const CGUISpriteInstance* button_bottom;

			if (m_ButtonMinusHovered)
			{
				if (m_ButtonMinusPressed)
					button_top = &(GetStyle()->m_SpriteButtonTopPressed || GetStyle()->m_SpriteButtonTop);
				else
					button_top = &(GetStyle()->m_SpriteButtonTopOver || GetStyle()->m_SpriteButtonTop);
			}
			else
				button_top = &GetStyle()->m_SpriteButtonTop;

			if (m_ButtonPlusHovered)
			{
				if (m_ButtonPlusPressed)
					button_bottom = &(GetStyle()->m_SpriteButtonBottomPressed || GetStyle()->m_SpriteButtonBottom);
				else
					button_bottom = &(GetStyle()->m_SpriteButtonBottomOver || GetStyle()->m_SpriteButtonBottom);
			}
			else
				button_bottom = &GetStyle()->m_SpriteButtonBottom;

			m_pGUI.DrawSprite(
				*button_top,
				0,
				m_Z+0.2f,
				CRect(
					outline.left,
					outline.top,
					outline.right,
					outline.top+GetStyle()->m_Width
				)
			);

			m_pGUI.DrawSprite(
				*button_bottom,
				0,
				m_Z+0.2f,
				CRect(
					outline.left,
					outline.bottom-GetStyle()->m_Width,
					outline.right,
					outline.bottom
				)
			);
		}

		m_pGUI.DrawSprite(
			GetStyle()->m_SpriteBarVertical,
			0,
			m_Z + 0.2f,
			GetBarRect()
		);
	}
}

void CGUIScrollBarVertical::HandleMessage(SGUIMessage& Message)
{
	IGUIScrollBar::HandleMessage(Message);
}

CRect CGUIScrollBarVertical::GetBarRect() const
{
	CRect ret;
	if (!GetStyle())
		return ret;

	// Get from where the scroll area begins to where it ends
	float from = m_Y;
	float to = m_Y + m_Length - m_BarSize;

	if (GetStyle()->m_UseEdgeButtons)
	{
		from += GetStyle()->m_Width;
		to -= GetStyle()->m_Width;
	}

	ret.top = from + (to - from) * m_Pos / GetMaxPos();
	ret.bottom = ret.top + m_BarSize;
	ret.right = m_X + (m_RightAligned ? 0 : GetStyle()->m_Width);
	ret.left = ret.right - GetStyle()->m_Width;

	return ret;
}

CRect CGUIScrollBarVertical::GetOuterRect() const
{
	CRect ret;
	if (!GetStyle())
		return ret;

	ret.top = m_Y;
	ret.bottom = m_Y+m_Length;
	ret.right = m_X + (m_RightAligned ? 0 : GetStyle()->m_Width);
	ret.left = ret.right - GetStyle()->m_Width;

	return ret;
}

bool CGUIScrollBarVertical::HoveringButtonMinus(const CPos& mouse)
{
	if (!GetStyle())
		return false;

	float StartX = m_RightAligned ? m_X-GetStyle()->m_Width : m_X;

	return mouse.x >= StartX &&
	       mouse.x <= StartX + GetStyle()->m_Width &&
	       mouse.y >= m_Y &&
	       mouse.y <= m_Y + GetStyle()->m_Width;
}

bool CGUIScrollBarVertical::HoveringButtonPlus(const CPos& mouse)
{
	if (!GetStyle())
		return false;

	float StartX = m_RightAligned ? m_X-GetStyle()->m_Width : m_X;

	return mouse.x > StartX &&
	       mouse.x < StartX + GetStyle()->m_Width &&
	       mouse.y > m_Y + m_Length - GetStyle()->m_Width &&
	       mouse.y < m_Y + m_Length;
}
