/* Copyright (C) 2011 Wildfire Games.
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

#include "lib/res/graphics/ogl_tex.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "ps/Overlay.h"

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

	virtual Binding GetUniformBinding(uniform_id_t id)
	{
		return Binding(-1, GetUniformIndex(id));
	}

protected:
	std::map<CStr, int> m_UniformIndexes;
};

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
		if (id.fragment == ID_losTransform)
		{
			pglActiveTextureARB(GL_TEXTURE2);
			GLfloat texgenS1[4] = { v0, 0, 0, v1 };
			GLfloat texgenT1[4] = { 0, 0, v0, v1 };
			glTexGenfv(GL_S, GL_OBJECT_PLANE, texgenS1);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, texgenT1);
		}
		else if (id.fragment == ID_objectColor)
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

/*static*/ CShaderProgram* CShaderProgram::ConstructFFP(const std::string& id, const std::map<CStr, CStr>& defines)
{
	if (id == "overlayline")
		return new CShaderProgramFFP_OverlayLine(defines);

	LOGERROR(L"CShaderProgram::ConstructFFP: Invalid id '%hs'", id.c_str());
	return NULL;
}
