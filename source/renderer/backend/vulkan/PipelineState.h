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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_PIPELINESTATE
#define INCLUDED_RENDERER_BACKEND_VULKAN_PIPELINESTATE

#include "renderer/backend/PipelineState.h"
#include "renderer/backend/vulkan/Framebuffer.h"
#include "renderer/backend/vulkan/ShaderProgram.h"
#include "renderer/backend/vulkan/DeviceObjectUID.h"

#include <cstdint>
#include <glad/vulkan.h>
#include <memory>
#include <unordered_map>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice;
class CFramebuffer;

class CGraphicsPipelineState final : public IGraphicsPipelineState
{
public:
	~CGraphicsPipelineState() override;

	IDevice* GetDevice() override;

	IShaderProgram* GetShaderProgram() const override { return m_Desc.shaderProgram; }

	const SGraphicsPipelineStateDesc& GetDesc() const { return m_Desc; }

	VkPipeline GetOrCreatePipeline(
		const CVertexInputLayout* vertexInputLayout, CFramebuffer* framebuffer);

	DeviceObjectUID GetUID() const { return m_UID; }

private:
	friend class CDevice;

	static std::unique_ptr<CGraphicsPipelineState> Create(
		CDevice* device, const SGraphicsPipelineStateDesc& desc);

	CGraphicsPipelineState() = default;

	CDevice* m_Device = nullptr;

	DeviceObjectUID m_UID{INVALID_DEVICE_OBJECT_UID};

	SGraphicsPipelineStateDesc m_Desc{};

	struct CacheKey
	{
		DeviceObjectUID vertexInputLayoutUID;
		// TODO: try to replace the UID by the only required parameters.
		DeviceObjectUID framebufferUID;
	};
	struct CacheKeyHash
	{
		size_t operator()(const CacheKey& cacheKey) const;
	};
	struct CacheKeyEqual
	{
		bool operator()(const CacheKey& lhs, const CacheKey& rhs) const;
	};
	std::unordered_map<CacheKey, VkPipeline, CacheKeyHash, CacheKeyEqual> m_PipelineMap;
};

class CComputePipelineState final : public IComputePipelineState
{
public:
	~CComputePipelineState() override;

	IDevice* GetDevice() override;

	IShaderProgram* GetShaderProgram() const override { return m_Desc.shaderProgram; }

	const SComputePipelineStateDesc& GetDesc() const { return m_Desc; }

	VkPipeline GetPipeline() { return m_Pipeline; }

	DeviceObjectUID GetUID() const { return m_UID; }

private:
	friend class CDevice;

	static std::unique_ptr<CComputePipelineState> Create(
		CDevice* device, const SComputePipelineStateDesc& desc);

	CComputePipelineState() = default;

	CDevice* m_Device{nullptr};

	DeviceObjectUID m_UID{INVALID_DEVICE_OBJECT_UID};

	SComputePipelineStateDesc m_Desc{};

	VkPipeline m_Pipeline{VK_NULL_HANDLE};
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_PIPELINESTATE
