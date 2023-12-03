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
#include "lib/code_annotation.h"
#include "lib/file/vfs/vfs_path.h"
#include "renderer/backend/PipelineState.h"

#include <functional>
#include <memory>
#include <vector>

/**
 * Implements a render pass consisting of a pipeline state and a shader,
 * used by CShaderTechnique.
 */
class CShaderPass
{
public:
	CShaderPass(
		std::unique_ptr<Renderer::Backend::IGraphicsPipelineState> pipelineState,
		const CShaderProgramPtr& shader);
	MOVABLE(CShaderPass);

	const CShaderProgramPtr& GetShaderProgram() const noexcept { return m_Shader; }

	Renderer::Backend::IGraphicsPipelineState*
	GetPipelineState() const noexcept { return m_PipelineState.get(); }

private:
	CShaderProgramPtr m_Shader;

	std::unique_ptr<Renderer::Backend::IGraphicsPipelineState> m_PipelineState;
};

/**
 * Implements a render technique consisting of a sequence of passes.
 * CShaderManager loads these from shader effect XML files.
 */
class CShaderTechnique
{
public:
	using PipelineStateDescCallback =
		std::function<void(Renderer::Backend::SGraphicsPipelineStateDesc& pipelineStateDesc)>;

	CShaderTechnique(const VfsPath& path, const CShaderDefines& defines, const PipelineStateDescCallback& callback);

	void SetPasses(std::vector<CShaderPass>&& passes);

	int GetNumPasses() const;

	Renderer::Backend::IShaderProgram* GetShader(int pass = 0) const;

	Renderer::Backend::IGraphicsPipelineState*
	GetGraphicsPipelineState(int pass = 0) const;

	/**
	 * Whether this technique uses alpha blending that requires objects to be
	 * drawn from furthest to nearest.
	 */
	bool GetSortByDistance() const;

	void SetSortByDistance(bool enable);

	const VfsPath& GetPath() { return m_Path; }

	const CShaderDefines& GetShaderDefines() { return m_Defines; }

	const PipelineStateDescCallback& GetPipelineStateDescCallback() const { return m_PipelineStateDescCallback; };

private:
	std::vector<CShaderPass> m_Passes;

	bool m_SortByDistance = false;

	// We need additional data to reload the technique.
	VfsPath m_Path;
	CShaderDefines m_Defines;

	PipelineStateDescCallback m_PipelineStateDescCallback;
};

#endif // INCLUDED_SHADERTECHNIQUE
