/* Copyright (C) 2024 Wildfire Games.
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

#include "PipelineState.h"

#include "renderer/backend/gl/Device.h"

namespace Renderer
{

namespace Backend
{

namespace GL
{

// static
std::unique_ptr<CGraphicsPipelineState> CGraphicsPipelineState::Create(
	CDevice* device, const SGraphicsPipelineStateDesc& desc)
{
	std::unique_ptr<CGraphicsPipelineState> pipelineState{new CGraphicsPipelineState()};
	pipelineState->m_Device = device;
	pipelineState->m_Desc = desc;
	return pipelineState;
}

IDevice* CGraphicsPipelineState::GetDevice()
{
	return m_Device;
}

// static
std::unique_ptr<CComputePipelineState> CComputePipelineState::Create(
	CDevice* device, const SComputePipelineStateDesc& desc)
{
	std::unique_ptr<CComputePipelineState> pipelineState{new CComputePipelineState()};
	pipelineState->m_Device = device;
	pipelineState->m_Desc = desc;
	return pipelineState;
}

IDevice* CComputePipelineState::GetDevice()
{
	return m_Device;
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
