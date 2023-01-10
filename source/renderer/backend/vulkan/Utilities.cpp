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

#include "precompiled.h"

#include "Utilities.h"

#include "lib/code_annotation.h"
#include "lib/config2.h"
#include "renderer/backend/vulkan/Buffer.h"
#include "renderer/backend/vulkan/Texture.h"

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

namespace Utilities
{

void SetTextureLayout(
	VkCommandBuffer commandBuffer, CTexture* texture,
	const VkImageLayout oldLayout, const VkImageLayout newLayout,
	const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask,
	const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask)
{
	ENSURE(texture->GetMIPLevelCount() == 1);
	ENSURE(texture->GetLayerCount() == 1);

	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.image = texture->GetImage();
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.subresourceRange.aspectMask = texture->GetAttachmentImageAspectMask();
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = texture->GetMIPLevelCount();
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = texture->GetLayerCount();

	vkCmdPipelineBarrier(commandBuffer,
		srcStageMask, dstStageMask, 0,
		0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	texture->SetInitialized();
}

void SubmitImageMemoryBarrier(
	VkCommandBuffer commandBuffer, VkImage image, const uint32_t level, const uint32_t layer,
	const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask,
	const VkImageLayout oldLayout, const VkImageLayout newLayout,
	const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask,
	const VkImageAspectFlags aspectMask)
{
	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldLayout;
	imageMemoryBarrier.newLayout = newLayout;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
	imageMemoryBarrier.subresourceRange.baseMipLevel = level;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = layer;
	imageMemoryBarrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(commandBuffer,
		srcStageMask, dstStageMask, 0,
		0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

void SubmitBufferMemoryBarrier(
	VkCommandBuffer commandBuffer, CBuffer* buffer,
	const uint32_t offset, const uint32_t size,
	const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask,
	const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask)
{
	VkBufferMemoryBarrier bufferMemoryBarrier{};
	bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.srcAccessMask = srcAccessMask;
	bufferMemoryBarrier.dstAccessMask = dstAccessMask;
	bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemoryBarrier.buffer = buffer->GetVkBuffer();
	bufferMemoryBarrier.offset = offset;
	bufferMemoryBarrier.size = size;

	vkCmdPipelineBarrier(
		commandBuffer, srcStageMask, dstStageMask, 0,
		0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);
}

void SubmitMemoryBarrier(
	VkCommandBuffer commandBuffer,
	const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask,
	const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask)
{
	VkMemoryBarrier memoryBarrier{};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = srcAccessMask;
	memoryBarrier.dstAccessMask = dstAccessMask;
	vkCmdPipelineBarrier(
		commandBuffer, srcStageMask, dstStageMask, 0,
		1, &memoryBarrier, 0, nullptr, 0, nullptr);
}

void SubmitPipelineBarrier(
	VkCommandBuffer commandBuffer,
	const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask)
{
	vkCmdPipelineBarrier(
		commandBuffer, srcStageMask, dstStageMask, 0,
		0, nullptr, 0, nullptr, 0, nullptr);
}

void SubmitDebugSyncMemoryBarrier(VkCommandBuffer commandBuffer)
{
	const VkAccessFlags accessMask =
		VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
		VK_ACCESS_INDEX_READ_BIT |
		VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
		VK_ACCESS_UNIFORM_READ_BIT |
		VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
		VK_ACCESS_SHADER_READ_BIT |
		VK_ACCESS_SHADER_WRITE_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_TRANSFER_READ_BIT |
		VK_ACCESS_TRANSFER_WRITE_BIT |
		VK_ACCESS_HOST_READ_BIT |
		VK_ACCESS_HOST_WRITE_BIT;
	SubmitMemoryBarrier(
		commandBuffer, accessMask, accessMask,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}

} // namespace Utilities

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
