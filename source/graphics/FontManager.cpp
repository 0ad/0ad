/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "FontManager.h"

#include "graphics/Font.h"
#include "graphics/TextureManager.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "renderer/Renderer.h"

#include <limits>

std::shared_ptr<CFont> CFontManager::LoadFont(CStrIntern fontName)
{
	FontsMap::iterator it = m_Fonts.find(fontName);
	if (it != m_Fonts.end())
		return it->second;

	std::shared_ptr<CFont> font(new CFont());

	if (!ReadFont(font.get(), fontName))
	{
		// Fall back to default font (unless this is the default font)
		if (fontName == str_sans_10)
			font.reset();
		else
			font = LoadFont(str_sans_10);
	}

	m_Fonts[fontName] = font;
	return font;
}

bool CFontManager::ReadFont(CFont* font, CStrIntern fontName)
{
	const VfsPath path(L"fonts/");

	// Read font definition file into a stringstream
	std::shared_ptr<u8> buffer;
	size_t size;
	const VfsPath fntName(fontName.string() + ".fnt");
	if (g_VFS->LoadFile(path / fntName, buffer, size) < 0)
	{
		LOGERROR("Failed to open font file %s", (path / fntName).string8());
		return false;
	}
	std::istringstream fontStream(
		std::string(reinterpret_cast<const char*>(buffer.get()), size));

	int version;
	fontStream >> version;
	// Make sure this is from a recent version of the font builder.
	if (version != 101)
	{
		LOGERROR("Font %s has invalid version", fontName.c_str());
		return false;
	}

	int textureWidth, textureHeight;
	fontStream >> textureWidth >> textureHeight;

	std::string format;
	fontStream >> format;
	if (format == "rgba")
		font->m_HasRGB = true;
	else if (format == "a")
		font->m_HasRGB = false;
	else
	{
		LOGWARNING("Invalid .fnt format string");
		return false;
	}

	int mumberOfGlyphs;
	fontStream >> mumberOfGlyphs;

	fontStream >> font->m_LineSpacing;
	fontStream >> font->m_Height;

	font->m_BoundsX0 = std::numeric_limits<float>::max();
	font->m_BoundsY0 = std::numeric_limits<float>::max();
	font->m_BoundsX1 = -std::numeric_limits<float>::max();
	font->m_BoundsY1 = -std::numeric_limits<float>::max();

	for (int i = 0; i < mumberOfGlyphs; ++i)
	{
		int codepoint, textureX, textureY, width, height, offsetX, offsetY, advance;
		fontStream >> codepoint
			>> textureX >> textureY >> width >> height
			>> offsetX >> offsetY >> advance;

		if (codepoint < 0 || codepoint > 0xFFFF)
		{
			LOGWARNING("Font %s has invalid codepoint 0x%x", fontName.c_str(), codepoint);
			continue;
		}

		const float u = static_cast<float>(textureX) / textureWidth;
		const float v = static_cast<float>(textureY) / textureHeight;
		const float w = static_cast<float>(width) / textureWidth;
		const float h = static_cast<float>(height) / textureHeight;

		CFont::GlyphData g =
		{
			u, -v, u + w, -v + h,
			static_cast<i16>(offsetX), static_cast<i16>(-offsetY),
			static_cast<i16>(offsetX + width), static_cast<i16>(-offsetY + height),
			static_cast<i16>(advance)
		};
		font->m_Glyphs.set(static_cast<u16>(codepoint), g);

		font->m_BoundsX0 = std::min(font->m_BoundsX0, static_cast<float>(g.x0));
		font->m_BoundsY0 = std::min(font->m_BoundsY0, static_cast<float>(g.y0));
		font->m_BoundsX1 = std::max(font->m_BoundsX1, static_cast<float>(g.x1));
		font->m_BoundsY1 = std::max(font->m_BoundsY1, static_cast<float>(g.y1));
	}

	// Ensure the height has been found (which should always happen if the font includes an 'I').
	ENSURE(font->m_Height);

	// Load glyph texture
	const VfsPath imageName(fontName.string() + ".png");
	CTextureProperties textureProps(path / imageName,
		font->m_HasRGB ? Renderer::Backend::Format::R8G8B8A8_UNORM : Renderer::Backend::Format::A8_UNORM);
	textureProps.SetIgnoreQuality(true);
	font->m_Texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);

	return true;
}
