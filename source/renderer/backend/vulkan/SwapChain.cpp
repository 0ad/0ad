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

#include "SwapChain.h"

#include "lib/hash.h"
#include "maths/MathUtil.h"
#include "ps/ConfigDB.h"
#include "ps/Profile.h"
#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Framebuffer.h"
#include "renderer/backend/vulkan/RingCommandContext.h"
#include "renderer/backend/vulkan/Texture.h"
#include "renderer/backend/vulkan/Utilities.h"

#include <algorithm>
#include <limits>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

// static
std::unique_ptr<CSwapChain> CSwapChain::Create(
	CDevice* device, VkSurfaceKHR surface, int surfaceDrawableWidth, int surfaceDrawableHeight,
	std::unique_ptr<CSwapChain> oldSwapChain)
{
	VkPhysicalDevice physicalDevice = device->GetChoosenPhysicalDevice().device;
	
	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		physicalDevice, surface, &surfaceCapabilities));

	const uint32_t swapChainWidth = Clamp<uint32_t>(surfaceDrawableWidth,
		surfaceCapabilities.minImageExtent.width,
		surfaceCapabilities.maxImageExtent.width);
	const uint32_t swapChainHeight = Clamp<uint32_t>(surfaceDrawableHeight,
		surfaceCapabilities.minImageExtent.height,
		surfaceCapabilities.maxImageExtent.height);

	// Some drivers (for example NVIDIA on Windows during minimize) might
	// return zeroes for both minImageExtent and maxImageExtent. It means we're
	// not able to create any swapchain. Because we can't choose zeros (they're
	// not allowed) and we can't choose values bigger than maxImageExtent
	// (which are also zeroes in that case).
	if (swapChainWidth == 0 || swapChainHeight == 0)
		return nullptr;

	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	uint32_t surfaceFormatCount = 0;
	ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(
		physicalDevice, surface, &surfaceFormatCount, nullptr));
	if (surfaceFormatCount > 0)
	{
		surfaceFormats.resize(surfaceFormatCount);
		ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(
			physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()));
	}

	std::vector<VkPresentModeKHR> presentModes;
	uint32_t presentModeCount = 0;
	ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(
		physicalDevice, surface, &presentModeCount, nullptr));
	if (presentModeCount > 0)
	{
		presentModes.resize(presentModeCount);
		ENSURE_VK_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(
			physicalDevice, surface, &presentModeCount, presentModes.data()));
	}

	// VK_PRESENT_MODE_FIFO_KHR is guaranteed to be supported.
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	auto isPresentModeAvailable = [&presentModes](const VkPresentModeKHR presentMode)
	{
		return std::find(presentModes.begin(), presentModes.end(), presentMode) != presentModes.end();
	};
	bool vsyncEnabled = true;
	CFG_GET_VAL("vsync", vsyncEnabled);
	if (vsyncEnabled)
	{
		// TODO: use the adaptive one when possible.
		// https://gitlab.freedesktop.org/mesa/mesa/-/issues/5516
		//if (isPresentModeAvailable(VK_PRESENT_MODE_MAILBOX_KHR))
		//	presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	}
	else
	{
		if (isPresentModeAvailable(VK_PRESENT_MODE_IMMEDIATE_KHR))
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

	// Spec says:
	//   The number of format pairs supported must be greater than or equal to 1.
	//   pSurfaceFormats must not contain an entry whose value for format is
	//   VK_FORMAT_UNDEFINED.
	const auto surfaceFormatIt =
		std::find_if(surfaceFormats.begin(), surfaceFormats.end(), IsSurfaceFormatSupported);
	if (surfaceFormatIt == surfaceFormats.end())
	{
		LOGERROR("Can't find a suitable surface format to render to.");
		return nullptr;
	}
	const VkSurfaceFormatKHR& surfaceFormat = *surfaceFormatIt;

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;
	// minImageCount + 1 is to have a less chance for a presenter to wait.
	// maxImageCount might be zero, it means it's unlimited.
	const uint32_t maxImageCount = surfaceCapabilities.maxImageCount > 0
		? surfaceCapabilities.maxImageCount
		: std::numeric_limits<uint32_t>::max();
	const uint32_t minImageCount = surfaceCapabilities.minImageCount < maxImageCount
		? surfaceCapabilities.minImageCount + 1
		: surfaceCapabilities.minImageCount;
	swapChainCreateInfo.minImageCount =
		Clamp<uint32_t>(NUMBER_OF_FRAMES_IN_FLIGHT,
			minImageCount, maxImageCount);
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent.width = swapChainWidth;
	swapChainCreateInfo.imageExtent.height = swapChainHeight;
	swapChainCreateInfo.imageArrayLayers = 1;
	// VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT is guaranteed to present.
	// VK_IMAGE_USAGE_TRANSFER_SRC_BIT allows a simpler backbuffer readback.
	// VK_IMAGE_USAGE_TRANSFER_DST_BIT allows a blit to the backbuffer.
	swapChainCreateInfo.imageUsage =
		(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT) &
		surfaceCapabilities.supportedUsageFlags;
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// We need to set these only if imageSharingMode is VK_SHARING_MODE_CONCURRENT.
	swapChainCreateInfo.queueFamilyIndexCount = 0;
	swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	// By default VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR is preferable.
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	else
		swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
	// By default VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR is preferable, other bits
	// might require some format or rendering adjustemnts to avoid
	// semi-transparent areas.
	const VkCompositeAlphaFlagBitsKHR compositeAlphaOrder[] =
	{
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
	};
	for (const VkCompositeAlphaFlagBitsKHR compositeAlpha : compositeAlphaOrder)
		if (compositeAlpha & surfaceCapabilities.supportedCompositeAlpha)
		{
			swapChainCreateInfo.compositeAlpha = compositeAlpha;
			break;
		}
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	if (oldSwapChain)
		swapChainCreateInfo.oldSwapchain = oldSwapChain->GetVkSwapchain();

	std::unique_ptr<CSwapChain> swapChain(new CSwapChain());
	swapChain->m_Device = device;
	ENSURE_VK_SUCCESS(vkCreateSwapchainKHR(
		device->GetVkDevice(), &swapChainCreateInfo, nullptr, &swapChain->m_SwapChain));

	char nameBuffer[64];
	snprintf(nameBuffer, std::size(nameBuffer), "SwapChain: %dx%d", surfaceDrawableWidth, surfaceDrawableHeight);
	device->SetObjectName(VK_OBJECT_TYPE_SWAPCHAIN_KHR, swapChain->m_SwapChain, nameBuffer);

	uint32_t imageCount = 0;
	VkResult getSwapchainImagesResult = VK_INCOMPLETE;
	do
	{
		getSwapchainImagesResult = vkGetSwapchainImagesKHR(
			device->GetVkDevice(), swapChain->m_SwapChain, &imageCount, nullptr);
		if (getSwapchainImagesResult == VK_SUCCESS && imageCount > 0)
		{
			swapChain->m_Images.resize(imageCount);
			getSwapchainImagesResult = vkGetSwapchainImagesKHR(
				device->GetVkDevice(), swapChain->m_SwapChain, &imageCount, swapChain->m_Images.data());
		}
	} while (getSwapchainImagesResult == VK_INCOMPLETE);
	LOGMESSAGE("SwapChain image count: %u (min: %u)", imageCount, swapChainCreateInfo.minImageCount);
	ENSURE_VK_SUCCESS(getSwapchainImagesResult);
	ENSURE(imageCount > 0);

	swapChain->m_DepthTexture = CTexture::Create(
		device, "SwapChainDepthTexture", ITexture::Type::TEXTURE_2D,
		ITexture::Usage::DEPTH_STENCIL_ATTACHMENT,
		device->GetPreferredDepthStencilFormat(
			Renderer::Backend::ITexture::Usage::DEPTH_STENCIL_ATTACHMENT,
			true, true),
		swapChainWidth, swapChainHeight, Sampler::MakeDefaultSampler(
			Sampler::Filter::NEAREST, Sampler::AddressMode::CLAMP_TO_EDGE),
		1, 1);

	swapChain->m_ImageFormat = swapChainCreateInfo.imageFormat;

	swapChain->m_Textures.resize(imageCount);
	swapChain->m_Backbuffers.resize(imageCount);
	for (size_t index = 0; index < imageCount; ++index)
	{
		snprintf(nameBuffer, std::size(nameBuffer), "SwapChainImage #%zu", index);
		device->SetObjectName(VK_OBJECT_TYPE_IMAGE, swapChain->m_Images[index], nameBuffer);
		snprintf(nameBuffer, std::size(nameBuffer), "SwapChainImageView #%zu", index);
		swapChain->m_Textures[index] = CTexture::WrapBackbufferImage(
			device, nameBuffer, swapChain->m_Images[index], swapChainCreateInfo.imageFormat,
			swapChainCreateInfo.imageUsage, swapChainWidth, swapChainHeight);
	}

	swapChain->m_IsValid = true;

	return swapChain;
}

CSwapChain::CSwapChain() = default;

CSwapChain::~CSwapChain()
{
	m_Backbuffers.clear();

	m_Textures.clear();
	m_DepthTexture.reset();

	if (m_SwapChain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(m_Device->GetVkDevice(), m_SwapChain, nullptr);
}

size_t CSwapChain::SwapChainBackbuffer::BackbufferKeyHash::operator()(const BackbufferKey& key) const
{
	size_t seed = 0;
	hash_combine(seed, std::get<0>(key));
	hash_combine(seed, std::get<1>(key));
	hash_combine(seed, std::get<2>(key));
	hash_combine(seed, std::get<3>(key));
	return seed;
}

CSwapChain::SwapChainBackbuffer::SwapChainBackbuffer() = default;

CSwapChain::SwapChainBackbuffer::SwapChainBackbuffer(SwapChainBackbuffer&& other) = default;

CSwapChain::SwapChainBackbuffer& CSwapChain::SwapChainBackbuffer::operator=(SwapChainBackbuffer&& other) = default;

bool CSwapChain::AcquireNextImage(VkSemaphore acquireImageSemaphore)
{
	ENSURE(m_CurrentImageIndex == std::numeric_limits<uint32_t>::max());

	const VkResult acquireResult = vkAcquireNextImageKHR(
		m_Device->GetVkDevice(), m_SwapChain, std::numeric_limits<uint64_t>::max(),
		acquireImageSemaphore,
		VK_NULL_HANDLE, &m_CurrentImageIndex);
	if (acquireResult != VK_SUCCESS)
	{
		if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
			m_IsValid = false;
		else if (acquireResult != VK_SUBOPTIMAL_KHR)
		{
			LOGERROR("Acquire result: %d (%s)",
				static_cast<int>(acquireResult), Utilities::GetVkResultName(acquireResult));
			debug_warn("Unknown acquire error.");
		}
	}
	return m_IsValid;
}

void CSwapChain::SubmitCommandsAfterAcquireNextImage(
	CRingCommandContext& commandContext)
{
	const bool firstAcquirement = !m_Textures[m_CurrentImageIndex]->IsInitialized();
	Utilities::SubmitImageMemoryBarrier(
		commandContext.GetCommandBuffer(),
		m_Images[m_CurrentImageIndex], 0, 0,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		firstAcquirement ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	if (!m_DepthTexture->IsInitialized())
	{
		Utilities::SubmitImageMemoryBarrier(
			commandContext.GetCommandBuffer(),
			m_DepthTexture->GetImage(), 0, 0,
			0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
	}
}

void CSwapChain::SubmitCommandsBeforePresent(
	CRingCommandContext& commandContext)
{
	ENSURE(m_CurrentImageIndex != std::numeric_limits<uint32_t>::max());

	Utilities::SubmitImageMemoryBarrier(
		commandContext.GetCommandBuffer(), m_Images[m_CurrentImageIndex], 0, 0,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}

void CSwapChain::Present(VkSemaphore submitDone, VkQueue queue)
{
	ENSURE(m_CurrentImageIndex != std::numeric_limits<uint32_t>::max());

	VkSwapchainKHR swapChains[] = {m_SwapChain};

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &m_CurrentImageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &submitDone;
	const VkResult presentResult = vkQueuePresentKHR(queue, &presentInfo);
	if (presentResult != VK_SUCCESS)
	{
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
			m_IsValid = false;
		else if (presentResult != VK_SUBOPTIMAL_KHR)
		{
			LOGERROR("Present result: %d (%s)",
				static_cast<int>(presentResult), Utilities::GetVkResultName(presentResult));
			debug_warn("Unknown present error.");
		}
	}

	m_CurrentImageIndex = std::numeric_limits<uint32_t>::max();
}

CFramebuffer* CSwapChain::GetCurrentBackbuffer(
	const AttachmentLoadOp colorAttachmentLoadOp,
	const AttachmentStoreOp colorAttachmentStoreOp,
	const AttachmentLoadOp depthStencilAttachmentLoadOp,
	const AttachmentStoreOp depthStencilAttachmentStoreOp)
{
	ENSURE(m_CurrentImageIndex != std::numeric_limits<uint32_t>::max());
	SwapChainBackbuffer& swapChainBackbuffer =
		m_Backbuffers[m_CurrentImageIndex];
	const SwapChainBackbuffer::BackbufferKey key{
		colorAttachmentLoadOp, colorAttachmentStoreOp,
		depthStencilAttachmentLoadOp, depthStencilAttachmentStoreOp};
	auto it = swapChainBackbuffer.backbuffers.find(key);
	if (it == swapChainBackbuffer.backbuffers.end())
	{
		char nameBuffer[64];
		snprintf(nameBuffer, std::size(nameBuffer), "Backbuffer #%u", m_CurrentImageIndex);

		SColorAttachment colorAttachment{};
		colorAttachment.texture = m_Textures[m_CurrentImageIndex].get();
		colorAttachment.loadOp = colorAttachmentLoadOp;
		colorAttachment.storeOp = colorAttachmentStoreOp;

		SDepthStencilAttachment depthStencilAttachment{};
		depthStencilAttachment.texture = m_DepthTexture.get();
		depthStencilAttachment.loadOp = depthStencilAttachmentLoadOp;
		depthStencilAttachment.storeOp = depthStencilAttachmentStoreOp;

		it = swapChainBackbuffer.backbuffers.emplace(key, CFramebuffer::Create(
			m_Device, nameBuffer, &colorAttachment, &depthStencilAttachment)).first;
	}
	return it->second.get();
}

CTexture* CSwapChain::GetCurrentBackbufferTexture()
{
	ENSURE(m_CurrentImageIndex != std::numeric_limits<uint32_t>::max());
	return m_Textures[m_CurrentImageIndex].get();
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
