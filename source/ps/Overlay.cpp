/*
COverlay
by Rich Cross
rich@0ad.wildfiregames.com
*/

#include "Overlay.h"

COverlay::COverlay()
	: m_Rect(CRect(0,0,0,0)), m_Z(0), m_Color(CColor(0,0,0,0)), m_Texture(""), m_HasBorder(false), m_BorderColor(CColor(0,0,0,0))
{
}


COverlay::COverlay(const CRect& rect,int z,const CColor& color,const char* texturename,
				   bool hasBorder,const CColor& bordercolor)
	: m_Rect(rect), m_Z(z), m_Color(color), m_Texture(texturename), m_HasBorder(hasBorder), m_BorderColor(bordercolor)
{
}

COverlay::~COverlay()
{
}