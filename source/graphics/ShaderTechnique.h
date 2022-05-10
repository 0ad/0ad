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

#include "graphics/ShaderDefines.h"
#include "graphics/ShaderProgram.h"
#include "graphics/ShaderTechniquePtr.h"
#include "lib/file/vfs/vfs_path.h"
#include "renderer/backend/PipelineState.h"

#include <vector>

/**
 * Implements a render pass consisting of various GL state changes and a shader,
 * used by CShaderTechnique.
 */
class CShaderPass
{
public:
	CShaderPass(const Renderer::Backend::GraphicsPipelineStateDesc& pipelineStateDesc, const CShaderProgramPtr& shader);

	Renderer::Backend::IShaderProgram* GetShader() const { return m_Shader->GetBackendShaderProgram(); }

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
	CShaderTechnique(const VfsPath& path, const CShaderDefines& defines);

	void SetPasses(std::vector<CShaderPass>&& passes);

	int GetNumPasses() const;

	Renderer::Backend::IShaderProgram* GetShader(int pass = 0) const;

	const Renderer::Backend::GraphicsPipelineStateDesc&
	GetGraphicsPipelineStateDesc(int pass = 0) const;

	/**
	 * Whether this technique uses alpha blending that requires objects to be
	 * drawn from furthest to nearest.
	 */
	bool GetSortByDistance() const;

	void SetSortByDistance(bool enable);

	const VfsPath& GetPath() { return m_Path; }

	const CShaderDefines& GetShaderDefines() { return m_Defines; }

private:
	std::vector<CShaderPass> m_Passes;

	bool m_SortByDistance = false;

	// We need additional data to reload the technique.
	VfsPath m_Path;
	CShaderDefines m_Defines;
};

#endif // INCLUDED_SHADERTECHNIQUE
