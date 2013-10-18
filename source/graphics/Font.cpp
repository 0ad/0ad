/* Copyright (C) 2013 Wildfire Games.
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
#include "Font.h"

#include "graphics/FontManager.h"
#include "ps/Filesystem.h"
#include "ps/CLogger.h"
#include "renderer/Renderer.h"

#include <map>
#include <string>

int CFont::GetCharacterWidth(wchar_t c) const
{
	GlyphMap::const_iterator it = m_Glyphs.find(c);

	if (it == m_Glyphs.end())
		it = m_Glyphs.find(0xFFFD); // Use the missing glyph symbol

	if (it == m_Glyphs.end())
		return 0;

	return it->second.xadvance;
}

void CFont::CalculateStringSize(const wchar_t* string, int& width, int& height) const
{
	width = 0;
	height = m_Height;

	for (const wchar_t* c = string; *c != '\0'; c++)
	{
		GlyphMap::const_iterator it = m_Glyphs.find(*c);

		if (it == m_Glyphs.end())
			it = m_Glyphs.find(0xFFFD); // Use the missing glyph symbol

		if (it != m_Glyphs.end())
			width += it->second.xadvance; // Add the character's advance distance
	}
}
