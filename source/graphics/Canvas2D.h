/* Copyright (C) 2022 Wildfire Games.
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

#ifndef INCLUDED_CANVAS2D
#define INCLUDED_CANVAS2D

#include "graphics/Texture.h"
#include "maths/Vector2D.h"

#include <memory>
#include <vector>

class CRect;
class CTextRenderer;

struct CColor;

// Encapsulates 2D drawing functionality to hide and optimize
// low level API calls.
class CCanvas2D
{
public:
	CCanvas2D();
	~CCanvas2D();

	CCanvas2D(const CCanvas2D&) = delete;
	CCanvas2D& operator=(const CCanvas2D&) = delete;
	CCanvas2D(CCanvas2D&&) = delete;
	CCanvas2D& operator=(CCanvas2D&&) = delete;

	/**
	 * Draws a line by the given points with the width and color.
	 */
	void DrawLine(
		const std::vector<CVector2D>& points,
		const float width, const CColor& color);

	/**
	 * Draws the rect filled with the color.
	 */
	void DrawRect(const CRect& rect, const CColor& color);

	/**
	 * Draws a piece of the texture from the source rect into the
	 * destination rect. The result color is set by the following formula:
	 *   TEXTURE_COLOR * COLOR_MULTIPLY + COLOR_ADD
	 * The texture color is blended with its own grayscale version according to
	 * the grayscale factor.
	 */
	void DrawTexture(CTexturePtr texture,
		const CRect& destination, const CRect& source,
		const CColor& multiply, const CColor& add, const float grayscaleFactor);

	/**
	 * A simpler version of the previous one, draws the texture into the
	 * destination rect without color modifications.
	 */
	void DrawTexture(CTexturePtr texture, const CRect& destination);

	/**
	 * Draws a text using canvas materials.
	 */
	void DrawText(CTextRenderer& textRenderer);

	/**
	 * Unbinds all binded resources and clears caches. Frequent calls might
	 * affect performance. Useful to call a custom rendering code.
	 */
	void Flush();

private:
	class Impl;
	std::unique_ptr<Impl> m;
};

#endif // INCLUDED_CANVAS2D
