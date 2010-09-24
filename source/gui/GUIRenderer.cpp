/* Copyright (C) 2010 Wildfire Games.
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

#include "graphics/TextureManager.h"
#include "lib/ogl.h"
#include "lib/utf8.h"
#include "lib/res/h_mgr.h"
#include "lib/tex/tex.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "renderer/Renderer.h"

#define LOG_CATEGORY L"gui"

using namespace GUIRenderer;


void DrawCalls::clear()
{
	for (iterator it = begin(); it != end(); ++it)
	{
		delete it->m_Effects;
	}
	std::vector<SDrawCall>::clear();
}

DrawCalls::DrawCalls()
{
}

DrawCalls::~DrawCalls()
{
	clear();
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

const DrawCalls& DrawCalls::operator=(const DrawCalls&)
{
	return *this;
}



// Implementations of graphical effects:


const GLint TexScale1[3] = { 1, 1, 1 };
const GLint TexScale2[3] = { 2, 2, 2 };
const GLint TexScale4[3] = { 4, 4, 4 };

class Effect_AddColor : public IGLState
{
	// Uses GL_COMBINE and GL_ADD/GL_SUBTRACT/GL_ADD_SIGNED, to allow
	// addition/subtraction of colors.

public:
	Effect_AddColor(CColor c)
	{
		// If everything's in [0,1], use GL_ADD
#define RANGE(lo,hi) c.r >= lo && c.r <= hi && c.g >= lo && c.g <= hi && c.b >= lo && c.b <= hi && c.a >= lo && c.a <= hi
		if (RANGE(0.f, 1.f))
		{
			m_Color = c;
			m_Method = ADD_NORMAL;
		}
		// If it's in [-1, 0] use GL_SUBTRACT
		else if (RANGE(-1.f, 0.f))
		{
			m_Color = CColor(-c.r, -c.g, -c.b, -c.a);
			m_Method = ADD_SUBTRACT;
		}
		// If it's in [-0.5, 0.5] use GL_ADD_SIGNED
		else if (RANGE(-0.5f, 0.5f))
		{
			m_Color = CColor(c.r+0.5f, c.g+0.5f, c.b+0.5f, c.a+0.5f);
			m_Method = ADD_SIGNED;
		}
		// Otherwise, complain.
		else
		{
			LOG(CLogger::Warning, LOG_CATEGORY, L"add_color effect has some components above 127 and some below -127 - colours will be clamped");
			m_Color = CColor(c.r+0.5f, c.g+0.5f, c.b+0.5f, c.a+0.5f);
			m_Method = ADD_SIGNED;
		}
	}

	~Effect_AddColor() {}

	void Set(const CTexturePtr& tex)
	{
		glEnable(GL_TEXTURE_2D);

		glColor4fv(m_Color.FloatArray());

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		if (m_Method == ADD_NORMAL)
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
		}
		else
		if (m_Method == ADD_SUBTRACT)
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_SUBTRACT);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_SUBTRACT);
		}
		else // if (m_Method == ADD_SIGNED)
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD_SIGNED);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD_SIGNED);
		}

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

		tex->Bind();
	}

	void Unset()
	{
		glColor4f(1.f, 1.f, 1.f, 1.f);
	}

private:
	CColor m_Color;
	enum { ADD_NORMAL, ADD_SUBTRACT, ADD_SIGNED, ADD_SIGNED_DOUBLED } m_Method;
};

class Effect_MultiplyColor : public IGLState
{
	// Uses GL_MODULATE to do the multiplication; but since all colours are
	// clamped to the range [0,1], it uses GL_RGB_SCALE to allow images to be
	// multiplied by [0,4]. Alpha is assumed to always be [0,1].
public:
	Effect_MultiplyColor(CColor c)
	{
		if (c.r <= 1.f && c.g <= 1.f && c.b <= 1.f)
		{
			m_Color = c;
			m_Scale = 1;
		}
		else if (c.r <= 2.f && c.g <= 2.f && c.b <= 2.f)
		{
			m_Color = CColor(c.r/2.f, c.g/2.f, c.b/2.f, c.a);
			m_Scale = 2;
		}
		else
		{
			if (c.r <= 4.f && c.g <= 4.f && c.b <= 4.f)
				;
			else
				// Oops - trying to multiply by >4
				LOG(CLogger::Warning, LOG_CATEGORY, L"multiply_color effect has a component >1020 - colours will be clamped");

			m_Color = CColor(c.r/4.f, c.g/4.f, c.b/4.f, c.a);
			m_Scale = 4;
		}
	}
	~Effect_MultiplyColor() {}
	void Set(const CTexturePtr& tex)
	{
		glEnable(GL_TEXTURE_2D);

		glColor4fv(m_Color.FloatArray());

		if (m_Scale == 1)
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
		else
		{
			// Duplicate the effect of GL_MODULATE, but using GL_COMBINE
			// so that GL_RGB_SCALE will work.
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
			
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

			if (m_Scale == 2)
				glTexEnviv(GL_TEXTURE_ENV, GL_RGB_SCALE, TexScale2);
			else if (m_Scale == 4)
				glTexEnviv(GL_TEXTURE_ENV, GL_RGB_SCALE, TexScale4);
		}

		tex->Bind();
	}
	void Unset()
	{
		if (m_Scale != 1)
			glTexEnviv(GL_TEXTURE_ENV, GL_RGB_SCALE, TexScale1);

		glColor4f(1.f, 1.f, 1.f, 1.f);
	}
private:
	CColor m_Color;
	int m_Scale;
};

#define X(n) (n##f/2.0f + 0.5f)
const float GreyscaleDotColor[4] = { X(0.3), X(0.59), X(0.11), 1.0f };
#undef X
const float GreyscaleInterpColor0[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
const float GreyscaleInterpColor1[4] = { 0.5f, 0.5f, 0.5f, 1.0f };

class Effect_Greyscale : public IGLState
{
public:
	~Effect_Greyscale() {}
	void Set(const CTexturePtr& tex)
	{
		/*

		For the main conversion, use GL_DOT3_RGB, which is defined as
		    L = 4 * ((Arg0r - 0.5) * (Arg1r - 0.5)+
		             (Arg0g - 0.5) * (Arg1g - 0.5)+
			         (Arg0b - 0.5) * (Arg1b - 0.5))
		where each of the RGB components is given the value 'L'.

		Use the magical luminance formula
		    L = 0.3R + 0.59G + 0.11B
		to calculate the greyscale value.

		But to work around the annoying "Arg0-0.5", we need to calculate
		Arg0+0.5. But we also need to scale it into the range 0.5-1.0, else
		Arg0>0.5 will be clamped to 1.0. So use GL_INTERPOLATE, which outputs:
		    A0 * A2 + A1 * (1 - A2)
		and set A2 = 0.5, A1 = 1.0, and A0 = texture (i.e. interpolating halfway
		between the texture and {1,1,1}) giving
		    A0/2 + 0.5
		and use that as Arg0.
		
		So L = 4*(A0/2 * (Arg1-.5))
		     = 2 (Rx+Gy+Bz)      (where Arg1 = {x+0.5, y+0.5, z+0.5})
			 = 2x R + 2y G + 2z B
			 = 0.3R + 0.59G + 0.11B
		so e.g. 2y = 0.59 = 2(Arg1g-0.5) => Arg1g = 0.59/2+0.5
		which fortunately doesn't get clamped.

		So, just implement that:

		*/

		// TODO: Render all greyscale objects at the same time, to reduce
		// the number of times the following code is called - it looks like
		// a rather worrying amount of work for rendering a single button...

		// Texture unit 0:

		tex->Bind(0);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GreyscaleInterpColor0);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_COLOR);
		glColor4fv(GreyscaleInterpColor1);

		// Texture unit 1:

		tex->Bind(1);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);

		// GL_DOT3_RGB requires GL_(EXT|ARB)_texture_env_dot3.
		// We currently don't bother implementing a fallback because it's
		// only lacking on Riva-class HW, but at least want the rest of the
		// game to run there without errors. Therefore, squelch the
		// OpenGL error that's raised if they aren't actually present.
		// Note: higher-level code checks for this extension, but
		// allows users the choice of continuing even if not present.
		ogl_SquelchError(GL_INVALID_ENUM);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GreyscaleDotColor);

	}
	void Unset()
	{
		glDisable(GL_TEXTURE_2D);
		pglActiveTextureARB(GL_TEXTURE0);

		glColor4f(1.f, 1.f, 1.f, 1.f);
	}
};



