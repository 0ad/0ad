/*
COverlayText
by Rich Cross
rich@0ad.wildfiregames.com
*/

#include "precompiled.h"

#include "OverlayText.h"
#include "NPFont.h"
#include "NPFontManager.h"

COverlayText::COverlayText()
	: m_X(0), m_Y(0), m_Z(0), m_Color(CColor(0,0,0,0)), m_Font(0), m_String("")
{
}


COverlayText::COverlayText(float x,float y,int z,const char* fontname,const char* string,const CColor& color)
	: m_X(x), m_Y(y), m_Z(z), m_Color(color), m_String(string)
{
	m_Font=NPFontManager::instance().add(fontname);
}

COverlayText::~COverlayText()
{
}

bool COverlayText::GetOutputStringSize(int& sx,int& sy) 
{
	if (!m_Font) return false;
	m_Font->GetOutputStringSize((const char*) m_String,sx,sy);
	return true;
}
