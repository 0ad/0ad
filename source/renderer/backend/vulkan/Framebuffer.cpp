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

#include "Framebuffer.h"

#include "renderer/backend/vulkan/Device.h"
#include "renderer/backend/vulkan/Mapping.h"
#include "renderer/backend/vulkan/RenderPassManager.h"
#include "renderer/backend/vulkan/Texture.h"
#include "renderer/backend/vulkan/Utilities.h"

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

// static
std::unique_ptr<CFramebuffer> CFramebuffer::Create(
	CDevice* device, const char* name,
	SColorAttachment* colorAttachment, SDepthStencilAttachment* depthStencilAttachment)
{
	ENSURE(colorAttachment || depthStencilAttachment);
	if (colorAttachment)
		ENSURE(colorAttachment->texture);
	if (depthStencilAttachment)
		ENSURE(depthStencilAttachment->texture);

	if (colorAttachment && depthStencilAttachment)
	{
		CTexture* colorAttachmentTexture = colorAttachment->texture->As<CTexture>();
		CTexture* depthStencilAttachmentTexture = depthStencilAttachment->texture->As<CTexture>();
		ENSURE(
			colorAttachmentTexture->GetWidth() == depthStencilAttachmentTexture->GetWidth() &&
			colorAttachmentTexture->GetHeight() == depthStencilAttachmentTexture->GetHeight() &&
			colorAttachmentTexture->GetSampleCount() == depthStencilAttachmentTexture->GetSampleCount());
	}

	std::unique_ptr<CFramebuffer> framebuffer(new CFramebuffer());
	framebuffer->m_Device = device;
	framebuffer->m_UID = device->GenerateNextDeviceObjectUID();
	if (colorAttachment)
		framebuffer->m_ClearColor = colorAttachment->clearColor;

	PS::StaticVector<VkImageView, 4> attachments;

	if (colorAttachment)
	{
		CTexture* colorAttachmentTexture = colorAttachment->texture->As<CTexture>();
		ENSURE(colorAttachmentTexture->GetUsage() & ITexture::Usage::COLOR_ATTACHMENT);

		framebuffer->m_Width = colorAttachmentTexture->GetWidth();
		framebuffer->m_Height = colorAttachmentTexture->GetHeight();
		framebuffer->m_SampleCount = colorAttachmentTexture->GetSampleCount();
		framebuffer->m_ColorAttachmentLoadOp = colorAttachment->loadOp;
		framebuffer->m_ColorAttachmentStoreOp = colorAttachment->storeOp;

		attachments.emplace_back(colorAttachmentTexture->GetAttachmentImageView());
		framebuffer->m_ColorAttachments.emplace_back(colorAttachmentTexture);
	}

	if (depthStencilAttachment)
	{
		CTexture* depthStencilAttachmentTexture = depthStencilAttachment->texture->As<CTexture>();
		ENSURE(depthStencilAttachmentTexture->GetUsage() & ITexture::Usage::DEPTH_STENCIL_ATTACHMENT);

		framebuffer->m_Width = depthStencilAttachmentTexture->GetWidth();
		framebuffer->m_Height = depthStencilAttachmentTexture->GetHeight();
		framebuffer->m_SampleCount = depthStencilAttachmentTexture->GetSampleCount();

		framebuffer->m_DepthStencilAttachmentLoadOp = depthStencilAttachment->loadOp;
		framebuffer->m_DepthStencilAttachmentStoreOp = depthStencilAttachment->storeOp;

		attachments.emplace_back(depthStencilAttachmentTexture->GetAttachmentImageView());
		framebuffer->m_DepthStencilAttachment = depthStencilAttachmentTexture;
	}

	ENSURE(framebuffer->m_Width > 0 && framebuffer->m_Height > 0);
	ENSURE(framebuffer->m_SampleCount > 0);

	framebuffer->m_RenderPass = device->GetRenderPassManager().GetOrCreateRenderPass(
		colorAttachment, depthStencilAttachment);

	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = framebuffer->m_RenderPass;
	framebufferInfo.attachmentCount = attachments.size();
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = framebuffer->m_Width;
	framebufferInfo.height = framebuffer->m_Height;
	framebufferInfo.layers = 1;

	ENSURE_VK_SUCCESS(vkCreateFramebuffer(
		device->GetVkDevice(), &framebufferInfo, nullptr, &framebuffer->m_Framebuffer));

	device->SetObjectName(VK_OBJECT_TYPE_FRAMEBUFFER, framebuffer->m_Framebuffer, name);

	return framebuffer;
}

CFramebuffer::~CFramebuffer()
{
	if (m_Framebuffer != VK_NULL_HANDLE)
		m_Device->ScheduleObjectToDestroy(
			VK_OBJECT_TYPE_FRAMEBUFFER, m_Framebuffer, VK_NULL_HANDLE);
}

IDevice* CFramebuffer::GetDevice()
{
	return m_Device;
}

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer
