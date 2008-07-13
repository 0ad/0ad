/*
IGUIScrollBar
*/

#include "precompiled.h"
#include "GUI.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "gui"


CGUIScrollBarVertical::CGUIScrollBarVertical()
{
}

CGUIScrollBarVertical::~CGUIScrollBarVertical()
{
}

void CGUIScrollBarVertical::SetPosFromMousePos(const CPos &mouse)
{
	if (!GetStyle())
		return;

	m_Pos = (m_PosWhenPressed + m_ScrollRange*(mouse.y-m_BarPressedAtPos.y)/(m_Length-GetStyle()->m_Width*2));
}

void CGUIScrollBarVertical::Draw()
{
	if (!GetStyle())
	{
		// TODO Gee: Report in error log
		return;
	}

	if (GetGUI())
	{
		CRect outline = GetOuterRect();

		// Draw background
		GetGUI()->DrawSprite(GetStyle()->m_SpriteBackVertical,
							 0,
							 m_Z+0.1f, 
							 CRect(outline.left,
								   outline.top+(m_UseEdgeButtons?GetStyle()->m_Width:0),
								   outline.right,
								   outline.bottom-(m_UseEdgeButtons?GetStyle()->m_Width:0))
							 );

		if (m_UseEdgeButtons)
		{
			// Get Appropriate sprites
			CGUISpriteInstance *button_top, *button_bottom;

			// figure out what sprite to use for top button
			if (m_ButtonMinusHovered)
			{
				if (m_ButtonMinusPressed)
					button_top = &GUI<>::FallBackSprite(GetStyle()->m_SpriteButtonTopPressed, GetStyle()->m_SpriteButtonTop);
				else
					button_top = &GUI<>::FallBackSprite(GetStyle()->m_SpriteButtonTopOver, GetStyle()->m_SpriteButtonTop);
			}
			else button_top = &GetStyle()->m_SpriteButtonTop;

			// figure out what sprite to use for top button
			if (m_ButtonPlusHovered)
			{
				if (m_ButtonPlusPressed)
					button_bottom = &GUI<>::FallBackSprite(GetStyle()->m_SpriteButtonBottomPressed, GetStyle()->m_SpriteButtonBottom);
				else
					button_bottom = &GUI<>::FallBackSprite(GetStyle()->m_SpriteButtonBottomOver, GetStyle()->m_SpriteButtonBottom);
			}
			else button_bottom = &GetStyle()->m_SpriteButtonBottom;
			
			// Draw top button
			GetGUI()->DrawSprite(*button_top,
								 0,
								 m_Z+0.2f,
								 CRect(outline.left,
									   outline.top,
									   outline.right,
									   outline.top+GetStyle()->m_Width)
								);
			
			// Draw bottom button
			GetGUI()->DrawSprite(*button_bottom,
								 0,
								 m_Z+0.2f,
								 CRect(outline.left,
									   outline.bottom-GetStyle()->m_Width,
									   outline.right,
									   outline.bottom)
								);
		}

		// Draw bar
		/*if (m_BarPressed)
			GetGUI()->DrawSprite(GUI<>::FallBackSprite(GetStyle()->m_SpriteBarVerticalPressed, GetStyle()->m_SpriteBarVertical),
								 0,
								 m_Z+0.2f,
								 GetBarRect());
		else
		if (m_BarHovered)
			GetGUI()->DrawSprite(GUI<>::FallBackSprite(GetStyle()->m_SpriteBarVerticalOver, GetStyle()->m_SpriteBarVertical),
								 0,
								 m_Z+0.2f,
								 GetBarRect());
		else*/
			GetGUI()->DrawSprite(GetStyle()->m_SpriteBarVertical,
								 0,
								 m_Z+0.2f,
								 GetBarRect());
	}
} 

void CGUIScrollBarVertical::HandleMessage(const SGUIMessage &Message)
{
	IGUIScrollBar::HandleMessage(Message);
}

CRect CGUIScrollBarVertical::GetBarRect() const
{
	CRect ret;
	if (!GetStyle())
		return ret;

	float size;
	float from, to;

	// is edge buttons used?
	if (m_UseEdgeButtons)
	{
		size = (m_Length-GetStyle()->m_Width*2.f)*m_BarSize;
		if (size < GetStyle()->m_MinimumBarSize)
			size = GetStyle()->m_MinimumBarSize;

		from = m_Y+GetStyle()->m_Width;
		to = m_Y+m_Length-GetStyle()->m_Width-size;
	}
	else
	{
		size = m_Length*m_BarSize;
		if (size < GetStyle()->m_MinimumBarSize)
			size = GetStyle()->m_MinimumBarSize;

		from = m_Y;
		to = m_Y+m_Length-size;
	}

	// Setup rectangle
	ret.top = (from + (to-from)*(m_Pos/(std::max(1.f, m_ScrollRange - m_ScrollSpace))));
	ret.bottom = ret.top+size;
	ret.right = m_X + ((m_RightAligned)?(0.f):(GetStyle()->m_Width));
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
	ret.right = m_X + ((m_RightAligned)?(0):(GetStyle()->m_Width));
	ret.left = ret.right - GetStyle()->m_Width;

	return ret;
}

bool CGUIScrollBarVertical::HoveringButtonMinus(const CPos &mouse)
{
	if (!GetStyle())
		return false;

	float StartX = (m_RightAligned)?(m_X-GetStyle()->m_Width):(m_X);

	return (mouse.x >= StartX &&
			mouse.x <= StartX + GetStyle()->m_Width &&
			mouse.y >= m_Y &&
			mouse.y <= m_Y + GetStyle()->m_Width);
}

bool CGUIScrollBarVertical::HoveringButtonPlus(const CPos &mouse)
{
	if (!GetStyle())
		return false;

	float StartX = (m_RightAligned)?(m_X-GetStyle()->m_Width):(m_X);

	return (mouse.x > StartX &&
			mouse.x < StartX + GetStyle()->m_Width &&
			mouse.y > m_Y + m_Length - GetStyle()->m_Width &&
			mouse.y < m_Y + m_Length);
}
