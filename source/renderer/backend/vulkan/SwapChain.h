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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_SWAPCHAIN
#define INCLUDED_RENDERER_BACKEND_VULKAN_SWAPCHAIN

#include "renderer/backend/IFramebuffer.h"

#include <glad/vulkan.h>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice;
class CFramebuffer;
class CRingCommandContext;
class CTexture;

class CSwapChain final
{
public:
	~CSwapChain();

	VkSwapchainKHR GetVkSwapchain() { return m_SwapChain; }

	bool IsValid() const { return m_IsValid; }

	bool AcquireNextImage(VkSemaphore acquireImageSemaphore);
	void SubmitCommandsAfterAcquireNextImage(
		CRingCommandContext& commandContext);
	void SubmitCommandsBeforePresent(
		CRingCommandContext& commandContext);
	void Present(VkSemaphore submitDone, VkQueue queue);

	CFramebuffer* GetCurrentBackbuffer(
		const AttachmentLoadOp colorAttachmentLoadOp,
		const AttachmentStoreOp colorAttachmentStoreOp,
		const AttachmentLoadOp depthStencilAttachmentLoadOp,
		const AttachmentStoreOp depthStencilAttachmentStoreOp);

	CTexture* GetDepthTexture() { return m_DepthTexture.get(); }

private:
	friend class CDevice;

	static std::unique_ptr<CSwapChain> Create(
		CDevice* device, VkSurfaceKHR surface, int surfaceDrawableWidth, int surfaceDrawableHeight,
		std::unique_ptr<CSwapChain> oldSwapChain);

	CSwapChain();

	CDevice* m_Device = nullptr;

	bool m_IsValid = false;
	VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;

	uint32_t m_CurrentImageIndex = std::numeric_limits<uint32_t>::max();

	std::vector<VkImage> m_Images;
	std::vector<std::unique_ptr<CTexture>> m_Textures;
	std::unique_ptr<CTexture> m_DepthTexture;
	VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;

	struct SwapChainBackbuffer
	{
		using BackbufferKey = std::tuple<
			AttachmentLoadOp, AttachmentStoreOp,
			AttachmentLoadOp, AttachmentStoreOp>;
		struct BackbufferKeyHash
		{
			size_t operator()(const BackbufferKey& key) const;
		};
		std::unordered_map<
			BackbufferKey, std::unique_ptr<CFramebuffer>, BackbufferKeyHash> backbuffers;

		SwapChainBackbuffer();

		SwapChainBackbuffer(const SwapChainBackbuffer&) = delete;
		SwapChainBackbuffer& operator=(const SwapChainBackbuffer&) = delete;

		SwapChainBackbuffer(SwapChainBackbuffer&& other);
		SwapChainBackbuffer& operator=(SwapChainBackbuffer&& other);
	};
	std::vector<SwapChainBackbuffer> m_Backbuffers;
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_SWAPCHAIN
