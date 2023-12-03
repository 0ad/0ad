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

#ifndef INCLUDED_GUIRENDERER
#define INCLUDED_GUIRENDERER

#include "graphics/Color.h"
#include "graphics/ShaderTechniquePtr.h"
#include "graphics/Texture.h"
#include "maths/Rect.h"
#include "ps/CStrForward.h"
#include "ps/CStrIntern.h"

#include <map>
#include <vector>

class CCanvas2D;
class CGUI;
class CGUISprite;
struct CGUIColor;
struct SGUIImage;

namespace GUIRenderer
{
	struct SDrawCall
	{
		SDrawCall(const SGUIImage* image) : m_Image(image) {}
		CRect ComputeTexCoords() const;

		const SGUIImage* m_Image;

		CTexturePtr m_Texture;

		CRect m_ObjectSize;
		CRect m_Vertices;

		CGUIColor* m_BackColor;

		CColor m_ColorAdd;
		CColor m_ColorMultiply;
		float m_GrayscaleFactor;
	};

	class DrawCalls : public std::vector<SDrawCall>
	{
	public:
		DrawCalls();
		// Copy/assignment results in an empty list, not an actual copy
		DrawCalls(const DrawCalls&);
		DrawCalls& operator=(const DrawCalls&);
	};

	void UpdateDrawCallCache(const CGUI& pGUI, DrawCalls& Calls, const CStr8& SpriteName, const CRect& Size, std::map<CStr8, std::unique_ptr<const CGUISprite>>& Sprites);

	void Draw(DrawCalls& Calls, CCanvas2D& canvas);
}

#endif // INCLUDED_GUIRENDERER
