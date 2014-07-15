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

#include "ShaderProgram.h"

#include "graphics/ShaderDefines.h"
#include "graphics/TextureManager.h"
#include "lib/res/graphics/ogl_tex.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "ps/Overlay.h"
#include "renderer/Renderer.h"

#if !CONFIG2_GLES

/**
 * CShaderProgramFFP allows rendering code to use the shader-based API
 * even if the 'shader' is actually implemented with the fixed-function
 * pipeline instead of anything programmable.
 *
 * Currently we just hard-code a number of FFP programs as subclasses of this.
 * If we have lots, it might be nicer to abstract out the common functionality
 * and load these from text files or something.
 */
class CShaderProgramFFP : public CShaderProgram
{
public:
	CShaderProgramFFP(int streamflags) :
		CShaderProgram(streamflags)
	{
	}

	~CShaderProgramFFP()
	{
	}

	virtual void Reload()
	{
		m_IsValid = true;
	}

	int GetUniformIndex(CStrIntern id)
	{
		std::map<CStrIntern, int>::iterator it = m_UniformIndexes.find(id);
		if (it == m_UniformIndexes.end())
			return -1;
		return it->second;
	}

	virtual Binding GetTextureBinding(uniform_id_t id)
	{
		int index = GetUniformIndex(CStrIntern(id));
		if (index == -1)
			return Binding();
		else
			return Binding((int)GL_TEXTURE_2D, index);
	}

	virtual void BindTexture(texture_id_t id, Handle tex)
	{
		int index = GetUniformIndex(CStrIntern(id));
		if (index != -1)
			ogl_tex_bind(tex, index);
	}

	virtual void BindTexture(texture_id_t id, GLuint tex)
	{
		int index = GetUniformIndex(CStrIntern(id));
		if (index != -1)
		{
			pglActiveTextureARB((int)(GL_TEXTURE0+index));
			glBindTexture(GL_TEXTURE_2D, tex);
		}
	}

	virtual void BindTexture(Binding id, Handle tex)
	{
		int index = id.second;
		if (index != -1)
			ogl_tex_bind(tex, index);
	}

	virtual Binding GetUniformBinding(uniform_id_t id)
	{
		return Binding(-1, GetUniformIndex(id));
	}

	virtual void Uniform(Binding UNUSED(id), float UNUSED(v0), float UNUSED(v1), float UNUSED(v2), float UNUSED(v3))
	{
	}

	virtual void Uniform(Binding UNUSED(id), const CMatrix3D& UNUSED(v))
	{
	}

	virtual void Uniform(Binding UNUSED(id), size_t UNUSED(count), const CMatrix3D* UNUSED(v))
	{
	}

protected:
	std::map<CStrIntern, int> m_UniformIndexes;

	void SetUniformIndex(const char* id, int value)
	{
		m_UniformIndexes[CStrIntern(id)] = value;
	}
};

//////////////////////////////////////////////////////////////////////////

/**
 * A shader that does nothing but provide a shader-compatible interface to
 * fixed-function features, for compatibility with existing fixed-function
 * code that isn't fully ported to the shader API.
 */
class CShaderProgramFFP_Dummy : public CShaderProgramFFP
{
public:
	CShaderProgramFFP_Dummy() :
		CShaderProgramFFP(0)
	{
		// Texture units, for when this shader is used with terrain:
		SetUniformIndex("baseTex", 0);
	}

	virtual void Bind()
	{
		BindClientStates();
	}

	virtual void Unbind()
	{
		UnbindClientStates();
	}
};

//////////////////////////////////////////////////////////////////////////

class CShaderProgramFFP_OverlayLine : public CShaderProgramFFP
{
	// Uniforms
	enum
	{
		ID_losTransform,
		ID_objectColor
	};

