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

#include "precompiled.h"

#include "ShaderTechnique.h"

#include "graphics/ShaderProgram.h"

CShaderPass::CShaderPass(
	const Renderer::Backend::GraphicsPipelineStateDesc& pipelineStateDesc,
	const CShaderProgramPtr& shader)
	: m_PipelineStateDesc(pipelineStateDesc), m_Shader(shader)
{
	m_PipelineStateDesc.shaderProgram = m_Shader->GetBackendShaderProgram();
}

CShaderTechnique::CShaderTechnique(const VfsPath& path, const CShaderDefines& defines)
	: m_Path(path), m_Defines(defines)
{
}

void CShaderTechnique::SetPasses(std::vector<CShaderPass>&& passes)
{
	m_Passes = std::move(passes);
}

int CShaderTechnique::GetNumPasses() const
{
	return m_Passes.size();
}

Renderer::Backend::IShaderProgram* CShaderTechnique::GetShader(int pass) const
{
	ENSURE(0 <= pass && pass < (int)m_Passes.size());
	return m_Passes[pass].GetShader();
}

const Renderer::Backend::GraphicsPipelineStateDesc&
CShaderTechnique::GetGraphicsPipelineStateDesc(int pass) const
{
	ENSURE(0 <= pass && pass < static_cast<int>(m_Passes.size()));
	return m_Passes[pass].GetPipelineStateDesc();
}

bool CShaderTechnique::GetSortByDistance() const
{
	return m_SortByDistance;
}

void CShaderTechnique::SetSortByDistance(bool enable)
{
	m_SortByDistance = enable;
}
