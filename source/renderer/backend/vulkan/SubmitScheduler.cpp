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

#include "SubmitScheduler.h"

#include "ps/ConfigDB.h"
#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/RingCommandContext.h"
#include "renderer/backend/vulkan/SwapChain.h"
#include "renderer/backend/vulkan/Utilities.h"

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

CSubmitScheduler::CSubmitScheduler(
	CDevice* device, const uint32_t queueFamilyIndex, VkQueue queue)
	: m_Device(device), m_Queue(queue)
{
	// Currently we need exactly NUMBER_OF_FRAMES_IN_FLIGHT fences to avoid
	// possible overlapping of different work between frames.
	constexpr size_t numberOfFences = NUMBER_OF_FRAMES_IN_FLIGHT;
	m_Fences.reserve(numberOfFences);
	for (size_t index = 0; index < numberOfFences; ++index)
	{
		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence = VK_NULL_HANDLE;
		ENSURE_VK_SUCCESS(vkCreateFence(
			m_Device->GetVkDevice(), &fenceCreateInfo, nullptr, &fence));
		m_Fences.push_back({fence, INVALID_SUBMIT_HANDLE});
	}

	for (FrameObject& frameObject : m_FrameObjects)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		ENSURE_VK_SUCCESS(vkCreateSemaphore(
			device->GetVkDevice(), &semaphoreCreateInfo, nullptr, &frameObject.acquireImageSemaphore));

		ENSURE_VK_SUCCESS(vkCreateSemaphore(
			device->GetVkDevice(), &semaphoreCreateInfo, nullptr, &frameObject.submitDone));
	}

	m_AcquireCommandContext = std::make_unique<CRingCommandContext>(
		device, NUMBER_OF_FRAMES_IN_FLIGHT, queueFamilyIndex, *this);
	m_PresentCommandContext = std::make_unique<CRingCommandContext>(
		device, NUMBER_OF_FRAMES_IN_FLIGHT, queueFamilyIndex, *this);

	CFG_GET_VAL("renderer.backend.vulkan.debugwaitidlebeforeacquire", m_DebugWaitIdleBeforeAcquire);
	CFG_GET_VAL("renderer.backend.vulkan.debugwaitidlebeforepresent", m_DebugWaitIdleBeforePresent);
	CFG_GET_VAL("renderer.backend.vulkan.debugwaitidleafterpresent", m_DebugWaitIdleAfterPresent);
}

CSubmitScheduler::~CSubmitScheduler()
{
	VkDevice device = m_Device->GetVkDevice();

	for (Fence& fence : m_Fences)
		if (fence.value != VK_NULL_HANDLE)
			vkDestroyFence(device, fence.value, nullptr);

	for (FrameObject& frameObject : m_FrameObjects)
	{
		if (frameObject.acquireImageSemaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(device, frameObject.acquireImageSemaphore, nullptr);

		if (frameObject.submitDone != VK_NULL_HANDLE)
			vkDestroySemaphore(device, frameObject.submitDone, nullptr);
	}
}

bool CSubmitScheduler::AcquireNextImage(CSwapChain& swapChain)
{
	if (m_DebugWaitIdleBeforeAcquire)
		vkDeviceWaitIdle(m_Device->GetVkDevice());

	FrameObject& frameObject = m_FrameObjects[m_FrameID % m_FrameObjects.size()];
	if (!swapChain.AcquireNextImage(frameObject.acquireImageSemaphore))
		return false;
	swapChain.SubmitCommandsAfterAcquireNextImage(*m_AcquireCommandContext);
	
	m_NextWaitSemaphore = frameObject.acquireImageSemaphore;
	m_NextWaitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	m_AcquireCommandContext->Flush();
	return true;
}

void CSubmitScheduler::Present(CSwapChain& swapChain)
{
	FrameObject& frameObject = m_FrameObjects[m_FrameID % m_FrameObjects.size()];
	swapChain.SubmitCommandsBeforePresent(*m_PresentCommandContext);
	m_NextSubmitSignalSemaphore = frameObject.submitDone;
	m_PresentCommandContext->Flush();
	Flush();

	if (m_DebugWaitIdleBeforePresent)
		vkDeviceWaitIdle(m_Device->GetVkDevice());

	swapChain.Present(frameObject.submitDone, m_Queue);

	if (m_DebugWaitIdleAfterPresent)
		vkDeviceWaitIdle(m_Device->GetVkDevice());
}

CSubmitScheduler::SubmitHandle CSubmitScheduler::Submit(VkCommandBuffer commandBuffer)
{
	m_SubmittedCommandBuffers.emplace_back(commandBuffer);
	return m_CurrentHandle;
}

void CSubmitScheduler::WaitUntilFree(const SubmitHandle handle)
{
	// We haven't submitted the current handle.
	if (handle == m_CurrentHandle)
		Flush();

	VkDevice device = m_Device->GetVkDevice();
	while (!m_SubmittedHandles.empty() && handle >= m_SubmittedHandles.front().value)
	{
		Fence& fence = m_Fences[m_SubmittedHandles.front().fenceIndex];
		ENSURE(fence.inUse);
		m_SubmittedHandles.pop();
		ENSURE_VK_SUCCESS(vkWaitForFences(device, 1, &fence.value, VK_TRUE, std::numeric_limits<uint64_t>::max()));
		ENSURE_VK_SUCCESS(vkResetFences(device, 1, &fence.value));
		fence.inUse = false;
		fence.lastUsedHandle = INVALID_SUBMIT_HANDLE;
	}
}

void CSubmitScheduler::Flush()
{
	ENSURE(!m_SubmittedCommandBuffers.empty());

	Fence& fence = m_Fences[m_FenceIndex];
	if (fence.inUse)
		WaitUntilFree(fence.lastUsedHandle);
	fence.lastUsedHandle = m_CurrentHandle;
	fence.inUse = true;
	m_SubmittedHandles.push({m_CurrentHandle, m_FenceIndex});
	++m_CurrentHandle;
	m_FenceIndex = (m_FenceIndex + 1) % m_Fences.size();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	if (m_NextWaitSemaphore != VK_NULL_HANDLE)
	{
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_NextWaitSemaphore;
		submitInfo.pWaitDstStageMask = &m_NextWaitDstStageMask;
	}
	if (m_NextSubmitSignalSemaphore != VK_NULL_HANDLE)
	{
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_NextSubmitSignalSemaphore;
	}
	submitInfo.commandBufferCount = m_SubmittedCommandBuffers.size();
	submitInfo.pCommandBuffers = m_SubmittedCommandBuffers.data();

	ENSURE_VK_SUCCESS(vkQueueSubmit(m_Queue, 1, &submitInfo, fence.value));

	m_NextWaitSemaphore = VK_NULL_HANDLE;
	m_NextWaitDstStageMask = 0;
	m_NextSubmitSignalSemaphore = VK_NULL_HANDLE;

	m_SubmittedCommandBuffers.clear();
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
