#include "precompiled.h"
#include "CGUISprite.h"

CGUISpriteInstance::CGUISpriteInstance()
{
}

CGUISpriteInstance::CGUISpriteInstance(CStr SpriteName)
	: m_SpriteName(SpriteName)
{
}

CGUISpriteInstance &CGUISpriteInstance::operator=(CStr SpriteName)
{
	m_SpriteName = SpriteName;
	Invalidate();
	return *this;
}

void CGUISpriteInstance::Draw(CRect Size, int CellID, std::map<CStr, CGUISprite> &Sprites)
{
	if (m_CachedSize != Size || m_CachedCellID != CellID)
	{
		GUIRenderer::UpdateDrawCallCache(m_DrawCallCache, m_SpriteName, Size, CellID, Sprites);
		m_CachedSize = Size;
		m_CachedCellID = CellID;
	}
	GUIRenderer::Draw(m_DrawCallCache);
}

void CGUISpriteInstance::Invalidate()
{
	m_CachedSize = CRect();
}

bool CGUISpriteInstance::IsEmpty() const
{
	return m_SpriteName=="";
}