	bool m_IgnoreLos;
	bool m_UseObjectColor;

public:
	CShaderProgramFFP_OverlayLine(const CShaderDefines& defines) :
		CShaderProgramFFP(0) // will be set manually in initializer below
	{
		SetUniformIndex("losTransform", ID_losTransform);
		SetUniformIndex("objectColor", ID_objectColor);

		// Texture units:
		SetUniformIndex("baseTex", 0);
		SetUniformIndex("maskTex", 1);
		SetUniformIndex("losTex", 2);

		m_IgnoreLos = (defines.GetInt("IGNORE_LOS") != 0);
		m_UseObjectColor = (defines.GetInt("USE_OBJECTCOLOR") != 0);

		m_StreamFlags = STREAM_POS | STREAM_UV0 | STREAM_UV1;
		if (!m_UseObjectColor)
			m_StreamFlags |= STREAM_COLOR;
	}

	bool IsIgnoreLos()
	{
		return m_IgnoreLos;
	}

	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3)
	{
		if (id.second == ID_losTransform)
		{
			pglActiveTextureARB(GL_TEXTURE2);
			GLfloat texgenS1[4] = { v0, 0, 0, v1 };
			GLfloat texgenT1[4] = { 0, 0, v0, v1 };
			glTexGenfv(GL_S, GL_OBJECT_PLANE, texgenS1);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, texgenT1);
		}
		else if (id.second == ID_objectColor)
		{
			float c[] = { v0, v1, v2, v3 };
			pglActiveTextureARB(GL_TEXTURE1);
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, c);
		}
		else
		{
			debug_warn(L"Invalid id");
		}
	}

	virtual void Uniform(Binding UNUSED(id), const CMatrix3D& UNUSED(v))
	{
		debug_warn(L"Not implemented");
	}

	virtual void Bind()
	{
		// RGB channels:
		//   Unit 0: Sample base texture
		//   Unit 1: Sample mask texture; interpolate with [objectColor or vertexColor] and base, depending on USE_OBJECTCOLOR
		//   Unit 2: (Load LOS texture; multiply) if not #IGNORE_LOS, pass through otherwise
		// Alpha channel:
		//   Unit 0: Sample base texture
		//   Unit 1: Multiply by objectColor
		//   Unit 2: Pass through

		pglActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		// Sample base texture RGB
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

		// Sample base texture Alpha
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		// -----------------------------------------------------------------------------

		pglActiveTextureARB(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		// RGB: interpolate component-wise between [objectColor or vertexColor] and base using mask:
		//   a0 * a2 + a1 * (1 - a2)
		// Overridden implementation of Uniform() sets GL_TEXTURE_ENV_COLOR to objectColor, which
		// is referenced as GL_CONSTANT (see spec)
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, m_UseObjectColor ? GL_CONSTANT : GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_COLOR);

		// ALPHA: Multiply base alpha with [objectColor or vertexColor] alpha
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, m_UseObjectColor ? GL_CONSTANT : GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);

		// -----------------------------------------------------------------------------

		pglActiveTextureARB(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		bool ignoreLos = IsIgnoreLos();
		if (ignoreLos)
		{
			// RGB pass through
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		}
		else
		{
			// Multiply RGB result up till now with LoS texture alpha channel
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			// Overridden implementation of Uniform() sets GL_OBJECT_PLANE values

			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_ALPHA);
		}

		// alpha pass through
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		BindClientStates();
	}

	virtual void Unbind()
	{
		UnbindClientStates();

		pglActiveTextureARB(GL_TEXTURE2);
		glDisable(GL_TEXTURE_2D);

		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);

		pglActiveTextureARB(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);

		pglActiveTextureARB(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
	}
};

//////////////////////////////////////////////////////////////////////////

