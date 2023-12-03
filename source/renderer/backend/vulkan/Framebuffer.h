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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_FRAMEBUFFER
#define INCLUDED_RENDERER_BACKEND_VULKAN_FRAMEBUFFER

#include "ps/containers/StaticVector.h"
#include "renderer/backend/IFramebuffer.h"
#include "renderer/backend/vulkan/DeviceObjectUID.h"

#include <glad/vulkan.h>
#include <memory>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

class CDevice;
class CTexture;

class CFramebuffer final : public IFramebuffer
{
public:
	~CFramebuffer() override;

	IDevice* GetDevice() override;

	const CColor& GetClearColor() const override { return m_ClearColor; }

	uint32_t GetWidth() const override { return m_Width; }
	uint32_t GetHeight() const override { return m_Height; }
	uint32_t GetSampleCount() const { return m_SampleCount; }

	VkRenderPass GetRenderPass() const { return m_RenderPass; }
	VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }

	using ColorAttachments = PS::StaticVector<CTexture*, 4>;
	const ColorAttachments& GetColorAttachments() { return m_ColorAttachments; }
	CTexture* GetDepthStencilAttachment() { return m_DepthStencilAttachment; }

	AttachmentLoadOp GetColorAttachmentLoadOp() const { return m_ColorAttachmentLoadOp; }
	AttachmentStoreOp GetColorAttachmentStoreOp() const { return m_ColorAttachmentStoreOp; }
	AttachmentLoadOp GetDepthStencilAttachmentLoadOp() const { return m_DepthStencilAttachmentLoadOp; }
	AttachmentStoreOp GetDepthStencilAttachmentStoreOp() const { return m_DepthStencilAttachmentStoreOp; }

	DeviceObjectUID GetUID() const { return m_UID; }

private:
	friend class CDevice;
	friend class CSwapChain;

	static std::unique_ptr<CFramebuffer> Create(
		CDevice* device, const char* name,
		SColorAttachment* colorAttachment, SDepthStencilAttachment* depthStencilAttachment);

	CFramebuffer() = default;
	
	CDevice* m_Device = nullptr;

	DeviceObjectUID m_UID{INVALID_DEVICE_OBJECT_UID};

	CColor m_ClearColor{};

	uint32_t m_Width = 0;
	uint32_t m_Height = 0;
	uint32_t m_SampleCount = 0;

	AttachmentLoadOp m_ColorAttachmentLoadOp = AttachmentLoadOp::DONT_CARE;
	AttachmentStoreOp m_ColorAttachmentStoreOp = AttachmentStoreOp::DONT_CARE;
	AttachmentLoadOp m_DepthStencilAttachmentLoadOp = AttachmentLoadOp::DONT_CARE;
	AttachmentStoreOp m_DepthStencilAttachmentStoreOp = AttachmentStoreOp::DONT_CARE;

	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

	// It's reponsibility of CFramebuffer owner to guarantee lifetime of
	// attachments.
	ColorAttachments m_ColorAttachments;
	CTexture* m_DepthStencilAttachment = nullptr;
};

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_FRAMEBUFFER
