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

#include "precompiled.h"

#include "RingCommandContext.h"

#include "lib/bits.h"
#include "renderer/backend/vulkan/Buffer.h"
#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Utilities.h"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

namespace
{

constexpr uint32_t INITIAL_STAGING_BUFFER_CAPACITY = 1024 * 1024;
constexpr VkDeviceSize SMALL_HOST_TOTAL_MEMORY_THRESHOLD = 1024 * 1024 * 1024;
constexpr uint32_t MAX_SMALL_STAGING_BUFFER_CAPACITY = 64 * 1024 * 1024;
constexpr uint32_t MAX_STAGING_BUFFER_CAPACITY = 256 * 1024 * 1024;

constexpr uint32_t INVALID_OFFSET = std::numeric_limits<uint32_t>::max();

} // anonymous namespace

CRingCommandContext::CRingCommandContext(
	CDevice* device, const size_t size, const uint32_t queueFamilyIndex,
	CSubmitScheduler& submitScheduler)
	: m_Device(device), m_SubmitScheduler(submitScheduler)
{
	ENSURE(m_Device);

	m_OptimalBufferCopyOffsetAlignment = std::max(
		1u, static_cast<uint32_t>(m_Device->GetChoosenPhysicalDevice().properties.limits.optimalBufferCopyOffsetAlignment));
	// In case of small amount of host memory it's better to make uploading
	// slower rather than crashing due to OOM, because memory for a
	// staging buffer is allocated in the host memory.
	m_MaxStagingBufferCapacity =
		m_Device->GetChoosenPhysicalDevice().hostTotalMemory <= SMALL_HOST_TOTAL_MEMORY_THRESHOLD
			? MAX_SMALL_STAGING_BUFFER_CAPACITY
			: MAX_STAGING_BUFFER_CAPACITY;

	m_Ring.resize(size);
	for (RingItem& item : m_Ring)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfoInfo{};
		commandPoolCreateInfoInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfoInfo.queueFamilyIndex = queueFamilyIndex;
		ENSURE_VK_SUCCESS(vkCreateCommandPool(
			m_Device->GetVkDevice(), &commandPoolCreateInfoInfo,
			nullptr, &item.commandPool));

		VkCommandBufferAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = item.commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		ENSURE_VK_SUCCESS(vkAllocateCommandBuffers(
			m_Device->GetVkDevice(), &allocateInfo, &item.commandBuffer));
		device->SetObjectName(
			VK_OBJECT_TYPE_COMMAND_BUFFER, item.commandBuffer, "RingCommandBuffer");
	}
}