class CShaderProgramFFP_GuiText : public CShaderProgramFFP
{
	// Uniforms
	enum
	{
		ID_transform,
		ID_colorMul
	};

public:
	CShaderProgramFFP_GuiText() :
		CShaderProgramFFP(STREAM_POS | STREAM_UV0)
	{
		SetUniformIndex("transform", ID_transform);
		SetUniformIndex("colorMul", ID_colorMul);

		// Texture units:
		SetUniformIndex("tex", 0);
	}

	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3)
	{
		if (id.second == ID_colorMul)
			glColor4f(v0, v1, v2, v3);
	}

	virtual void Uniform(Binding id, const CMatrix3D& v)
	{
		if (id.second == ID_transform)
			glLoadMatrixf(&v._11);
	}

	virtual void Bind()
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		pglActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		BindClientStates();
	}

	virtual void Unbind()
	{
		UnbindClientStates();

		pglActiveTextureARB(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
};

//////////////////////////////////////////////////////////////////////////

class CShaderProgramFFP_Gui_Base : public CShaderProgramFFP
{
protected:
	// Uniforms
	enum
	{
		ID_transform,
		ID_color
	};

public:
	CShaderProgramFFP_Gui_Base(int streamflags) :
		CShaderProgramFFP(streamflags)
	{
		SetUniformIndex("transform", ID_transform);
		SetUniformIndex("color", ID_color);

		// Texture units:
		SetUniformIndex("tex", 0);
	}

	virtual void Uniform(Binding id, const CMatrix3D& v)
	{
		if (id.second == ID_transform)
			glLoadMatrixf(&v._11);
	}

	virtual void Bind()
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		BindClientStates();
	}

	virtual void Unbind()
	{
		UnbindClientStates();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
};

class CShaderProgramFFP_GuiMinimap : public CShaderProgramFFP
{
protected:
	CShaderDefines m_Defines;
	// Uniforms
	enum
	{
		ID_transform,
		ID_textureTransform,
		ID_color,
	};
public:
	CShaderProgramFFP_GuiMinimap(const CShaderDefines& defines) :
		CShaderProgramFFP(0) // We set the streamflags later, during initialization.
	{
		m_Defines = defines;
		SetUniformIndex("transform", ID_transform);
		SetUniformIndex("textureTransform", ID_textureTransform);
		SetUniformIndex("color", ID_color);

		if (m_Defines.GetInt("MINIMAP_BASE") || m_Defines.GetInt("MINIMAP_LOS"))
		{
			SetUniformIndex("baseTex", 0);
			m_StreamFlags = STREAM_POS | STREAM_UV0;
		}
		else if (m_Defines.GetInt("MINIMAP_POINT"))
			m_StreamFlags = STREAM_POS | STREAM_COLOR;
		else
			m_StreamFlags = STREAM_POS;
	}

	virtual void Uniform(Binding id, const CMatrix3D& v)
	{
		if (id.second == ID_textureTransform)
		{
			glMatrixMode(GL_TEXTURE);
			glLoadMatrixf(&v._11);
		}
		else if (id.second == ID_transform)
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(&v._11);
		}
	}

	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3)
	{
		if (id.second == ID_color)
			glColor4f(v0, v1, v2, v3);
	}

	virtual void Bind()
	{
		// Setup matrix environment
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glLoadIdentity();

		BindClientStates();

		// Setup texture environment
		if (m_Defines.GetInt("MINIMAP_BASE"))
		{
			glEnable(GL_TEXTURE_2D);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else if (m_Defines.GetInt("MINIMAP_LOS"))
		{
			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);
			glColor3f(0.0f, 0.0f, 0.0f);
		}
		else if (m_Defines.GetInt("MINIMAP_POINT"))
		{
			glDisable(GL_TEXTURE_2D);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);
		}
		else if (m_Defines.GetInt("MINIMAP_LINE"))
		{
			// JoshuaJB 13.7.2014: This doesn't seem to do anything on my drivers.
			glEnable(GL_LINE_SMOOTH);
		}
	}

	virtual void Unbind()
	{
		// Reset texture environment
		if (m_Defines.GetInt("MINIMAP_POINT"))
		{
			glEnable(GL_TEXTURE_2D);
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
		}
		else if (m_Defines.GetInt("MINIMAP_LINE"))
		{
			glDisable(GL_LINE_SMOOTH);
		}

		UnbindClientStates();

		// Clear matrix stack
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
};

class CShaderProgramFFP_GuiBasic : public CShaderProgramFFP_Gui_Base
{
public:
	CShaderProgramFFP_GuiBasic() :
		CShaderProgramFFP_Gui_Base(STREAM_POS | STREAM_UV0)
	{
	}

	virtual void Bind()
	{
		CShaderProgramFFP_Gui_Base::Bind();

		pglActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	}

	virtual void Unbind()
	{
		pglActiveTextureARB(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);

		CShaderProgramFFP_Gui_Base::Unbind();
	}
};

