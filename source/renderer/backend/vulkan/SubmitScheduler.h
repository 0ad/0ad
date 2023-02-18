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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_SUBMITSCHEDULER
#define INCLUDED_RENDERER_BACKEND_VULKAN_SUBMITSCHEDULER

#include "renderer/backend/vulkan/Device.h"

#include <glad/vulkan.h>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice;
class CRingCommandContext;
class CSwapChain;

/**
 * A helper class to batch VkQueueSubmit calls and track VkCommandBuffer usages
 * properly.
 */
class CSubmitScheduler
{
public:
	using SubmitHandle = uint32_t;
	static constexpr SubmitHandle INVALID_SUBMIT_HANDLE = 0;

	CSubmitScheduler(CDevice* device, const uint32_t queueFamilyIndex, VkQueue queue);
	~CSubmitScheduler();

	bool AcquireNextImage(CSwapChain& swapChain);

	void Present(CSwapChain& swapChain);

	SubmitHandle Submit(VkCommandBuffer commandBuffer);

	void WaitUntilFree(const SubmitHandle handle);

	uint32_t GetFrameID() const { return m_FrameID; }

	void Flush();

private:
	CDevice* m_Device = nullptr;
	VkQueue m_Queue = VK_NULL_HANDLE;

	struct Fence
	{
		VkFence value = VK_NULL_HANDLE;
		SubmitHandle lastUsedHandle = INVALID_SUBMIT_HANDLE;
		bool inUse = false;
	};
	std::vector<Fence> m_Fences;
	uint32_t m_FenceIndex = 0;

	// We assume that we won't run so long that the frame ID will overflow.
	uint32_t m_FrameID = 0;
	SubmitHandle m_CurrentHandle = INVALID_SUBMIT_HANDLE + 1;
	struct SubmittedHandle
	{
		SubmitHandle value;
		uint32_t fenceIndex;
	};
	std::queue<SubmittedHandle> m_SubmittedHandles;

	// We can't reuse frame data immediately after present because it might
	// still be processing on GPU.
	struct FrameObject
	{
		// We need to wait for the image on GPU to draw to it.
		VkSemaphore acquireImageSemaphore = VK_NULL_HANDLE;
		// We need to present only after all submit work is done.
		VkSemaphore submitDone = VK_NULL_HANDLE;
	};
	std::array<FrameObject, NUMBER_OF_FRAMES_IN_FLIGHT> m_FrameObjects;

	VkSemaphore m_NextWaitSemaphore = VK_NULL_HANDLE;
	VkPipelineStageFlags m_NextWaitDstStageMask = 0;
	VkSemaphore m_NextSubmitSignalSemaphore = VK_NULL_HANDLE;

	std::vector<VkCommandBuffer> m_SubmittedCommandBuffers;

	std::unique_ptr<CRingCommandContext> m_AcquireCommandContext;
	std::unique_ptr<CRingCommandContext> m_PresentCommandContext;
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_SUBMITSCHEDULER
