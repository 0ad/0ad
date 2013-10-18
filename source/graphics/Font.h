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

/**
 * Storage for a bitmap font. Loaded by CFontManager.
 */
class CFont
{
	friend class CFontManager;
	CFont() {}
public:
	struct GlyphData
	{
		float u0, v0, u1, v1;
		i16 x0, y0, x1, y1;
		i16 xadvance;
		u8 defined;
	};

	/**
	 * Relatively efficient lookup of GlyphData from 16-bit Unicode codepoint.
	 * This is stored as a sparse 2D array, exploiting the knowledge that a font
	 * typically only supports a small number of 256-codepoint blocks, so most
	 * elements of m_Data will be NULL.
	 */
	class GlyphMap
	{
		NONCOPYABLE(GlyphMap);
	public:
		GlyphMap();
		~GlyphMap();
		void set(u16 i, const GlyphData& val);

		const GlyphData* get(u16 i) const
		{
			if (!m_Data[i >> 8])
				return NULL;
			if (!m_Data[i >> 8][i & 0xff].defined)
				return NULL;
			return &m_Data[i >> 8][i & 0xff];
		}

	private:
		GlyphData* m_Data[256];
	};

	bool HasRGB() const { return m_HasRGB; }
	int GetLineSpacing() const { return m_LineSpacing; }
	int GetHeight() const { return m_Height; }
	int GetCharacterWidth(wchar_t c) const;
	void CalculateStringSize(const wchar_t* string, int& w, int& h) const;
	void GetGlyphBounds(float& x0, float& y0, float& x1, float& y1) const
	{
		x0 = m_BoundsX0;
		y0 = m_BoundsY0;
		x1 = m_BoundsX1;
		y1 = m_BoundsY1;
	}
	const GlyphMap& GetGlyphs() const { return m_Glyphs; }
	CTexturePtr GetTexture() const { return m_Texture; }

private:
	CTexturePtr m_Texture;

	bool m_HasRGB; // true if RGBA, false if ALPHA

	GlyphMap m_Glyphs;

	int m_LineSpacing;
	int m_Height; // height of a capital letter, roughly

	// Bounding box of all glyphs
	float m_BoundsX0;
	float m_BoundsY0;
	float m_BoundsX1;
	float m_BoundsY1;
};

#endif // INCLUDED_FONT