class CShaderProgramFFP_GuiAdd : public CShaderProgramFFP_Gui_Base
{
public:
	using CShaderProgramFFP_Gui_Base::Uniform;

	CShaderProgramFFP_GuiAdd() :
		CShaderProgramFFP_Gui_Base(STREAM_POS | STREAM_UV0)
	{
	}

	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3)
	{
		if (id.second == ID_color)
			glColor4f(v0, v1, v2, v3);
	}

	virtual void Bind()
	{
		CShaderProgramFFP_Gui_Base::Bind();

		pglActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
	}

	virtual void Unbind()
	{
		glColor4f(1.f, 1.f, 1.f, 1.f);

		pglActiveTextureARB(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);

		CShaderProgramFFP_Gui_Base::Unbind();
	}
};

class CShaderProgramFFP_GuiGrayscale : public CShaderProgramFFP_Gui_Base
{
public:
	CShaderProgramFFP_GuiGrayscale() :
		CShaderProgramFFP_Gui_Base(STREAM_POS | STREAM_UV0)
	{
	}

	virtual void Bind()
	{
		CShaderProgramFFP_Gui_Base::Bind();

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

		static const float GreyscaleDotColor[4] = {
			0.3f / 2.f + 0.5f,
			0.59f / 2.f + 0.5f,
			0.11f / 2.f + 0.5f,
			1.0f
		};
		static const float GreyscaleInterpColor0[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		static const float GreyscaleInterpColor1[4] = { 0.5f, 0.5f, 0.5f, 1.0f };

		pglActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GreyscaleInterpColor0);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		glColor4fv(GreyscaleInterpColor1);

		pglActiveTextureARB(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);

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

		// To activate the second texture unit, we have to have some kind
		// of texture bound into it, but we don't actually use the texture data,
		// so bind a dummy texture
		g_Renderer.GetTextureManager().GetErrorTexture()->Bind(1);
	}

	virtual void Unbind()
	{
		glColor4f(1.f, 1.f, 1.f, 1.f);

		pglActiveTextureARB(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);

		pglActiveTextureARB(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);

		CShaderProgramFFP_Gui_Base::Unbind();
	}
};

class CShaderProgramFFP_GuiSolid : public CShaderProgramFFP_Gui_Base
{
public:
	using CShaderProgramFFP_Gui_Base::Uniform;

	CShaderProgramFFP_GuiSolid() :
		CShaderProgramFFP_Gui_Base(STREAM_POS)
	{
	}

	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3)
	{
		if (id.second == ID_color)
			glColor4f(v0, v1, v2, v3);
	}

	virtual void Bind()
	{
		CShaderProgramFFP_Gui_Base::Bind();

		pglActiveTextureARB(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
	}
};

//////////////////////////////////////////////////////////////////////////

/**
 * Common functionality for model rendering in the fixed renderpath.
 */
class CShaderProgramFFP_Model_Base : public CShaderProgramFFP
{
protected:
	// Uniforms
	enum
	{
		ID_transform,
		ID_objectColor,
		ID_playerColor,
		ID_losTransform
	};
	
	bool m_IgnoreLos;

public:
	CShaderProgramFFP_Model_Base(const CShaderDefines& defines, int streamflags)
		: CShaderProgramFFP(streamflags)
	{
		SetUniformIndex("transform", ID_transform);
		SetUniformIndex("losTransform", ID_losTransform);

		if (defines.GetInt("USE_OBJECTCOLOR"))
			SetUniformIndex("objectColor", ID_objectColor);

		if (defines.GetInt("USE_PLAYERCOLOR"))
			SetUniformIndex("playerColor", ID_playerColor);

		m_IgnoreLos = (defines.GetInt("IGNORE_LOS") != 0);

		// Texture units:
		SetUniformIndex("baseTex", 0);
		SetUniformIndex("losTex", 3);
	}

	virtual void Uniform(Binding id, const CMatrix3D& v)
	{
		if (id.second == ID_transform)
			glLoadMatrixf(&v._11);
	}
	
