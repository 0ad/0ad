/* Copyright (C) 2023 Wildfire Games.
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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_RENDERPASSMANAGER
#define INCLUDED_RENDERER_BACKEND_VULKAN_RENDERPASSMANAGER

#include "renderer/backend/IFramebuffer.h"

#include <glad/vulkan.h>
#include <optional>
#include <unordered_map>
#include <vector>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice;

/**
 * A helper class to store unique render passes.
 */
class CRenderPassManager
{
public:
	CRenderPassManager(CDevice* device);
	~CRenderPassManager();

	/**
	 * @return a renderpass with required attachments. Currently we use only
	 * single subpass renderpasses.
	 * @note it should be called as rarely as possible.
	 */
	VkRenderPass GetOrCreateRenderPass(
		SColorAttachment* colorAttachment,
		SDepthStencilAttachment* depthStencilAttachment);

private:
	CDevice* m_Device = nullptr;

	struct Attachment
	{
		VkFormat format;
		AttachmentLoadOp loadOp;
		AttachmentStoreOp storeOp;
	};
	struct Desc
	{
		uint8_t sampleCount;
		std::optional<Attachment> colorAttachment;
		std::optional<Attachment> depthStencilAttachment;
	};
	struct DescHash
	{
		size_t operator()(const Desc& desc) const;
	};
	struct DescEqual
	{
		bool operator()(const Desc& lhs, const Desc& rhs) const;
	};
	std::unordered_map<Desc, VkRenderPass, DescHash, DescEqual> m_RenderPassMap;
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_RENDERPASSMANAGER
