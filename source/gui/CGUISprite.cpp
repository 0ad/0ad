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

#include "CGUISprite.h"

CGUISprite::~CGUISprite()
{
	for (SGUIImage* const& img : m_Images)
		delete img;
}

void CGUISprite::AddImage(SGUIImage* image)
{
	m_Images.push_back(image);
}

void CGUISpriteInstance::Draw(const CGUI* pGUI, const CRect& Size, int CellID, std::map<CStr, const CGUISprite*>& Sprites, float Z) const
{
	if (m_CachedSize != Size || m_CachedCellID != CellID)
	{
		GUIRenderer::UpdateDrawCallCache(pGUI, m_DrawCallCache, m_SpriteName, Size, CellID, Sprites);
		m_CachedSize = Size;
		m_CachedCellID = CellID;
	}
	GUIRenderer::Draw(m_DrawCallCache, Z);
}

// Plus a load of constructors / assignment operators, which don't copy the
// DrawCall cache (to avoid losing track of who has allocated various bits
// of data):

CGUISpriteInstance::CGUISpriteInstance()
	: m_CachedCellID(-1)
{
}

CGUISpriteInstance::CGUISpriteInstance(const CStr& SpriteName)
	: m_SpriteName(SpriteName), m_CachedCellID(-1)
{
}

void CGUISpriteInstance::SetName(const CStr& SpriteName)
{
	m_SpriteName = SpriteName;
	m_CachedSize = CRect();
	m_DrawCallCache.clear();
	m_CachedCellID = -1;
}