	virtual void Uniform(Binding id, float v0, float v1, float UNUSED(v2), float UNUSED(v3))
	{
		if (id.second == ID_losTransform)
		{
			pglActiveTextureARB(GL_TEXTURE3);
			GLfloat texgenS1[4] = { v0, 0, 0, v1 };
			GLfloat texgenT1[4] = { 0, 0, v0, v1 };
			glTexGenfv(GL_S, GL_OBJECT_PLANE, texgenS1);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, texgenT1);
		}
	}

	virtual void Bind()
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		
		// -----------------------------------------------------------------------------
		
		pglActiveTextureARB(GL_TEXTURE3);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		if (m_IgnoreLos)
		{
			// RGB pass through
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		}
		else
		{
			// Multiply RGB result up till now with LoS texture alpha channel
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			// Overridden implementation of Uniform() sets GL_OBJECT_PLANE values
			
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_ALPHA);
		}

		// alpha pass through
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
		
		// -----------------------------------------------------------------------------

		BindClientStates();
	}

	virtual void Unbind()
	{
		UnbindClientStates();
		
		pglActiveTextureARB(GL_TEXTURE3);
		glDisable(GL_TEXTURE_2D);

		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		
		pglActiveTextureARB(GL_TEXTURE0);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
};

/**
 * Basic non-recolored diffuse-textured model rendering.
 */
class CShaderProgramFFP_Model : public CShaderProgramFFP_Model_Base
{
public:
	CShaderProgramFFP_Model(const CShaderDefines& defines)
		: CShaderProgramFFP_Model_Base(defines, STREAM_POS | STREAM_COLOR | STREAM_UV0)
	{
	}

	virtual void Bind()
	{
		// Set up texture environment for base pass - modulate texture and vertex color
		pglActiveTextureARB(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		// Copy alpha channel from texture
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		// The vertex color is scaled by 0.5 to permit overbrightness without clamping.
		// We therefore need to scale by 2.0 after the modulation, and before any
		// further clamping, to get the right color.
		float scale2[] = { 2.0f, 2.0f, 2.0f };
		glTexEnvfv(GL_TEXTURE_ENV, GL_RGB_SCALE, scale2);

		CShaderProgramFFP_Model_Base::Bind();
	}

	virtual void Unbind()
	{
		CShaderProgramFFP_Model_Base::Unbind();

		pglActiveTextureARB(GL_TEXTURE0);

		// Revert the scaling to default
		float scale1[] = { 1.0f, 1.0f, 1.0f };
		glTexEnvfv(GL_TEXTURE_ENV, GL_RGB_SCALE, scale1);
	}
};

/**
 * Player-coloring diffuse-textured model rendering.
 */
class CShaderProgramFFP_ModelColor : public CShaderProgramFFP_Model_Base
{
public:
	CShaderProgramFFP_ModelColor(const CShaderDefines& defines)
		: CShaderProgramFFP_Model_Base(defines, STREAM_POS | STREAM_COLOR | STREAM_UV0)
	{
	}

	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3)
	{
		if (id.second == ID_objectColor || id.second == ID_playerColor)
		{
			// (Player color and object color are mutually exclusive)
			float color[] = { v0, v1, v2, v3 };
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
		}
	}

	virtual void Bind()
	{
		// Player color uses a single pass with three texture environments
		// Note: This uses ARB_texture_env_crossbar (which is checked in GameSetup),
		// and it requires MAX_TEXTURE_IMAGE_UNITS >= 3 (which only excludes GF2MX/GF4MX)
		//
		// We calculate: Result = Color*Texture*(PlayerColor*(1-Texture.a) + 1.0*Texture.a)
		// Algebra gives us:
		// Result = (1 - ((1 - PlayerColor) * (1 - Texture.a)))*Texture*Color

		// TexEnv #0
		pglActiveTextureARB(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_ALPHA);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_ONE_MINUS_SRC_COLOR);

