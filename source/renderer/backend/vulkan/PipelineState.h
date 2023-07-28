/* Copyright (C) 2023 Wildfire Games.
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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_PIPELINESTATE
#define INCLUDED_RENDERER_BACKEND_VULKAN_PIPELINESTATE

#include "renderer/backend/PipelineState.h"
#include "renderer/backend/vulkan/Framebuffer.h"
#include "renderer/backend/vulkan/ShaderProgram.h"

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

	using UID = uint32_t;
	UID GetUID() const { return m_UID; }

private:
	friend class CDevice;

	static std::unique_ptr<CGraphicsPipelineState> Create(
		CDevice* device, const SGraphicsPipelineStateDesc& desc);

	CGraphicsPipelineState()
	{
		static uint32_t m_LastAvailableUID = 1;
		m_UID = m_LastAvailableUID++;
	}

	CDevice* m_Device = nullptr;

	UID m_UID = 0;

	SGraphicsPipelineStateDesc m_Desc{};

	struct CacheKey
	{
		CVertexInputLayout::UID vertexInputLayoutUID;
		// TODO: try to replace the UID by the only required parameters.
		CFramebuffer::UID framebufferUID;
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

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_PIPELINESTATE
