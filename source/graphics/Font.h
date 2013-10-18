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

#ifndef INCLUDED_FONT
#define INCLUDED_FONT

#include "graphics/Texture.h"
#include "lib/res/handle.h"

#include <map>

class CStrW;
struct UnifontGlyphData;

class CFont
{
	friend class CFontManager;
public:
	struct GlyphData
	{
		float u0, v0, u1, v1;
		i16 x0, y0, x1, y1;
		i16 xadvance;
	};

	typedef std::map<u16, GlyphData> GlyphMap;

	bool HasRGB() const { return m_HasRGB; }
	int GetLineSpacing() const { return m_LineSpacing; }
	int GetHeight() const { return m_Height; }
	int GetCharacterWidth(wchar_t c) const;
	void CalculateStringSize(const wchar_t* string, int& w, int& h) const;
	const GlyphMap& GetGlyphs() const { return m_Glyphs; }
	CTexturePtr GetTexture() const { return m_Texture; }

private:
	CTexturePtr m_Texture;

	bool m_HasRGB; // true if RGBA, false if ALPHA

	GlyphMap m_Glyphs;

	int m_LineSpacing;
	int m_Height; // height of a capital letter, roughly
};

#endif // INCLUDED_FONT
