/*
IGUIScrollBar
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx.h"
#include "GUI.h"

using namespace std;

void CGUIScrollBarVertical::SetPosFromMousePos(int _x, int _y)
{
	m_Pos = m_PosWhenPressed + ((float)_y-m_BarPressedAtY)/(m_Length-GetStyle().m_Width*2)*2.f;
}

void CGUIScrollBarVertical::Draw()
{
	int StartX = (m_RightAligned)?(m_X-GetStyle().m_Width):(m_X);

	// Draw background
	g_GUI.DrawSprite(GetStyle().m_SpriteScrollBackVertical, m_Z+0.1f, 
					 CRect(	StartX, 
							m_Y+GetStyle().m_Width, 
							StartX+GetStyle().m_Width, 
							m_Y+m_Length-GetStyle().m_Width) 
					);

	// Draw top button
	g_GUI.DrawSprite(GetStyle().m_SpriteButtonTop, m_Z+0.2f, CRect(StartX, m_Y, StartX+GetStyle().m_Width, m_Y+GetStyle().m_Width));
	
	// Draw bottom button
	g_GUI.DrawSprite(GetStyle().m_SpriteButtonBottom, m_Z+0.2f, CRect(StartX, m_Y+m_Length-GetStyle().m_Width, StartX+GetStyle().m_Width, m_Y+m_Length));

	// Draw bar
	if (m_BarPressed)
		g_GUI.DrawSprite(GetStyle().m_SpriteScrollBarVertical, m_Z+0.2f, GetBarRect());
	else
		g_GUI.DrawSprite(GetStyle().m_SpriteScrollBarVertical, m_Z+0.2f, GetBarRect());
}

bool CGUIScrollBarVertical::HandleMessage(const SGUIMessage &Message)
{
	IGUIScrollBar::HandleMessage(Message);

/*	switch (Message.type)
	{

*/	return true;
}

CRect CGUIScrollBarVertical::GetBarRect() const
{
	int size;
	float from, to;

	size = (int)((m_Length-GetStyle().m_Width*2)*m_BarSize);
	from = (float)(m_Y+GetStyle().m_Width);
	to = (float)(m_Y+m_Length-GetStyle().m_Width-size);

	// Setup rectangle
	CRect ret;
	ret.top = (int)(from + (to-from)*m_Pos);
	ret.bottom = ret.top+size;
	ret.right = m_X + ((m_RightAligned)?(0):(GetStyle().m_Width));
	ret.left = ret.right - GetStyle().m_Width;

	return ret;
}

bool CGUIScrollBarVertical::HoveringButtonMinus(int mouse_x, int mouse_y)
{
	int StartX = (m_RightAligned)?(m_X-GetStyle().m_Width):(m_X);

	return (mouse_x > StartX &&
			mouse_x < StartX + GetStyle().m_Width &&
			mouse_y > m_Y &&
			mouse_y < m_Y + GetStyle().m_Width);
}

bool CGUIScrollBarVertical::HoveringButtonPlus(int mouse_x, int mouse_y)
{
	int StartX = (m_RightAligned)?(m_X-GetStyle().m_Width):(m_X);

	return (mouse_x > StartX &&
			mouse_x < StartX + GetStyle().m_Width &&
			mouse_y > m_Y + m_Length - GetStyle().m_Width &&
			mouse_y < m_Y + m_Length);
}
