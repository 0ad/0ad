/* Copyright (C) 2022 Wildfire Games.
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

#include "Framebuffer.h"

#include "lib/code_annotation.h"
#include "lib/config2.h"
#include "ps/CLogger.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/backend/gl/Texture.h"

namespace Renderer
{

namespace Backend
{

namespace GL
{

// static
std::unique_ptr<CFramebuffer> CFramebuffer::Create(
	CDevice* device, const char* name, SColorAttachment* colorAttachment,
	SDepthStencilAttachment* depthStencilAttachment)
{
	ENSURE(colorAttachment || depthStencilAttachment);

	std::unique_ptr<CFramebuffer> framebuffer(new CFramebuffer());
	framebuffer->m_Device = device;
	if (colorAttachment)
	{
		framebuffer->m_ClearColor = colorAttachment->clearColor;
		framebuffer->m_ColorAttachmentLoadOp = colorAttachment->loadOp;
		framebuffer->m_ColorAttachmentStoreOp = colorAttachment->storeOp;
	}
	if (depthStencilAttachment)
	{
		framebuffer->m_DepthStencilAttachmentLoadOp = depthStencilAttachment->loadOp;
		framebuffer->m_DepthStencilAttachmentStoreOp = depthStencilAttachment->storeOp;
	}

	glGenFramebuffersEXT(1, &framebuffer->m_Handle);
	if (!framebuffer->m_Handle)
	{
		LOGERROR("Failed to create CFramebuffer object");
		return nullptr;
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer->m_Handle);

	if (colorAttachment)
	{
		CTexture* colorAttachmentTexture = colorAttachment->texture->As<CTexture>();
		ENSURE(device->IsFramebufferFormatSupported(colorAttachmentTexture->GetFormat()));
		ENSURE(colorAttachmentTexture->GetUsage() & Renderer::Backend::ITexture::Usage::COLOR_ATTACHMENT);

		framebuffer->m_AttachmentMask |= GL_COLOR_BUFFER_BIT;

#if CONFIG2_GLES
		ENSURE(colorAttachmentTexture->GetType() == CTexture::Type::TEXTURE_2D);
		const GLenum textureTarget = GL_TEXTURE_2D;
#else
		const GLenum textureTarget = colorAttachmentTexture->GetType() == CTexture::Type::TEXTURE_2D_MULTISAMPLE ?
			GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
#endif
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0,
			textureTarget, colorAttachmentTexture->GetHandle(), 0);
	}

	if (depthStencilAttachment)
	{
		CTexture* depthStencilAttachmentTexture = depthStencilAttachment->texture->As<CTexture>();
		ENSURE(depthStencilAttachmentTexture->GetUsage() & Renderer::Backend::ITexture::Usage::DEPTH_STENCIL_ATTACHMENT);

		framebuffer->m_Width = depthStencilAttachmentTexture->GetWidth();
		framebuffer->m_Height = depthStencilAttachmentTexture->GetHeight();
		framebuffer->m_AttachmentMask |= GL_DEPTH_BUFFER_BIT;
		if (depthStencilAttachmentTexture->GetFormat() == Format::D24_S8)
			framebuffer->m_AttachmentMask |= GL_STENCIL_BUFFER_BIT;
		if (colorAttachment)
		{
			ENSURE(colorAttachment->texture->GetWidth() == depthStencilAttachmentTexture->GetWidth());
			ENSURE(colorAttachment->texture->GetHeight() == depthStencilAttachmentTexture->GetHeight());
			ENSURE(colorAttachment->texture->GetType() == depthStencilAttachmentTexture->GetType());
		}
		ENSURE(
			depthStencilAttachmentTexture->GetFormat() == Format::D16 ||
			depthStencilAttachmentTexture->GetFormat() == Format::D24 ||
			depthStencilAttachmentTexture->GetFormat() == Format::D32 ||
			depthStencilAttachmentTexture->GetFormat() == Format::D24_S8);
#if CONFIG2_GLES
		ENSURE(depthStencilAttachmentTexture->GetFormat() != Format::D24_S8);
		const GLenum attachment = GL_DEPTH_ATTACHMENT;
		ENSURE(depthStencilAttachmentTexture->GetType() == CTexture::Type::TEXTURE_2D);
		const GLenum textureTarget = GL_TEXTURE_2D;
#else
		const GLenum attachment = depthStencilAttachmentTexture->GetFormat() == Format::D24_S8 ?
			GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
		const GLenum textureTarget = depthStencilAttachmentTexture->GetType() == CTexture::Type::TEXTURE_2D_MULTISAMPLE ?
			GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
#endif
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment,
			textureTarget, depthStencilAttachmentTexture->GetHandle(), 0);
	}
	else
	{
		framebuffer->m_Width = colorAttachment->texture->GetWidth();
		framebuffer->m_Height = colorAttachment->texture->GetHeight();
	}

	ogl_WarnIfError();

#if !CONFIG2_GLES
	if (!colorAttachment)
	{
		glReadBuffer(GL_NONE);
		glDrawBuffer(GL_NONE);
	}
	else
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
#endif

	ogl_WarnIfError();

#if !CONFIG2_GLES
	if (framebuffer->m_Device->GetCapabilities().debugLabels)
	{
		glObjectLabel(GL_FRAMEBUFFER, framebuffer->m_Handle, -1, name);
	}
#else
	UNUSED2(name);
#endif

	const GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGERROR("CFramebuffer object incomplete: 0x%04X", status);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		return nullptr;
	}

	ogl_WarnIfError();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	return framebuffer;
}

// static
std::unique_ptr<CFramebuffer> CFramebuffer::CreateBackbuffer(
	CDevice* device,
	const AttachmentLoadOp colorAttachmentLoadOp,
	const AttachmentStoreOp colorAttachmentStoreOp,
	const AttachmentLoadOp depthStencilAttachmentLoadOp,
	const AttachmentStoreOp depthStencilAttachmentStoreOp)
{
	// Backbuffer for GL is a special case with a zero framebuffer.
	std::unique_ptr<CFramebuffer> framebuffer(new CFramebuffer());
	framebuffer->m_Device = device;
	framebuffer->m_AttachmentMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	framebuffer->m_ClearColor = CColor(0.0f, 0.0f, 0.0f, 0.0f);
	framebuffer->m_ColorAttachmentLoadOp = colorAttachmentLoadOp;
	framebuffer->m_ColorAttachmentStoreOp = colorAttachmentStoreOp;
	framebuffer->m_DepthStencilAttachmentLoadOp = depthStencilAttachmentLoadOp;
	framebuffer->m_DepthStencilAttachmentStoreOp = depthStencilAttachmentStoreOp;
	return framebuffer;
}

CFramebuffer::CFramebuffer() = default;

CFramebuffer::~CFramebuffer()
{
	if (m_Handle)
		glDeleteFramebuffersEXT(1, &m_Handle);
}

IDevice* CFramebuffer::GetDevice()
{
	return m_Device;
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
