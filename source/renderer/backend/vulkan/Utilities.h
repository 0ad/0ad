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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_UTILITIES
#define INCLUDED_RENDERER_BACKEND_VULKAN_UTILITIES

#include "ps/CStr.h"

#include <glad/vulkan.h>

#define ENSURE_VK_SUCCESS(EXPR) \
	do \
	{ \
		const VkResult result = (EXPR); \
		if (result != VK_SUCCESS) \
		{ \
			LOGERROR(#EXPR " returned %d instead of VK_SUCCESS", static_cast<int>(result)); \
			ENSURE(false && #EXPR); \
		} \
	} while (0)

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CBuffer;
class CTexture;

namespace Utilities
{

// https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples-(Legacy-synchronization-APIs)

void SetTextureLayout(
	VkCommandBuffer commandBuffer, CTexture* texture,
	const VkImageLayout oldLayout, const VkImageLayout newLayout,
	const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask,
	const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask);

void SubmitImageMemoryBarrier(
	VkCommandBuffer commandBuffer, VkImage image, const uint32_t level, const uint32_t layer,
	const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask,
	const VkImageLayout oldLayout, const VkImageLayout newLayout,
	const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask,
	const VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

void SubmitBufferMemoryBarrier(
	VkCommandBuffer commandBuffer, CBuffer* buffer,
	const VkDeviceSize offset, const VkDeviceSize size,
	const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask,
	const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask);

void SubmitMemoryBarrier(
	VkCommandBuffer commandBuffer,
	const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask,
	const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask);

void SubmitPipelineBarrier(
	VkCommandBuffer commandBuffer,
	const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask);

void SubmitDebugSyncMemoryBarrier(VkCommandBuffer commandBuffer);

} // namespace Utilities

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_UTILITIES
