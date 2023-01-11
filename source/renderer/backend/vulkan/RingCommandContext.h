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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_RINGCOMMANDCONTEXT
#define INCLUDED_RENDERER_BACKEND_VULKAN_RINGCOMMANDCONTEXT

#include "renderer/backend/vulkan/SubmitScheduler.h"

#include <glad/vulkan.h>
#include <memory>
#include <vector>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CBuffer;
class CDevice;

/**
 * A simple helper class to decouple command buffers rotation from frames
 * presenting. It might be useful when sometimes we need to submit more work
 * than we can usually have during a frame. For example if we need to upload
 * something, an upload buffer is full and we can't extend it at the moment.
 * Then the only way is to wait until uploading is done and submit more work.
 * @note not thread-safe, should be created and used from the same thread.
 */
class CRingCommandContext
{
public:
	CRingCommandContext(
		CDevice* device, const size_t size, const uint32_t queueFamilyIndex,
		CSubmitScheduler& submitScheduler);
	~CRingCommandContext();

	/**
	 * @return the current available command buffer. If there is none waits until
	 * it appeared.
	 */
	VkCommandBuffer GetCommandBuffer();

	/**
	 * Submits the current command buffer to the SubmitScheduler.
	 */
	void Flush();

	/**
	 * Schedules uploads until next render pass or flush.
	 * @note doesn't save a command buffer returned by GetCommandBuffer during
	 * scheduling uploads, because it might be changed.
	 */
	void ScheduleUpload(
		CTexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t level, const uint32_t layer);
	void ScheduleUpload(
		CTexture* texture, const Format dataFormat,
		const void* data, const size_t dataSize,
		const uint32_t xOffset, const uint32_t yOffset,
		const uint32_t width, const uint32_t height,
		const uint32_t level, const uint32_t layer);

	void ScheduleUpload(
		CBuffer* buffer, const void* data, const uint32_t dataOffset,
		const uint32_t dataSize);
	using UploadBufferFunction = std::function<void(u8*)>;
	void ScheduleUpload(
		CBuffer* buffer,
		const uint32_t dataOffset, const uint32_t dataSize,
		const UploadBufferFunction& uploadFunction);

private:
	void Begin();
	void End();

	void ScheduleUpload(
		CBuffer* buffer, const uint32_t dataOffset, const uint32_t dataSize,
		const uint32_t acquiredOffset);

	uint32_t AcquireFreeSpace(
		const uint32_t requiredSize, const uint32_t requiredAlignment);
	uint32_t GetFreeSpaceOffset(
		const uint32_t requiredSize, const uint32_t requiredAlignment) const;

	CDevice* m_Device = nullptr;
	CSubmitScheduler& m_SubmitScheduler;

	std::unique_ptr<CBuffer> m_StagingBuffer;
	uint32_t m_StagingBufferFirst = 0, m_StagingBufferCurrentFirst = 0, m_StagingBufferLast = 0;
	uint32_t m_OptimalBufferCopyOffsetAlignment = 1;
	uint32_t m_MaxStagingBufferCapacity = 0;

	struct RingItem
	{
		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		CSubmitScheduler::SubmitHandle handle = CSubmitScheduler::INVALID_SUBMIT_HANDLE;
		bool isBegan = false;
		uint32_t stagingBufferFirst = 0, stagingBufferLast = 0;
	};
	std::vector<RingItem> m_Ring;
	size_t m_RingIndex = 0;

	void WaitUntilFree(RingItem& item);
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_RINGCOMMANDCONTEXT