// Functions to perform drawing-related actions:

void GUIRenderer::UpdateDrawCallCache(DrawCalls &Calls, const CStr& SpriteName, const CRect &Size, int CellID, std::map<CStr, CGUISprite> &Sprites)
{
	// This is called only when something has changed (like the size of the
	// sprite), so it doesn't need to be particularly efficient.

	// Clean up the old data
	Calls.clear();

	// If this object has zero size, there's nothing to render. (This happens
	// with e.g. tooltips that have zero size before they're first drawn, so
	// it isn't necessarily an error.)
	if (Size.left==Size.right && Size.top==Size.bottom)
		return;


	std::map<CStr, CGUISprite>::iterator it (Sprites.find(SpriteName));
	if (it == Sprites.end())
	{
		// Sprite not found. Check whether this a special sprite:
		//     stretched:filename.ext
		//     <currently that's the only one>
		// and if so, try to create it as a new sprite.
		if (SpriteName.substr(0, 10) == "stretched:")
		{
			SGUIImage Image;
			Image.m_TextureName = VfsPath(L"art/textures/ui")/wstring_from_utf8(SpriteName.substr(10));
			CClientArea ca(CRect(0, 0, 0, 0), CRect(0, 0, 100, 100));
			Image.m_Size = ca;
			Image.m_TextureSize = ca;

			CGUISprite Sprite;
			Sprite.AddImage(Image);

			Sprites[SpriteName] = Sprite;
			
			it = Sprites.find(SpriteName);
			debug_assert(it != Sprites.end()); // The insertion above shouldn't fail
		}
		else
		{
			// Otherwise, just complain and give up:
			LOG(CLogger::Error, LOG_CATEGORY, L"Trying to use a sprite that doesn't exist (\"%hs\").", SpriteName.c_str());
			return;
		}
	}

	Calls.reserve(it->second.m_Images.size());

	// Iterate through all the sprite's images, loading the texture and
	// calculating the texture coordinates
	std::vector<SGUIImage>::const_iterator cit;
	for (cit = it->second.m_Images.begin(); cit != it->second.m_Images.end(); ++cit)
	{
		SDrawCall Call(&*cit); // pointers are safe since we never modify sprites/images after startup

		CRect ObjectSize = cit->m_Size.GetClientArea(Size);

		if (ObjectSize.GetWidth() == 0.0 || ObjectSize.GetHeight() == 0.0)
		{
			// Zero sized object. Don't report as an error, since it's common for e.g. hitpoint bars.
			continue; // i.e. don't continue with this image
		}

		Call.m_Vertices = ObjectSize;
		// Round the vertex coordinates to integers, to avoid ugly filtering artifacts
		Call.m_Vertices.left = (int)(Call.m_Vertices.left + 0.5f);
		Call.m_Vertices.right = (int)(Call.m_Vertices.right + 0.5f);
		Call.m_Vertices.top = (int)(Call.m_Vertices.top + 0.5f);
		Call.m_Vertices.bottom = (int)(Call.m_Vertices.bottom + 0.5f);

		if (! cit->m_TextureName.empty())
		{
			CTextureProperties textureProps(cit->m_TextureName);
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
			Call.m_EnableBlending = !(fabs(cit->m_BackColor.a - 1.0f) < 0.0000001f);
		}

		Call.m_BackColor = cit->m_BackColor;
		Call.m_BorderColor = cit->m_Border ? cit->m_BorderColor : CColor();
		Call.m_DeltaZ = cit->m_DeltaZ;
		
		if (cit->m_Effects)
		{
			if (cit->m_Effects->m_AddColor != CColor())
			{
				Call.m_Effects = new Effect_AddColor(cit->m_Effects->m_AddColor);
				// Always enable blending if something's being subtracted from
				// the alpha channel
				if (cit->m_Effects->m_AddColor.a < 0.f)
					Call.m_EnableBlending = true;
			}
			else if (cit->m_Effects->m_MultiplyColor != CColor())
			{
				Call.m_Effects = new Effect_MultiplyColor(cit->m_Effects->m_MultiplyColor);
				// Always enable blending if the alpha channel is being multiplied
				if (cit->m_Effects->m_AddColor.a != 1.f)
					Call.m_EnableBlending = true;
			}
			else if (cit->m_Effects->m_Greyscale)
			{
				Call.m_Effects = new Effect_Greyscale;
			}
			else /* Slight confusion - why no effects? */
			{
				Call.m_Effects = NULL;
			}

		}
		else
		{
			Call.m_Effects = NULL;
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

void GUIRenderer::Draw(DrawCalls &Calls)
{
	// Called every frame, to draw the object (based on cached calculations)

	glDisable(GL_BLEND);

	// Iterate through each DrawCall, and execute whatever drawing code is being called
	for (DrawCalls::const_iterator cit = Calls.begin(); cit != Calls.end(); ++cit)
	{
		if (cit->m_HasTexture)
		{
			// TODO: Handle the GL state in a nicer way

			if (cit->m_Effects)
				cit->m_Effects->Set(cit->m_Texture);
			else
			{
				glEnable(GL_TEXTURE_2D);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				cit->m_Texture->Bind();
			}

			if (cit->m_EnableBlending || cit->m_Texture->HasAlpha()) // (shouldn't call HasAlpha before Bind)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}

			CRect TexCoords = cit->ComputeTexCoords();

			glBegin(GL_QUADS);

				glTexCoord2f(TexCoords.right, TexCoords.bottom);
				glVertex3f(cit->m_Vertices.right, cit->m_Vertices.bottom, cit->m_DeltaZ);

				glTexCoord2f(TexCoords.left, TexCoords.bottom);
				glVertex3f(cit->m_Vertices.left, cit->m_Vertices.bottom, cit->m_DeltaZ);

				glTexCoord2f(TexCoords.left, TexCoords.top);
				glVertex3f(cit->m_Vertices.left, cit->m_Vertices.top, cit->m_DeltaZ);

				glTexCoord2f(TexCoords.right, TexCoords.top);
				glVertex3f(cit->m_Vertices.right, cit->m_Vertices.top, cit->m_DeltaZ);

			glEnd();

			if (cit->m_Effects)
				cit->m_Effects->Unset();
		}
		else
		{
			glDisable(GL_TEXTURE_2D);

			if (cit->m_EnableBlending)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}

			glColor4fv(cit->m_BackColor.FloatArray());

			glBegin(GL_QUADS);
				glVertex3f(cit->m_Vertices.right,	cit->m_Vertices.bottom,	cit->m_DeltaZ);
				glVertex3f(cit->m_Vertices.left,	cit->m_Vertices.bottom,	cit->m_DeltaZ);
				glVertex3f(cit->m_Vertices.left,	cit->m_Vertices.top,	cit->m_DeltaZ);
				glVertex3f(cit->m_Vertices.right,	cit->m_Vertices.top,	cit->m_DeltaZ);
			glEnd();


			if (cit->m_BorderColor != CColor())
			{
				glColor4fv(cit->m_BorderColor.FloatArray());
				glBegin(GL_LINE_LOOP);
					glVertex3f(cit->m_Vertices.left + 0.5f, cit->m_Vertices.top + 0.5f, cit->m_DeltaZ);
					glVertex3f(cit->m_Vertices.right - 0.5f, cit->m_Vertices.top + 0.5f, cit->m_DeltaZ);
					glVertex3f(cit->m_Vertices.right - 0.5f, cit->m_Vertices.bottom - 0.5f, cit->m_DeltaZ);
					glVertex3f(cit->m_Vertices.left + 0.5f, cit->m_Vertices.bottom - 0.5f, cit->m_DeltaZ);
				glEnd();
			}
		}

		glDisable(GL_BLEND);
	}
}
