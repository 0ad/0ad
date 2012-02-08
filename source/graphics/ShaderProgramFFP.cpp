/* Copyright (C) 2012 Wildfire Games.
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

#include "graphics/TextureManager.h"
#include "lib/res/graphics/ogl_tex.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "ps/Overlay.h"
#include "renderer/Renderer.h"

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

	int GetUniformIndex(uniform_id_t id)
	{
		std::map<CStr, int>::iterator it = m_UniformIndexes.find(id);
		if (it == m_UniformIndexes.end())
			return -1;
		return it->second;
	}

	virtual bool HasTexture(texture_id_t id)
	{
		if (GetUniformIndex(id) != -1)
			return true;
		return false;
	}

	virtual void BindTexture(texture_id_t id, Handle tex)
	{
		int index = GetUniformIndex(id);
		if (index != -1)
			ogl_tex_bind(tex, index);
	}

	virtual void BindTexture(texture_id_t id, GLuint tex)
	{
		int index = GetUniformIndex(id);
		if (index != -1)
		{
			pglActiveTextureARB((int)(GL_TEXTURE0+index));
			glBindTexture(GL_TEXTURE_2D, tex);
		}
	}

	virtual int GetTextureUnit(texture_id_t id)
	{
		return GetUniformIndex(id);
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

protected:
	std::map<CStr, int> m_UniformIndexes;
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

public:
	CShaderProgramFFP_OverlayLine(const std::map<CStr, CStr>& defines) :
		CShaderProgramFFP(STREAM_POS | STREAM_UV0 | STREAM_UV1)
	{
		m_UniformIndexes["losTransform"] = ID_losTransform;
		m_UniformIndexes["objectColor"] = ID_objectColor;

		// Texture units:
		m_UniformIndexes["baseTex"] = 0;
		m_UniformIndexes["maskTex"] = 1;
		m_UniformIndexes["losTex"] = 2;

		m_IgnoreLos = (defines.find(CStr("IGNORE_LOS")) != defines.end());
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
		//   Unit 0: Load base texture
		//   Unit 1: Load mask texture; interpolate with objectColor & base
		//   Unit 2: (Load LOS texture; multiply) if not #IGNORE_LOS, pass through otherwise
		// Alpha channel:
		//   Unit 0: Load base texture
		//   Unit 1: Multiply by objectColor
		//   Unit 2: Pass through

		pglActiveTextureARB(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		// -----------------------------------------------------------------------------

		pglActiveTextureARB(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		// Uniform() sets GL_TEXTURE_ENV_COLOR

		// load mask texture; interpolate with objectColor and base; GL_INTERPOLATE takes 3 arguments:
		// a0 * a2 + a1 * (1 - a2)
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_CONSTANT);
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
			// multiply RGB with LoS texture alpha channel
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			// Uniform() sets GL_OBJECT_PLANE values
			
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
		
	}

	virtual void Unbind()
	{
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
		m_UniformIndexes["transform"] = ID_transform;
		m_UniformIndexes["colorMul"] = ID_colorMul;

		// Texture units:
		m_UniformIndexes["tex"] = 0;
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
	}

	virtual void Unbind()
	{
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
		m_UniformIndexes["transform"] = ID_transform;
		m_UniformIndexes["color"] = ID_color;

		// Texture units:
		m_UniformIndexes["tex"] = 0;
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
	}

	virtual void Unbind()
	{
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

/*static*/ CShaderProgram* CShaderProgram::ConstructFFP(const std::string& id, const std::map<CStr, CStr>& defines)
{
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

	LOGERROR(L"CShaderProgram::ConstructFFP: Invalid id '%hs'", id.c_str());
	return NULL;
}
