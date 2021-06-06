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

#include "GUIRenderer.h"

#include "graphics/Canvas2D.h"
#include "graphics/TextureManager.h"
#include "gui/CGUI.h"
#include "gui/CGUISprite.h"
#include "gui/GUIMatrix.h"
#include "gui/SettingTypes/CGUIColor.h"
#include "i18n/L10n.h"
#include "lib/ogl.h"
#include "lib/res/graphics/ogl_tex.h"
#include "lib/res/h_mgr.h"
#include "lib/tex/tex.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "renderer/Renderer.h"

using namespace GUIRenderer;

DrawCalls::DrawCalls()
{
}

// DrawCalls needs to be copyable, so it can be used in other copyable types.
// But actually copying data is hard, since we'd need to avoid losing track of
// who owns various pointers, so instead we just return an empty list.
// The list should get filled in again (by GUIRenderer::UpdateDrawCallCache)
// before it's used for rendering. (TODO: Is this class actually used safely
// in practice?)

DrawCalls::DrawCalls(const DrawCalls&)
	: std::vector<SDrawCall>()
{
}

DrawCalls& DrawCalls::operator=(const DrawCalls&)
{
	return *this;
}


void GUIRenderer::UpdateDrawCallCache(const CGUI& pGUI, DrawCalls& Calls, const CStr& SpriteName, const CRect& Size, std::map<CStr, std::unique_ptr<const CGUISprite>>& Sprites)
{
	// This is called only when something has changed (like the size of the
	// sprite), so it doesn't need to be particularly efficient.

	// Clean up the old data
	Calls.clear();

	// If this object has zero size, there's nothing to render. (This happens
	// with e.g. tooltips that have zero size before they're first drawn, so
	// it isn't necessarily an error.)
	if (Size.left == Size.right && Size.top == Size.bottom)
		return;


	std::map<CStr, std::unique_ptr<const CGUISprite>>::iterator it(Sprites.find(SpriteName));
	if (it == Sprites.end())
	{
		/*
		 * Sprite not found. Check whether this a special sprite,
		 * and if so create a new sprite:
		 * "stretched:filename.ext" - stretched image
		 * "stretched:grayscale:filename.ext" - stretched grayscale image.
		 * "cropped:0.5, 0.25"    - stretch this ratio (x,y) of the top left of the image
		 * "color:r g b a"        - solid color
		 *     > "textureAsMask"  - when using color, use the (optional) texture alpha channel as mask.
		 * These can be combined, but they must be separated by a ":"
		 * so you can have a white overlay over an stretched grayscale image with:
		 * "grayscale:color:255 255 255 100:stretched:filename.ext"
		 */
		// Check that this can be a special sprite.
		if (SpriteName.ReverseFind(":") == -1 && SpriteName.Find("color(") == -1)
		{
			LOGERROR("Trying to use a sprite that doesn't exist (\"%s\").", SpriteName.c_str());
			return;
		}
		auto sprite = std::make_unique<CGUISprite>();
		VfsPath TextureName = VfsPath("art/textures/ui") / wstring_from_utf8(SpriteName.AfterLast(":"));
		if (SpriteName.Find("stretched:") != -1)
		{
			// TODO: Should check (nicely) that this is a valid file?
			auto image = std::make_unique<SGUIImage>();

			image->m_TextureName = TextureName;
			if (SpriteName.Find("grayscale:") != -1)
			{
				image->m_Effects = std::make_shared<SGUIImageEffects>();
				image->m_Effects->m_Greyscale = true;
			}

			sprite->AddImage(std::move(image));
		}
		else if (SpriteName.Find("cropped:") != -1)
		{
			// TODO: Should check (nicely) that this is a valid file?
			auto image = std::make_unique<SGUIImage>();

			const bool centered = SpriteName.Find("center:") != -1;

			CStr info = SpriteName.AfterLast("cropped:").BeforeFirst(":");
			double xRatio = info.BeforeFirst(",").ToDouble();
			double yRatio = info.AfterLast(",").ToDouble();
			const CRect percentSize = centered
				? CRect(50 - 50 / xRatio, 50 - 50 / yRatio, 50 + 50 / xRatio, 50 + 50 / yRatio)
				: CRect(0, 0, 100 / xRatio, 100 / yRatio);
			image->m_TextureSize = CGUISize(CRect(0, 0, 0, 0), percentSize);
			image->m_TextureName = TextureName;

			if (SpriteName.Find("grayscale:") != -1)
			{
				image->m_Effects = std::make_shared<SGUIImageEffects>();
				image->m_Effects->m_Greyscale = true;
			}

			sprite->AddImage(std::move(image));
		}
		if (SpriteName.Find("color:") != -1)
		{
			CStrW value = wstring_from_utf8(SpriteName.AfterLast("color:").BeforeFirst(":"));

			auto image = std::make_unique<SGUIImage>();
			CGUIColor* color;

			// If we are using a mask, this is an effect.
			// Otherwise we can fallback to the "back color" attribute
			// TODO: we are assuming there is a filename here.
			if (SpriteName.Find("textureAsMask:") != -1)
			{
				image->m_TextureName = TextureName;
				image->m_Effects = std::make_shared<SGUIImageEffects>();
				color = &image->m_Effects->m_SolidColor;
			}
			else
				color = &image->m_BackColor;

			// Check color is valid
			if (!CGUI::ParseString<CGUIColor>(&pGUI, value, *color))
			{
				LOGERROR("GUI: Error parsing sprite 'color' (\"%s\")", utf8_from_wstring(value));
				return;
			}

			sprite->AddImage(std::move(image));
		}
		if (sprite->m_Images.empty())
		{
			LOGERROR("Trying to use a sprite that doesn't exist (\"%s\").", SpriteName.c_str());
			return;
		}
		else
			it = Sprites.insert_or_assign(SpriteName, std::move(sprite)).first;
	}

	Calls.reserve(it->second->m_Images.size());

	// Iterate through all the sprite's images, loading the texture and
	// calculating the texture coordinates
	std::vector<std::unique_ptr<SGUIImage>>::const_iterator cit;
	for (cit = it->second->m_Images.begin(); cit != it->second->m_Images.end(); ++cit)
	{
		SDrawCall Call(cit->get()); // pointers are safe since we never modify sprites/images after startup

		CRect ObjectSize = (*cit)->m_Size.GetSize(Size);

		if (ObjectSize.GetWidth() == 0.0 || ObjectSize.GetHeight() == 0.0)
		{
			// Zero sized object. Don't report as an error, since it's common for e.g. hitpoint bars.
			continue; // i.e. don't continue with this image
		}

		Call.m_Vertices = ObjectSize;
		if ((*cit)->m_RoundCoordinates)
		{
			// Round the vertex coordinates to integers, to avoid ugly filtering artifacts
			Call.m_Vertices.left = (int)(Call.m_Vertices.left + 0.5f);
			Call.m_Vertices.right = (int)(Call.m_Vertices.right + 0.5f);
			Call.m_Vertices.top = (int)(Call.m_Vertices.top + 0.5f);
			Call.m_Vertices.bottom = (int)(Call.m_Vertices.bottom + 0.5f);
		}

		bool hasTexture = false;
		if (!(*cit)->m_TextureName.empty())
		{
			CTextureProperties textureProps(g_L10n.LocalizePath((*cit)->m_TextureName));
			textureProps.SetWrap((*cit)->m_WrapMode);
			CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
			texture->Prefetch();
			hasTexture = true;
			Call.m_Texture = texture;
			Call.m_ObjectSize = ObjectSize;
		}

		Call.m_BackColor = &(*cit)->m_BackColor;
		Call.m_GrayscaleFactor = 0.0f;
		if (!hasTexture)
		{
			Call.m_ColorAdd = *Call.m_BackColor;
			Call.m_ColorMultiply = CColor(0.0f, 0.0f, 0.0f, 0.0f);
			Call.m_Texture = g_Renderer.GetTextureManager().GetTransparentTexture();
		}
		else if ((*cit)->m_Effects)
		{
			if ((*cit)->m_Effects->m_AddColor != CGUIColor())
			{
				const CColor color = (*cit)->m_Effects->m_AddColor;
				Call.m_ColorAdd = CColor(color.r, color.g, color.b, 0.0f);
				Call.m_ColorMultiply = CColor(1.0f, 1.0f, 1.0f, 1.0f);
			}
			else if ((*cit)->m_Effects->m_Greyscale)
			{
				Call.m_ColorAdd = CColor(0.0f, 0.0f, 0.0f, 0.0f);
				Call.m_ColorMultiply = CColor(1.0f, 1.0f, 1.0f, 1.0f);
				Call.m_GrayscaleFactor = 1.0f;
			}
			else if ((*cit)->m_Effects->m_SolidColor != CGUIColor())
			{
				const CColor color = (*cit)->m_Effects->m_SolidColor;
				Call.m_ColorAdd = CColor(color.r, color.g, color.b, 0.0f);
				Call.m_ColorMultiply = CColor(0.0f, 0.0f, 0.0f, color.a);
			}
			else /* Slight confusion - why no effects? */
			{
				Call.m_ColorAdd = CColor(0.0f, 0.0f, 0.0f, 0.0f);
				Call.m_ColorMultiply = CColor(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
		else
		{
			Call.m_ColorAdd = CColor(0.0f, 0.0f, 0.0f, 0.0f);
			Call.m_ColorMultiply = CColor(1.0f, 1.0f, 1.0f, 1.0f);
		}

		Calls.push_back(Call);
	}
}

CRect SDrawCall::ComputeTexCoords() const
{
	float TexWidth = m_Texture->GetWidth();
	float TexHeight = m_Texture->GetHeight();

	if (!TexWidth || !TexHeight)
		return CRect(0, 0, 1, 1);

	// Textures are positioned by defining a rectangular block of the
	// texture (usually the whole texture), and a rectangular block on
	// the screen. The texture is positioned to make those blocks line up.

	// Get the screen's position/size for the block
	CRect BlockScreen = m_Image->m_TextureSize.GetSize(m_ObjectSize);

	if (m_Image->m_FixedHAspectRatio)
		BlockScreen.right = BlockScreen.left + BlockScreen.GetHeight() * m_Image->m_FixedHAspectRatio;

	// Get the texture's position/size for the block:
	CRect BlockTex;

	// "real_texture_placement" overrides everything
	if (m_Image->m_TexturePlacementInFile != CRect())
		BlockTex = m_Image->m_TexturePlacementInFile;
	// Use the whole texture
	else
		BlockTex = CRect(0, 0, TexWidth, TexHeight);

	// When rendering, BlockTex will be transformed onto BlockScreen.
	// Also, TexCoords will be transformed onto ObjectSize (giving the
	// UV coords at each vertex of the object). We know everything
	// except for TexCoords, so calculate it:

	CVector2D translation(BlockTex.TopLeft()-BlockScreen.TopLeft());
	float ScaleW = BlockTex.GetWidth()/BlockScreen.GetWidth();
	float ScaleH = BlockTex.GetHeight()/BlockScreen.GetHeight();

	CRect TexCoords (
				// Resize (translating to/from the origin, so the
				// topleft corner stays in the same place)
				(m_ObjectSize-m_ObjectSize.TopLeft())
				.Scale(ScaleW, ScaleH)
				+ m_ObjectSize.TopLeft()
				// Translate from BlockTex to BlockScreen
				+ translation
	);

	// The tex coords need to be scaled so that (texwidth,texheight) is
	// mapped onto (1,1)
	TexCoords.left   /= TexWidth;
	TexCoords.right  /= TexWidth;
	TexCoords.top    /= TexHeight;
	TexCoords.bottom /= TexHeight;

	return TexCoords;
}

void GUIRenderer::Draw(DrawCalls& Calls, CCanvas2D& canvas)
{
	if (Calls.empty())
		return;

	// Called every frame, to draw the object (based on cached calculations)

	// Iterate through each DrawCall, and execute whatever drawing code is being called
	for (DrawCalls::const_iterator cit = Calls.begin(); cit != Calls.end(); ++cit)
	{
		// A hack to preload the handle to get a correct texture size.
		GLuint h;
		ogl_tex_get_texture_id(cit->m_Texture->GetHandle(), &h);

		CRect texCoords = cit->ComputeTexCoords().Scale(
			cit->m_Texture->GetWidth(), cit->m_Texture->GetHeight());

		// Ensure the quad has the correct winding order
		CRect rect = cit->m_Vertices;
		if (rect.right < rect.left)
		{
			std::swap(rect.right, rect.left);
			std::swap(texCoords.right, texCoords.left);
		}
		if (rect.bottom < rect.top)
		{
			std::swap(rect.bottom, rect.top);
			std::swap(texCoords.bottom, texCoords.top);
		}

		canvas.DrawTexture(cit->m_Texture,
			rect, texCoords, cit->m_ColorMultiply, cit->m_ColorAdd, cit->m_GrayscaleFactor);
	}
}
