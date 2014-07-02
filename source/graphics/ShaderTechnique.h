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

#include "graphics/ShaderProgramPtr.h"
#include "lib/ogl.h"

/**
 * Implements a render pass consisting of various GL state changes and a shader,
 * used by CShaderTechnique.
 */
class CShaderPass
{
public:
	CShaderPass();

	/**
	 * Set the shader program used for rendering with this pass.
	 */
	void SetShader(const CShaderProgramPtr& shader) { m_Shader = shader; }

	// Add various bits of GL state to the pass:
	void AlphaFunc(GLenum func, GLclampf ref);
	void BlendFunc(GLenum src, GLenum dst);
	void ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
	void DepthMask(GLboolean mask);
	void DepthFunc(GLenum func);

	/**
	 * Set up all the GL state that was previously specified on this pass.
	 */
	void Bind();

	/**
	 * Reset the GL state to the default.
	 */
	void Unbind();

	const CShaderProgramPtr& GetShader() const { return m_Shader; }

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
 * CShaderManager loads these from shader effect XML files.
 */
class CShaderTechnique
{
public:
	CShaderTechnique();
	void AddPass(const CShaderPass& pass);

	int GetNumPasses() const;

	void BeginPass(int pass = 0);
	void EndPass(int pass = 0);
	const CShaderProgramPtr& GetShader(int pass = 0) const;

	/**
	 * Whether this technique uses alpha blending that requires objects to be
	 * drawn from furthest to nearest.
	 */
	bool GetSortByDistance() const;

	void SetSortByDistance(bool enable);

	void Reset();

private:
	std::vector<CShaderPass> m_Passes;

	bool m_SortByDistance;
};

typedef shared_ptr<CShaderTechnique> CShaderTechniquePtr;

#endif // INCLUDED_SHADERTECHNIQUE
