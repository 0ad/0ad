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

#include "ShaderTechnique.h"

CShaderPass::CShaderPass(const CShaderProgramPtr& shader) :
	m_Shader(shader),
	m_HasAlpha(false), m_HasBlend(false), m_HasColorMask(false), m_HasDepthMask(false)
{
}

void CShaderPass::Bind()
{
	m_Shader->Bind();

	if (m_HasAlpha)
	{
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(m_AlphaFunc, m_AlphaRef);
	}

	if (m_HasBlend)
	{
		glEnable(GL_BLEND);
		glBlendFunc(m_BlendSrc, m_BlendDst);
	}

	if (m_HasColorMask)
		glColorMask(m_ColorMaskR, m_ColorMaskG, m_ColorMaskB, m_ColorMaskA);

	if (m_HasDepthMask)
		glDepthMask(m_DepthMask);
}

void CShaderPass::Unbind()
{
	m_Shader->Unbind();

	if (m_HasAlpha)
	{
		glDisable(GL_ALPHA_TEST);
	}

	if (m_HasBlend)
	{
		glDisable(GL_BLEND);
	}

	if (m_HasColorMask)
		glColorMask(1, 1, 1, 1);

	if (m_HasDepthMask)
		glDepthMask(1);
}

void CShaderPass::AlphaFunc(GLenum func, GLclampf ref)
{
	m_HasAlpha = true;
	m_AlphaFunc = func;
	m_AlphaRef = ref;
}

void CShaderPass::BlendFunc(GLenum src, GLenum dst)
{
	m_HasBlend = true;
	m_BlendSrc = src;
	m_BlendDst = dst;
}

void CShaderPass::ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
	m_HasColorMask = true;
	m_ColorMaskR = r;
	m_ColorMaskG = g;
	m_ColorMaskB = b;
	m_ColorMaskA = a;
}

void CShaderPass::DepthMask(GLboolean mask)
{
	m_HasDepthMask = true;
	m_DepthMask = mask;
}


CShaderTechnique::CShaderTechnique(const CShaderPass& pass)
{
	m_Passes.push_back(pass);
}

void CShaderTechnique::AddPass(const CShaderPass& pass)
{
	m_Passes.push_back(pass);
}

int CShaderTechnique::GetNumPasses()
{
	return m_Passes.size();
}

void CShaderTechnique::BeginPass(int pass)
{
	ENSURE(0 <= pass && pass < (int)m_Passes.size());
	m_Passes[pass].Bind();
}

void CShaderTechnique::EndPass(int pass)
{
	ENSURE(0 <= pass && pass < (int)m_Passes.size());
	m_Passes[pass].Unbind();
}

CShaderProgramPtr CShaderTechnique::GetShader(int pass)
{
	ENSURE(0 <= pass && pass < (int)m_Passes.size());
	return m_Passes[pass].GetShader();

}