		// Don't care about alpha; set it to something harmless
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		// TexEnv #1
		pglActiveTextureARB(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_ONE_MINUS_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		// Don't care about alpha; set it to something harmless
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		// TexEnv #2
		pglActiveTextureARB(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		// Don't care about alpha; set it to something harmless
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

		// Scale colors at the end of all the computation (see CShaderProgramFFP_Model::Bind)
		float scale[] = { 2.0f, 2.0f, 2.0f };
		glTexEnvfv(GL_TEXTURE_ENV, GL_RGB_SCALE, scale);

		// Need to bind some kind of texture to enable the texture units.
		// Unit 0 has baseTex, but the others need a texture.
		g_Renderer.GetTextureManager().GetErrorTexture()->Bind(1);
		g_Renderer.GetTextureManager().GetErrorTexture()->Bind(2);

		CShaderProgramFFP_Model_Base::Bind();
	}

	virtual void Unbind()
	{
		CShaderProgramFFP_Model_Base::Unbind();

		pglActiveTextureARB(GL_TEXTURE2);
		glDisable(GL_TEXTURE_2D);

		float scale[] = { 1.0f, 1.0f, 1.0f };
		glTexEnvfv(GL_TEXTURE_ENV, GL_RGB_SCALE, scale);

		pglActiveTextureARB(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);

		pglActiveTextureARB(GL_TEXTURE0);
	}
};

/**
 * Optionally-player-colored untextured model rendering.
 */
class CShaderProgramFFP_ModelSolid : public CShaderProgramFFP_Model_Base
{
public:
	CShaderProgramFFP_ModelSolid(const CShaderDefines& defines)
		: CShaderProgramFFP_Model_Base(defines, STREAM_POS)
	{
	}

	virtual void Uniform(Binding id, float v0, float v1, float v2, float v3)
	{
		if (id.second == ID_playerColor)
		{
			float color[] = { v0, v1, v2, v3 };
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
		}
	}

	virtual void Bind()
	{
		float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

		pglActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		CShaderProgramFFP_Model_Base::Bind();
	}
};

/**
 * Plain unlit texture model rendering, for e.g. alpha-blended shadow casters.
 */
class CShaderProgramFFP_ModelSolidTex : public CShaderProgramFFP_Model_Base
{
public:
	CShaderProgramFFP_ModelSolidTex(const CShaderDefines& defines)
		: CShaderProgramFFP_Model_Base(defines, STREAM_POS | STREAM_UV0)
	{
	}

	virtual void Bind()
	{
		pglActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		CShaderProgramFFP_Model_Base::Bind();
	}
};

//////////////////////////////////////////////////////////////////////////

/*static*/ CShaderProgram* CShaderProgram::ConstructFFP(const std::string& id, const CShaderDefines& defines)
{
	if (id == "dummy")
		return new CShaderProgramFFP_Dummy();
	if (id == "overlayline")
		return new CShaderProgramFFP_OverlayLine(defines);
	if (id == "gui_text")
		return new CShaderProgramFFP_GuiText();
	if (id == "gui_basic")
		return new CShaderProgramFFP_GuiBasic();
	if (id == "gui_add")
		return new CShaderProgramFFP_GuiAdd();
	if (id == "gui_grayscale")
		return new CShaderProgramFFP_GuiGrayscale();
	if (id == "gui_solid")
		return new CShaderProgramFFP_GuiSolid();
	if (id == "minimap")
		return new CShaderProgramFFP_GuiMinimap(defines);
	if (id == "solid")
		return new CShaderProgramFFP_GuiSolid(); // works for non-GUI objects too
	if (id == "model")
		return new CShaderProgramFFP_Model(defines);
	if (id == "model_color")
		return new CShaderProgramFFP_ModelColor(defines);
	if (id == "model_solid")
		return new CShaderProgramFFP_ModelSolid(defines);
	if (id == "model_solid_tex")
		return new CShaderProgramFFP_ModelSolidTex(defines);

	LOGERROR(L"CShaderProgram::ConstructFFP: '%hs': Invalid id", id.c_str());
	debug_warn(L"CShaderProgram::ConstructFFP: Invalid id");
	return NULL;
}

#else // CONFIG2_GLES

/*static*/ CShaderProgram* CShaderProgram::ConstructFFP(const std::string& UNUSED(id), const CShaderDefines& UNUSED(defines))
{
	debug_warn(L"CShaderProgram::ConstructFFP: FFP not supported on this device");
	return NULL;
}

#endif // CONFIG2_GLES
