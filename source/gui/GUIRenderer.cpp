/* Copyright (C) 2014 Wildfire Games.
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

#include "graphics/ShaderManager.h"
#include "graphics/TextureManager.h"
#include "gui/GUIutil.h"
#include "i18n/L10n.h"
#include "lib/ogl.h"
#include "lib/utf8.h"
#include "lib/res/h_mgr.h"
#include "lib/tex/tex.h"
#include "ps/CLogger.h"
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


void GUIRenderer::UpdateDrawCallCache(DrawCalls &Calls, const CStr& SpriteName, const CRect &Size, int CellID, std::map<CStr, CGUISprite*> &Sprites)
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


	std::map<CStr, CGUISprite*>::iterator it (Sprites.find(SpriteName));
	if (it == Sprites.end())
	{
		// Sprite not found. Check whether this a special sprite:
		//     "stretched:filename.ext" - stretched image
		//     "stretched:grayscale:filename.ext" - stretched grayscale image
		//     "cropped:(0.5, 0.25)"    - stretch this ratio (x,y) of the top left of the image
		//     "colour:r g b a"         - solid colour
		//
		// and if so, try to create it as a new sprite.
		if (SpriteName.substr(0, 10) == "stretched:")
		{
			// TODO: Should check (nicely) that this is a valid file?
			SGUIImage* Image = new SGUIImage;
			
			// Allow grayscale images for disabled portraits
			if (SpriteName.substr(10, 10) == "grayscale:")
			{
				Image->m_TextureName = VfsPath("art/textures/ui") / wstring_from_utf8(SpriteName.substr(20));
				Image->m_Effects = new SGUIImageEffects;
				Image->m_Effects->m_Greyscale = true;
			}
			else
			{
				Image->m_TextureName = VfsPath("art/textures/ui") / wstring_from_utf8(SpriteName.substr(10));
			}

			CClientArea ca(CRect(0, 0, 0, 0), CRect(0, 0, 100, 100));
			Image->m_Size = ca;
			Image->m_TextureSize = ca;

			CGUISprite* Sprite = new CGUISprite;
			Sprite->AddImage(Image);

			Sprites[SpriteName] = Sprite;
			
			it = Sprites.find(SpriteName);
			ENSURE(it != Sprites.end()); // The insertion above shouldn't fail
		}
		else if (SpriteName.substr(0, 8) == "cropped:")
		{
			// TODO: Should check (nicely) that this is a valid file?
			SGUIImage* Image = new SGUIImage;

			double xRatio = SpriteName.BeforeFirst(",").AfterLast("(").ToDouble();
			double yRatio = SpriteName.BeforeFirst(")").AfterLast(",").ToDouble();
			
			int PathStart = SpriteName.Find(")") + 1;
			
			Image->m_TextureName = VfsPath("art/textures/ui") / wstring_from_utf8(SpriteName.substr(PathStart));

			CClientArea ca(CRect(0, 0, 0, 0), CRect(0, 0, 100, 100));
			CClientArea cb(CRect(0, 0, 0, 0), CRect(0, 0, 100/xRatio, 100/yRatio));
			Image->m_Size = ca;
			Image->m_TextureSize = cb;

			CGUISprite* Sprite = new CGUISprite;
			Sprite->AddImage(Image);

			Sprites[SpriteName] = Sprite;
			
			it = Sprites.find(SpriteName);
			ENSURE(it != Sprites.end()); // The insertion above shouldn't fail
		}
		else if (SpriteName.substr(0, 7) == "colour:")
		{
			CStrW value = wstring_from_utf8(SpriteName.substr(7));
			CColor color;

			// Check colour is valid
			if (!GUI<CColor>::ParseString(value, color))
			{
				LOGERROR(L"GUI: Error parsing sprite 'colour' (\"%ls\")", value.c_str());
				return;
			}

			SGUIImage* Image = new SGUIImage;

			Image->m_BackColor = color;

			CClientArea ca(CRect(0, 0, 0, 0), CRect(0, 0, 100, 100));
			Image->m_Size = ca;
			Image->m_TextureSize = ca;

			CGUISprite* Sprite = new CGUISprite;
			Sprite->AddImage(Image);

			Sprites[SpriteName] = Sprite;

			it = Sprites.find(SpriteName);
			ENSURE(it != Sprites.end()); // The insertion above shouldn't fail
		}
		else
		{
			// Otherwise, just complain and give up:
			LOGERROR(L"Trying to use a sprite that doesn't exist (\"%hs\").", SpriteName.c_str());
			return;
		}
	}

	Calls.reserve(it->second->m_Images.size());

	// Iterate through all the sprite's images, loading the texture and
	// calculating the texture coordinates
	std::vector<SGUIImage*>::const_iterator cit;
	for (cit = it->second->m_Images.begin(); cit != it->second->m_Images.end(); ++cit)
	{
		SDrawCall Call(*cit); // pointers are safe since we never modify sprites/images after startup

		CRect ObjectSize = (*cit)->m_Size.GetClientArea(Size);

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

		if (!(*cit)->m_TextureName.empty())
		{
			CTextureProperties textureProps(L10n::Instance().LocalizePath((*cit)->m_TextureName));
			textureProps.SetWrap((*cit)->m_WrapMode);
			CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
			texture->Prefetch();
			Call.m_HasTexture = true;
			Call.m_Texture = texture;

			Call.m_EnableBlending = false; // will be overridden if the texture has an alpha channel

			Call.m_ObjectSize = ObjectSize;
			Call.m_CellID = CellID;
		}
		else
		{
			Call.m_HasTexture = false;
			// Enable blending if it's transparent (allowing a little error in the calculations)
			Call.m_EnableBlending = !(fabs((*cit)->m_BackColor.a - 1.0f) < 0.0000001f);
		}

		Call.m_BackColor = (*cit)->m_BackColor;
		Call.m_BorderColor = (*cit)->m_Border ? (*cit)->m_BorderColor : CColor();
		Call.m_DeltaZ = (*cit)->m_DeltaZ;

		if (!Call.m_HasTexture)
		{
			Call.m_Shader = g_Renderer.GetShaderManager().LoadEffect(str_gui_solid);
		}
		else if ((*cit)->m_Effects)
		{
			if ((*cit)->m_Effects->m_AddColor != CColor())
			{
				Call.m_Shader = g_Renderer.GetShaderManager().LoadEffect(str_gui_add);
				Call.m_ShaderColorParameter = (*cit)->m_Effects->m_AddColor;
				// Always enable blending if something's being subtracted from
				// the alpha channel
				if ((*cit)->m_Effects->m_AddColor.a < 0.f)
					Call.m_EnableBlending = true;
			}
			else if ((*cit)->m_Effects->m_Greyscale)
			{
				Call.m_Shader = g_Renderer.GetShaderManager().LoadEffect(str_gui_grayscale);
			}
			else /* Slight confusion - why no effects? */
			{
				Call.m_Shader = g_Renderer.GetShaderManager().LoadEffect(str_gui_basic);
			}
		}
		else
		{
			Call.m_Shader = g_Renderer.GetShaderManager().LoadEffect(str_gui_basic);
		}

		Calls.push_back(Call);
	}
}

