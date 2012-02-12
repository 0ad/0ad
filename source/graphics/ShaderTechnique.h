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

#ifndef INCLUDED_SHADERTECHNIQUE
#define INCLUDED_SHADERTECHNIQUE

#include "graphics/ShaderProgram.h"

/**
 * Implements a render pass consisting of various GL state changes and a shader.
 */
class CShaderPass
{
public:
	CShaderPass(const CShaderProgramPtr& shader);

	// Add various bits of GL state to the pass:
	void AlphaFunc(GLenum func, GLclampf ref);
	void BlendFunc(GLenum src, GLenum dst);
	void ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
	void DepthMask(GLboolean mask);
	void DepthFunc(GLenum func);

	/**
	 * Set all the GL state that was previously specified on this pass.
	 */
	void Bind();

	/**
	 * Reset the GL state to the default.
	 */
	void Unbind();

	CShaderProgramPtr GetShader() { return m_Shader; }

private:
	CShaderProgramPtr m_Shader;

	bool m_HasAlpha;
	GLenum m_AlphaFunc;
	GLclampf m_AlphaRef;

	bool m_HasBlend;
	GLenum m_BlendSrc;
	GLenum m_BlendDst;

	bool m_HasColorMask;
	GLboolean m_ColorMaskR;
	GLboolean m_ColorMaskG;
	GLboolean m_ColorMaskB;
	GLboolean m_ColorMaskA;

	bool m_HasDepthMask;
	GLboolean m_DepthMask;

	bool m_HasDepthFunc;
	GLenum m_DepthFunc;
};

/**
 * Implements a render technique consisting of a sequence of passes.
 * In theory these should probably be loaded from an XML file or something,
 * but currently you have to construct them manually.
 */
class CShaderTechnique
{
public:
	CShaderTechnique();
	CShaderTechnique(const CShaderPass& pass);
	void AddPass(const CShaderPass& pass);

	int GetNumPasses();

	void BeginPass(int pass = 0);
	void EndPass(int pass = 0);
	CShaderProgramPtr GetShader(int pass = 0);

	void Reset();

private:
	std::vector<CShaderPass> m_Passes;
};

typedef shared_ptr<CShaderTechnique> CShaderTechniquePtr;

#endif // INCLUDED_SHADERTECHNIQUE
