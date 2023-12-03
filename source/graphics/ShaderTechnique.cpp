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
#include "renderer/backend/IDevice.h"

CShaderPass::CShaderPass(
	std::unique_ptr<Renderer::Backend::IGraphicsPipelineState> pipelineState,
	const CShaderProgramPtr& shader)
	: m_Shader(shader), m_PipelineState(std::move(pipelineState))
{
	ENSURE(shader);
}

CShaderTechnique::CShaderTechnique(
	const VfsPath& path, const CShaderDefines& defines,
	const PipelineStateDescCallback& callback)
	: m_Path(path), m_Defines(defines), m_PipelineStateDescCallback(callback)
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
	return m_Passes[pass].GetPipelineState()->GetShaderProgram();
}

Renderer::Backend::IGraphicsPipelineState*
CShaderTechnique::GetGraphicsPipelineState(int pass) const
{
	ENSURE(0 <= pass && pass < static_cast<int>(m_Passes.size()));
	return m_Passes[pass].GetPipelineState();
}

bool CShaderTechnique::GetSortByDistance() const
{
	return m_SortByDistance;
}

void CShaderTechnique::SetSortByDistance(bool enable)
{
	m_SortByDistance = enable;
}
