#include "precompiled.h"
#include "CGUISprite.h"

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
	m_CachedCellID = -1;
}

bool CGUISpriteInstance::IsEmpty() const
{
	return m_SpriteName=="";
}

// Plus a load of constructors / assignment operators, which don't copy the
// DrawCall cache (to avoid losing track of who has allocated various bits
// of data):

CGUISpriteInstance::CGUISpriteInstance()
	: m_CachedCellID(-1)
{
}

CGUISpriteInstance::CGUISpriteInstance(CStr SpriteName)
: m_SpriteName(SpriteName)
{
}

CGUISpriteInstance::CGUISpriteInstance(const CGUISpriteInstance &Sprite)
: m_SpriteName(Sprite.m_SpriteName)
{
}

CGUISpriteInstance &CGUISpriteInstance::operator=(CStr SpriteName)
{
	m_SpriteName = SpriteName;
	m_DrawCallCache.clear();
	Invalidate();
	return *this;
}