CRingCommandContext::~CRingCommandContext()
{
	VkDevice device = m_Device->GetVkDevice();
	for (RingItem& item : m_Ring)
	{
		if (item.commandBuffer != VK_NULL_HANDLE)
			vkFreeCommandBuffers(device, item.commandPool, 1, &item.commandBuffer);

		if (item.commandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(device, item.commandPool, nullptr);
	}
}

VkCommandBuffer CRingCommandContext::GetCommandBuffer()
{
	RingItem& item = m_Ring[m_RingIndex];
	if (!item.isBegan)
		Begin();
	return item.commandBuffer;
}

void CRingCommandContext::Flush()
{
	RingItem& item = m_Ring[m_RingIndex];
	if (!item.isBegan)
		return;

	End();

	item.handle = m_SubmitScheduler.Submit(item.commandBuffer);

	m_RingIndex = (m_RingIndex + 1) % m_Ring.size();
}

void CRingCommandContext::FlushAndWait()
{
	RingItem& item = m_Ring[m_RingIndex];
	ENSURE(item.isBegan);

	End();

	item.handle = m_SubmitScheduler.Submit(item.commandBuffer);
	WaitUntilFree(item);
}

void CRingCommandContext::ScheduleUpload(
	CTexture* texture, const Format dataFormat,
	const void* data, const size_t dataSize,
	const uint32_t level, const uint32_t layer)
{
	const uint32_t mininumSize = 1u;
	const uint32_t width = std::max(mininumSize, texture->GetWidth() >> level);
	const uint32_t height = std::max(mininumSize, texture->GetHeight() >> level);
	ScheduleUpload(
		texture, dataFormat, data, dataSize,
		0, 0, width, height, level, layer);
}

void CRingCommandContext::ScheduleUpload(
	CTexture* texture, const Format UNUSED(dataFormat),
	const void* data, const size_t dataSize,
	const uint32_t xOffset, const uint32_t yOffset,
	const uint32_t width, const uint32_t height,
	const uint32_t level, const uint32_t layer)
{
	ENSURE(texture->GetType() != ITexture::Type::TEXTURE_2D_MULTISAMPLE);
	const Format format = texture->GetFormat();
	if (texture->GetType() != ITexture::Type::TEXTURE_CUBE)
		ENSURE(layer == 0);
	ENSURE(format != Format::R8G8B8_UNORM);

	const bool isCompressedFormat =
		format == Format::BC1_RGB_UNORM ||
		format == Format::BC1_RGBA_UNORM ||
		format == Format::BC2_UNORM ||
		format == Format::BC3_UNORM;
	ENSURE(
		format == Format::R8_UNORM ||
		format == Format::R8G8_UNORM ||
		format == Format::R8G8B8A8_UNORM ||
		format == Format::A8_UNORM ||
		format == Format::L8_UNORM ||
		isCompressedFormat);

	// TODO: use a more precise format alignment.
	constexpr uint32_t formatAlignment = 16;
	const uint32_t offset = AcquireFreeSpace(dataSize, std::max(formatAlignment, m_OptimalBufferCopyOffsetAlignment));

	std::memcpy(static_cast<std::byte*>(m_StagingBuffer->GetMappedData()) + offset, data, dataSize);

	VkCommandBuffer commandBuffer = GetCommandBuffer();
	VkImage image = texture->GetImage();

	Utilities::SubmitImageMemoryBarrier(
		commandBuffer, image, level, layer,
		VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	VkBufferImageCopy region{};

	region.bufferOffset = offset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = level;
	region.imageSubresource.baseArrayLayer = layer;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {static_cast<int32_t>(xOffset), static_cast<int32_t>(yOffset), 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(
		commandBuffer, m_StagingBuffer->GetVkBuffer(), image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	VkAccessFlags dstAccessFlags = VK_ACCESS_SHADER_READ_BIT;
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	Utilities::SubmitImageMemoryBarrier(
		commandBuffer, image, level, layer,
		VK_ACCESS_TRANSFER_WRITE_BIT, dstAccessFlags,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, dstStageMask);
	texture->SetInitialized();
}

void CRingCommandContext::ScheduleUpload(
	CBuffer* buffer, const void* data, const uint32_t dataOffset,
	const uint32_t dataSize)
{
	constexpr uint32_t alignment = 16;
	const uint32_t offset = AcquireFreeSpace(dataSize, alignment);

	std::memcpy(static_cast<std::byte*>(m_StagingBuffer->GetMappedData()) + offset, data, dataSize);

	ScheduleUpload(buffer, dataOffset, dataSize, offset);
}

void CRingCommandContext::ScheduleUpload(
	CBuffer* buffer, const uint32_t dataOffset, const uint32_t dataSize,
	const UploadBufferFunction& uploadFunction)
{
	constexpr uint32_t alignment = 16;
	const uint32_t offset = AcquireFreeSpace(dataSize, alignment);

	CBuffer* stagingBuffer = m_StagingBuffer->As<CBuffer>();

	uploadFunction(static_cast<uint8_t*>(stagingBuffer->GetMappedData()) + offset - dataOffset);

	ScheduleUpload(buffer, dataOffset, dataSize, offset);
}

void CRingCommandContext::ScheduleUpload(
	CBuffer* buffer, const uint32_t dataOffset, const uint32_t dataSize,
	const uint32_t acquiredOffset)
{
	CBuffer* stagingBuffer = m_StagingBuffer->As<CBuffer>();
	VkCommandBuffer commandBuffer = GetCommandBuffer();

	VkBufferCopy region{};
	region.srcOffset = acquiredOffset;
	region.dstOffset = dataOffset;
	region.size = dataSize;

	// TODO: remove transfer mask from pipeline barrier, as we need to batch copies.
	VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	if (buffer->GetType() == IBuffer::Type::VERTEX || buffer->GetType() == IBuffer::Type::INDEX)
		srcStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	else if (buffer->GetType() == IBuffer::Type::UNIFORM)
		srcStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	Utilities::SubmitPipelineBarrier(
		commandBuffer, srcStageMask, dstStageMask);

	// TODO: currently we might overwrite data which triggers validation
	// assertion about Write-After-Write hazard.
	if (buffer->IsDynamic())
	{
		Utilities::SubmitBufferMemoryBarrier(
			commandBuffer, buffer, dataOffset, dataSize,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	}

	vkCmdCopyBuffer(
		commandBuffer, stagingBuffer->GetVkBuffer(), buffer->GetVkBuffer(), 1, &region);

	VkAccessFlags srcAccessFlags = VK_ACCESS_TRANSFER_WRITE_BIT;
	VkAccessFlags dstAccessFlags = 0;
	srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	dstStageMask = 0;
	if (buffer->GetType() == IBuffer::Type::VERTEX)
	{
		dstAccessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		dstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	}
	else if (buffer->GetType() == IBuffer::Type::INDEX)
	{
		dstAccessFlags = VK_ACCESS_INDEX_READ_BIT;
		dstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	}
	else if (buffer->GetType() == IBuffer::Type::UNIFORM)
	{
		dstAccessFlags = VK_ACCESS_UNIFORM_READ_BIT;
		dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	Utilities::SubmitBufferMemoryBarrier(
		commandBuffer, buffer, dataOffset, dataSize,
		srcAccessFlags, dstAccessFlags, srcStageMask, dstStageMask);
}

void CRingCommandContext::Begin()
{
	RingItem& item = m_Ring[m_RingIndex];
	item.isBegan = true;

	WaitUntilFree(item);

	m_StagingBufferCurrentFirst = m_StagingBufferLast;

	ENSURE_VK_SUCCESS(vkResetCommandPool(m_Device->GetVkDevice(), item.commandPool, 0));

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;
	ENSURE_VK_SUCCESS(vkBeginCommandBuffer(item.commandBuffer, &beginInfo));
}

void CRingCommandContext::End()
{
	RingItem& item = m_Ring[m_RingIndex];
	item.isBegan = false;
	item.stagingBufferFirst = m_StagingBufferCurrentFirst;
	item.stagingBufferLast = m_StagingBufferLast;

	ENSURE_VK_SUCCESS(vkEndCommandBuffer(item.commandBuffer));
}

void CRingCommandContext::WaitUntilFree(RingItem& item)
{
	m_SubmitScheduler.WaitUntilFree(item.handle);
	if (item.stagingBufferFirst != item.stagingBufferLast)
	{
		m_StagingBufferFirst = item.stagingBufferLast;
		item.stagingBufferFirst = 0;
		item.stagingBufferLast = 0;
	}
}

uint32_t CRingCommandContext::AcquireFreeSpace(
	const uint32_t requiredSize, const uint32_t requiredAlignment)
{
	ENSURE(requiredSize <= m_MaxStagingBufferCapacity);
	const uint32_t offsetCandidate =
		GetFreeSpaceOffset(requiredSize, requiredAlignment);
	const bool needsResize =
		!m_StagingBuffer || offsetCandidate == INVALID_OFFSET;
	const bool canResize =
		!m_StagingBuffer || m_StagingBuffer->GetSize() < m_MaxStagingBufferCapacity;
	if (needsResize && canResize)
	{
		const uint32_t minimumRequiredCapacity = round_up_to_pow2(requiredSize);
		const uint32_t newCapacity = std::min(
			std::max(m_StagingBuffer ? m_StagingBuffer->GetSize() * 2 : INITIAL_STAGING_BUFFER_CAPACITY, minimumRequiredCapacity),
			m_MaxStagingBufferCapacity);
		m_StagingBuffer = m_Device->CreateCBuffer(
			"UploadRingBuffer", IBuffer::Type::UPLOAD, newCapacity, false);
		ENSURE(m_StagingBuffer);
		m_StagingBufferFirst = 0;
		m_StagingBufferCurrentFirst = 0;
		m_StagingBufferLast = requiredSize;

		for (RingItem& item : m_Ring)
		{
			item.stagingBufferFirst = 0;
			item.stagingBufferLast = 0;
		}

		return 0;
	}
	else if (needsResize)
	{
		// In case we can't resize we need to wait until all scheduled uploads are
		// completed.
		for (size_t ringIndexOffset = 1; ringIndexOffset < m_Ring.size() && GetFreeSpaceOffset(requiredSize, requiredAlignment) == INVALID_OFFSET; ++ringIndexOffset)
		{
			const size_t ringIndex = (m_RingIndex + ringIndexOffset) % m_Ring.size();
			RingItem& item = m_Ring[ringIndex];
			WaitUntilFree(item);
		}
		// If we still don't have a free space it means we need to flush the
		// current command buffer.
		const uint32_t offset = GetFreeSpaceOffset(requiredSize, requiredAlignment);
		if (offset == INVALID_OFFSET)
		{
			RingItem& item = m_Ring[m_RingIndex];
			if (item.isBegan)
				Flush();
			WaitUntilFree(item);
			m_StagingBufferFirst = 0;
			m_StagingBufferCurrentFirst = 0;
			m_StagingBufferLast = requiredSize;
			return 0;
		}
		else
		{
			m_StagingBufferLast = offset + requiredSize;
			return offset;
		}
	}
	else
	{
		m_StagingBufferLast = offsetCandidate + requiredSize;
		return offsetCandidate;
	}
}

uint32_t CRingCommandContext::GetFreeSpaceOffset(
	const uint32_t requiredSize, const uint32_t requiredAlignment) const
{
	if (!m_StagingBuffer)
		return INVALID_OFFSET;
	const uint32_t candidateOffset =
		round_up(m_StagingBufferLast, requiredAlignment);
	const uint32_t candidateLast = candidateOffset + requiredSize;
	if (m_StagingBufferFirst <= m_StagingBufferLast)
	{
		if (candidateLast <= m_StagingBuffer->GetSize())
			return candidateOffset;
		// We intentionally use exclusive comparison to avoid distinguishing
		// completely full and completely empty staging buffers.
		else if (requiredSize < m_StagingBufferFirst)
			return 0; // We assume the first byte is always perfectly aligned.
		else
			return INVALID_OFFSET;
	}
	else
	{
		if (candidateLast < m_StagingBufferFirst)
			return candidateOffset;
		else
			return INVALID_OFFSET;
	}
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
