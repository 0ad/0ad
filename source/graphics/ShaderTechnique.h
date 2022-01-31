/* Copyright (C) 2022 Wildfire Games.
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
#include "graphics/ShaderTechniquePtr.h"
#include "lib/ogl.h"
#include "renderer/backend/PipelineState.h"

#include <vector>

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

	void SetPipelineStateDesc(
		const Renderer::Backend::GraphicsPipelineStateDesc& pipelineStateDesc);

	/**
	 * Set up all the GL state that was previously specified on this pass.
	 */
	void Bind();

	/**
	 * Reset the GL state to the default.
	 */
	void Unbind();

	const CShaderProgramPtr& GetShader() const { return m_Shader; }

	const Renderer::Backend::GraphicsPipelineStateDesc&
	GetPipelineStateDesc() const { return m_PipelineStateDesc; }

private:
	CShaderProgramPtr m_Shader;

	Renderer::Backend::GraphicsPipelineStateDesc m_PipelineStateDesc{};
};

/**
 * Implements a render technique consisting of a sequence of passes.
 * CShaderManager loads these from shader effect XML files.
 */
class CShaderTechnique
{
public:
	CShaderTechnique();
	void SetPasses(std::vector<CShaderPass>&& passes);

	int GetNumPasses() const;

	void BeginPass(int pass = 0);
	void EndPass(int pass = 0);
	const CShaderProgramPtr& GetShader(int pass = 0) const;

	const Renderer::Backend::GraphicsPipelineStateDesc&
	GetGraphicsPipelineStateDesc(int pass = 0) const;

	/**
	 * Whether this technique uses alpha blending that requires objects to be
	 * drawn from furthest to nearest.
	 */
	bool GetSortByDistance() const;

	void SetSortByDistance(bool enable);

private:
	std::vector<CShaderPass> m_Passes;

	bool m_SortByDistance = false;
};

#endif // INCLUDED_SHADERTECHNIQUE
