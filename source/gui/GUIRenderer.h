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

#ifndef INCLUDED_GUIRENDERER
#define INCLUDED_GUIRENDERER

#include "graphics/Color.h"
#include "graphics/ShaderTechniquePtr.h"
#include "graphics/Texture.h"
#include "lib/res/handle.h"
#include "maths/Rect.h"

#include <map>
#include <vector>

class CGUI;
class CGUISprite;
class CStr8;
struct CGUIColor;
struct SGUIImage;

namespace GUIRenderer
{
	struct SDrawCall
	{
		SDrawCall(const SGUIImage* image) : m_Image(image) {}
		CRect ComputeTexCoords() const;

		const SGUIImage* m_Image;

		bool m_HasTexture;
		CTexturePtr m_Texture;

		CRect m_ObjectSize;

		bool m_EnableBlending;

		CShaderTechniquePtr m_Shader;
		CColor m_ShaderColorParameter;

		CRect m_Vertices;

		CGUIColor* m_BorderColor; // == nullptr for no border
		CGUIColor* m_BackColor;
	};

	class DrawCalls : public std::vector<SDrawCall>
	{
	public:
		DrawCalls();
		// Copy/assignment results in an empty list, not an actual copy
		DrawCalls(const DrawCalls&);
		DrawCalls& operator=(const DrawCalls&);
	};

	void UpdateDrawCallCache(const CGUI& pGUI, DrawCalls& Calls, const CStr8& SpriteName, const CRect& Size, std::map<CStr8, const CGUISprite*>& Sprites);

	void Draw(DrawCalls& Calls);
}

#endif // INCLUDED_GUIRENDERER
