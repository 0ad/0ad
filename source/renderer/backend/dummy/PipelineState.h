/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_RENDERER_BACKEND_DUMMY_PIPELINESTATE
#define INCLUDED_RENDERER_BACKEND_DUMMY_PIPELINESTATE

#include "renderer/backend/PipelineState.h"

#include <cstdint>
#include <memory>

namespace Renderer
{

namespace Backend
{

namespace Dummy
{

class CDevice;

class CGraphicsPipelineState final : public IGraphicsPipelineState
{
public:
	~CGraphicsPipelineState() override = default;

	IDevice* GetDevice() override;

	IShaderProgram* GetShaderProgram() const override { return m_Desc.shaderProgram; }

	const SGraphicsPipelineStateDesc& GetDesc() const { return m_Desc; }

private:
	friend class CDevice;

	static std::unique_ptr<CGraphicsPipelineState> Create(
		CDevice* device, const SGraphicsPipelineStateDesc& desc);

	CGraphicsPipelineState() = default;

	CDevice* m_Device = nullptr;

	SGraphicsPipelineStateDesc m_Desc{};
};

} // namespace Dummy

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_DUMMY_PIPELINESTATE
