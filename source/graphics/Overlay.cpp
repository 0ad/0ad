/* Copyright (C) 2017 Wildfire Games.
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

#include "Overlay.h"

#include "graphics/TextureManager.h"
#include "ps/CStr.h"
#include "renderer/Renderer.h"

SOverlayTexturedLine::LineCapType SOverlayTexturedLine::StrToLineCapType(const std::wstring& str)
{
	if (str == L"round")
		return LINECAP_ROUND;
	else if (str == L"sharp")
		return LINECAP_SHARP;
	else if (str == L"square")
		return LINECAP_SQUARE;
	else if (str == L"flat")
		return LINECAP_FLAT;
	else {
		debug_warn(L"[Overlay] Unrecognized line cap type identifier");
		return LINECAP_FLAT;
	}
}

void SOverlayTexturedLine::CreateOverlayTexture(const SOverlayDescriptor* overlayDescriptor)
{
	CTextureProperties texturePropsBase(overlayDescriptor->m_LineTexture.c_str());
	texturePropsBase.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE);
	texturePropsBase.SetMaxAnisotropy(4.f);

	CTextureProperties texturePropsMask(overlayDescriptor->m_LineTextureMask.c_str());
	texturePropsMask.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE);
	texturePropsMask.SetMaxAnisotropy(4.f);

	m_AlwaysVisible = false;
	m_Closed = true;
	m_Thickness = overlayDescriptor->m_LineThickness;
	m_TextureBase = g_Renderer.GetTextureManager().CreateTexture(texturePropsBase);
	m_TextureMask = g_Renderer.GetTextureManager().CreateTexture(texturePropsMask);

	ENSURE(m_TextureBase);
}
