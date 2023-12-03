/* Copyright (C) 2021 Wildfire Games.
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

#include "CGUISprite.h"

CGUISprite::~CGUISprite() = default;

void CGUISprite::AddImage(std::unique_ptr<SGUIImage> image)
{
	m_Images.emplace_back(std::move(image));
}

void CGUISpriteInstance::Draw(CGUI& pGUI, CCanvas2D& canvas, const CRect& Size, std::map<CStr, std::unique_ptr<const CGUISprite>>& Sprites) const
{
	if (m_CachedSize != Size)
	{
		GUIRenderer::UpdateDrawCallCache(pGUI, m_DrawCallCache, m_SpriteName, Size, Sprites);
		m_CachedSize = Size;
	}
	GUIRenderer::Draw(m_DrawCallCache, canvas);
}

// Plus a load of constructors / assignment operators, which don't copy the
// DrawCall cache (to avoid losing track of who has allocated various bits
// of data):

CGUISpriteInstance::CGUISpriteInstance()
{
}

CGUISpriteInstance::CGUISpriteInstance(const CStr& SpriteName)
	: m_SpriteName(SpriteName)
{
}

void CGUISpriteInstance::SetName(const CStr& SpriteName)
{
	m_SpriteName = SpriteName;
	m_CachedSize = CRect();
	m_DrawCallCache.clear();
}