CRect SDrawCall::ComputeTexCoords() const
{
	float TexWidth = m_Texture->GetWidth();
	float TexHeight = m_Texture->GetHeight();

	if (!TexWidth || !TexHeight)
	{
		return CRect(0, 0, 1, 1);
	}

	// Textures are positioned by defining a rectangular block of the
	// texture (usually the whole texture), and a rectangular block on
	// the screen. The texture is positioned to make those blocks line up.

	// Get the screen's position/size for the block
	CRect BlockScreen = m_Image->m_TextureSize.GetClientArea(m_ObjectSize);

	if (m_Image->m_FixedHAspectRatio)
		BlockScreen.right = BlockScreen.left + BlockScreen.GetHeight() * m_Image->m_FixedHAspectRatio;

	// Get the texture's position/size for the block:
	CRect BlockTex;

	// "real_texture_placement" overrides everything
	if (m_Image->m_TexturePlacementInFile != CRect())
	{
		BlockTex = m_Image->m_TexturePlacementInFile;
	}
	// Check whether this sprite has "cell_size" set (and non-zero)
	else if ((int)m_Image->m_CellSize.cx)
	{
		int cols = (int)TexWidth / (int)m_Image->m_CellSize.cx;
		if (cols == 0)
			cols = 1; // avoid divide-by-zero
		int col = m_CellID % cols;
		int row = m_CellID / cols;
		BlockTex = CRect(m_Image->m_CellSize.cx*col, m_Image->m_CellSize.cy*row,
		                 m_Image->m_CellSize.cx*(col+1), m_Image->m_CellSize.cy*(row+1));
	}
	// Use the whole texture
	else
		BlockTex = CRect(0, 0, TexWidth, TexHeight);

	// When rendering, BlockTex will be transformed onto BlockScreen.
	// Also, TexCoords will be transformed onto ObjectSize (giving the
	// UV coords at each vertex of the object). We know everything
	// except for TexCoords, so calculate it:

	CPos translation (BlockTex.TopLeft()-BlockScreen.TopLeft());
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

void GUIRenderer::Draw(DrawCalls &Calls, float Z)
{
	// Called every frame, to draw the object (based on cached calculations)

	// TODO: batching by shader/texture/etc would be nice

	CMatrix3D matrix = GetDefaultGuiMatrix();

	glDisable(GL_BLEND);

	// Set LOD bias so mipmapped textures are prettier
#if CONFIG2_GLES
#warning TODO: implement GUI LOD bias for GLES
#else
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -1.f);
#endif

	// Iterate through each DrawCall, and execute whatever drawing code is being called
	for (DrawCalls::const_iterator cit = Calls.begin(); cit != Calls.end(); ++cit)
	{
		cit->m_Shader->BeginPass();
		CShaderProgramPtr shader = cit->m_Shader->GetShader();
		shader->Uniform(str_transform, matrix);

		if (cit->m_HasTexture)
		{
			shader->Uniform(str_color, cit->m_ShaderColorParameter);
			shader->BindTexture(str_tex, cit->m_Texture);

			if (cit->m_EnableBlending || cit->m_Texture->HasAlpha()) // (shouldn't call HasAlpha before BindTexture)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}

			CRect TexCoords = cit->ComputeTexCoords();

			// Ensure the quad has the correct winding order, and update texcoords to match
			CRect Verts = cit->m_Vertices;
			if (Verts.right < Verts.left)
			{
				std::swap(Verts.right, Verts.left);
				std::swap(TexCoords.right, TexCoords.left);
			}
			if (Verts.bottom < Verts.top)
			{
				std::swap(Verts.bottom, Verts.top);
				std::swap(TexCoords.bottom, TexCoords.top);
			}

			std::vector<float> data;
#define ADD(u, v, x, y, z) STMT(data.push_back(u); data.push_back(v); data.push_back(x); data.push_back(y); data.push_back(z))
			ADD(TexCoords.left, TexCoords.bottom, Verts.left, Verts.bottom, Z + cit->m_DeltaZ);
			ADD(TexCoords.right, TexCoords.bottom, Verts.right, Verts.bottom, Z + cit->m_DeltaZ);
			ADD(TexCoords.right, TexCoords.top, Verts.right, Verts.top, Z + cit->m_DeltaZ);

			ADD(TexCoords.right, TexCoords.top, Verts.right, Verts.top, Z + cit->m_DeltaZ);
			ADD(TexCoords.left, TexCoords.top, Verts.left, Verts.top, Z + cit->m_DeltaZ);
			ADD(TexCoords.left, TexCoords.bottom, Verts.left, Verts.bottom, Z + cit->m_DeltaZ);
#undef ADD

			shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 5*sizeof(float), &data[0]);
			shader->VertexPointer(3, GL_FLOAT, 5*sizeof(float), &data[2]);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		else
		{
			shader->Uniform(str_color, cit->m_BackColor);

			if (cit->m_EnableBlending)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}

			// Ensure the quad has the correct winding order
			CRect Verts = cit->m_Vertices;
			if (Verts.right < Verts.left)
				std::swap(Verts.right, Verts.left);
			if (Verts.bottom < Verts.top)
				std::swap(Verts.bottom, Verts.top);

			std::vector<float> data;
#define ADD(x, y, z) STMT(data.push_back(x); data.push_back(y); data.push_back(z))
			ADD(Verts.left, Verts.bottom, Z + cit->m_DeltaZ);
			ADD(Verts.right, Verts.bottom, Z + cit->m_DeltaZ);
			ADD(Verts.right, Verts.top, Z + cit->m_DeltaZ);

			ADD(Verts.right, Verts.top, Z + cit->m_DeltaZ);
			ADD(Verts.left, Verts.top, Z + cit->m_DeltaZ);
			ADD(Verts.left, Verts.bottom, Z + cit->m_DeltaZ);

			shader->VertexPointer(3, GL_FLOAT, 3*sizeof(float), &data[0]);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			if (cit->m_BorderColor != CColor())
			{
				shader->Uniform(str_color, cit->m_BorderColor);

				data.clear();
				ADD(Verts.left + 0.5f, Verts.top + 0.5f, Z + cit->m_DeltaZ);
				ADD(Verts.right - 0.5f, Verts.top + 0.5f, Z + cit->m_DeltaZ);
				ADD(Verts.right - 0.5f, Verts.bottom - 0.5f, Z + cit->m_DeltaZ);
				ADD(Verts.left + 0.5f, Verts.bottom - 0.5f, Z + cit->m_DeltaZ);

				shader->VertexPointer(3, GL_FLOAT, 3*sizeof(float), &data[0]);
				glDrawArrays(GL_LINE_LOOP, 0, 4);
			}
#undef ADD
		}
		
		cit->m_Shader->EndPass();

		glDisable(GL_BLEND);
	}

#if CONFIG2_GLES
#warning TODO: implement GUI LOD bias for GLES
#else
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0.f);
#endif
}
