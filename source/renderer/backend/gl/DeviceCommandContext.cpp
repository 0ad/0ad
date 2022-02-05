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

#include "DeviceCommandContext.h"

#include "renderer/backend/gl/Device.h"
#include "renderer/backend/gl/Framebuffer.h"
#include "renderer/backend/gl/Mapping.h"
#include "renderer/backend/gl/Texture.h"

#include <limits>

namespace Renderer
{

namespace Backend
{

namespace GL
{

namespace
{

bool operator==(const StencilOpState& lhs, const StencilOpState& rhs)
{
	return
		lhs.failOp == rhs.failOp &&
		lhs.passOp == rhs.passOp &&
		lhs.depthFailOp == rhs.depthFailOp &&
		lhs.compareOp == rhs.compareOp;
}
bool operator!=(const StencilOpState& lhs, const StencilOpState& rhs)
{
	return !operator==(lhs, rhs);
}

bool operator==(
	const CDeviceCommandContext::ScissorRect& lhs,
	const CDeviceCommandContext::ScissorRect& rhs)
{
	return
		lhs.x == rhs.x && lhs.y == rhs.y &&
		lhs.width == rhs.width && lhs.height == rhs.height;
}

bool operator!=(
	const CDeviceCommandContext::ScissorRect& lhs,
	const CDeviceCommandContext::ScissorRect& rhs)
{
	return !operator==(lhs, rhs);
}

void ApplyDepthMask(const bool depthWriteEnabled)
{
	glDepthMask(depthWriteEnabled ? GL_TRUE : GL_FALSE);
}

void ApplyColorMask(const uint8_t colorWriteMask)
{
	glColorMask(
		(colorWriteMask & ColorWriteMask::RED) != 0 ? GL_TRUE : GL_FALSE,
		(colorWriteMask & ColorWriteMask::GREEN) != 0 ? GL_TRUE : GL_FALSE,
		(colorWriteMask & ColorWriteMask::BLUE) != 0 ? GL_TRUE : GL_FALSE,
		(colorWriteMask & ColorWriteMask::ALPHA) != 0 ? GL_TRUE : GL_FALSE);
}

void ApplyStencilMask(const uint32_t stencilWriteMask)
{
	glStencilMask(stencilWriteMask);
}

} // anonymous namespace

// static
std::unique_ptr<CDeviceCommandContext> CDeviceCommandContext::Create(CDevice* device)
{
	std::unique_ptr<CDeviceCommandContext> deviceCommandContext(new CDeviceCommandContext(device));
	deviceCommandContext->m_Framebuffer = device->GetCurrentBackbuffer();
	deviceCommandContext->ResetStates();
	return deviceCommandContext;
}

CDeviceCommandContext::CDeviceCommandContext(CDevice* device)
	: m_Device(device)
{
}

CDeviceCommandContext::~CDeviceCommandContext() = default;

void CDeviceCommandContext::SetGraphicsPipelineState(
	const GraphicsPipelineStateDesc& pipelineStateDesc)
{
	SetGraphicsPipelineStateImpl(pipelineStateDesc, false);
}

void CDeviceCommandContext::UploadTexture(
	CTexture* texture, const Format format,
	const void* data, const size_t dataSize,
	const uint32_t level, const uint32_t layer)
{
	UploadTextureRegion(texture, format, data, dataSize,
		0, 0, texture->GetWidth(), texture->GetHeight(), level, layer);
}

void CDeviceCommandContext::UploadTextureRegion(
	CTexture* texture, const Format dataFormat,
	const void* data, const size_t dataSize,
	const uint32_t xOffset, const uint32_t yOffset,
	const uint32_t width, const uint32_t height,
	const uint32_t level, const uint32_t layer)
{
	ENSURE(texture);
	if (texture->GetType() == CTexture::Type::TEXTURE_2D)
	{
		ENSURE(level == 0 && layer == 0);
		if (texture->GetFormat() == Format::R8G8B8A8 || texture->GetFormat() == Format::A8)
		{
			ENSURE(width > 0 && height > 0);
			ENSURE(texture->GetFormat() == dataFormat);
			const size_t bpp = dataFormat == Format::R8G8B8A8 ? 4 : 1;
			ENSURE(dataSize == width * height * bpp);
			ENSURE(xOffset + width <= texture->GetWidth());
			ENSURE(yOffset + height <= texture->GetHeight());

			glBindTexture(GL_TEXTURE_2D, texture->GetHandle());
			glTexSubImage2D(GL_TEXTURE_2D, level,
				xOffset, yOffset, width, height,
				dataFormat == Format::R8G8B8A8 ? GL_RGBA : GL_ALPHA, GL_UNSIGNED_BYTE, data);
			glBindTexture(GL_TEXTURE_2D, 0);

			ogl_WarnIfError();
		}
		else
			debug_warn("Unsupported format");
	}
	else if (texture->GetType() == CTexture::Type::TEXTURE_CUBE)
	{
		if (texture->GetFormat() == Format::R8G8B8A8)
		{
			ENSURE(texture->GetFormat() == dataFormat);
			ENSURE(level == 0 && layer < 6);
			ENSURE(xOffset == 0 && yOffset == 0 && texture->GetWidth() == width && texture->GetHeight() == height);
			const size_t bpp = 4;
			ENSURE(dataSize == width * height * bpp);

			// The order of layers should be the following:
			//   front, back, top, bottom, right, left
			static const GLenum targets[6] =
			{
				GL_TEXTURE_CUBE_MAP_POSITIVE_X,
				GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
				GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
				GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
				GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
				GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
			};

			glBindTexture(GL_TEXTURE_CUBE_MAP, texture->GetHandle());
			glTexImage2D(targets[layer], level, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

			ogl_WarnIfError();
		}
		else
			debug_warn("Unsupported format");
	}
	else
		debug_warn("Unsupported type");
}

void CDeviceCommandContext::Flush()
{
	ResetStates();
}

void CDeviceCommandContext::ResetStates()
{
	SetGraphicsPipelineStateImpl(MakeDefaultGraphicsPipelineStateDesc(), true);
	SetScissors(0, nullptr);
	SetFramebuffer(m_Device->GetCurrentBackbuffer());
}

void CDeviceCommandContext::SetGraphicsPipelineStateImpl(
	const GraphicsPipelineStateDesc& pipelineStateDesc, const bool force)
{
	const DepthStencilStateDesc& currentDepthStencilStateDesc = m_GraphicsPipelineStateDesc.depthStencilState;
	const DepthStencilStateDesc& nextDepthStencilStateDesc = pipelineStateDesc.depthStencilState;
	if (force || currentDepthStencilStateDesc.depthTestEnabled != nextDepthStencilStateDesc.depthTestEnabled)
	{
		if (nextDepthStencilStateDesc.depthTestEnabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}
	if (force || currentDepthStencilStateDesc.depthCompareOp != nextDepthStencilStateDesc.depthCompareOp)
	{
		glDepthFunc(Mapping::FromCompareOp(nextDepthStencilStateDesc.depthCompareOp));
	}
	if (force || currentDepthStencilStateDesc.depthWriteEnabled != nextDepthStencilStateDesc.depthWriteEnabled)
	{
		ApplyDepthMask(nextDepthStencilStateDesc.depthWriteEnabled);
	}

	if (force || currentDepthStencilStateDesc.stencilTestEnabled != nextDepthStencilStateDesc.stencilTestEnabled)
	{
		if (nextDepthStencilStateDesc.stencilTestEnabled)
			glEnable(GL_STENCIL_TEST);
		else
			glDisable(GL_STENCIL_TEST);
	}
	if (force ||
		currentDepthStencilStateDesc.stencilFrontFace != nextDepthStencilStateDesc.stencilFrontFace ||
		currentDepthStencilStateDesc.stencilBackFace != nextDepthStencilStateDesc.stencilBackFace)
	{
		if (nextDepthStencilStateDesc.stencilFrontFace == nextDepthStencilStateDesc.stencilBackFace)
		{
			glStencilOp(
				Mapping::FromStencilOp(nextDepthStencilStateDesc.stencilFrontFace.failOp),
				Mapping::FromStencilOp(nextDepthStencilStateDesc.stencilFrontFace.depthFailOp),
				Mapping::FromStencilOp(nextDepthStencilStateDesc.stencilFrontFace.passOp));
		}
		else
		{
			if (force || currentDepthStencilStateDesc.stencilFrontFace != nextDepthStencilStateDesc.stencilFrontFace)
			{
				glStencilOpSeparate(
					GL_FRONT,
					Mapping::FromStencilOp(nextDepthStencilStateDesc.stencilFrontFace.failOp),
					Mapping::FromStencilOp(nextDepthStencilStateDesc.stencilFrontFace.depthFailOp),
					Mapping::FromStencilOp(nextDepthStencilStateDesc.stencilFrontFace.passOp));
			}
			if (force || currentDepthStencilStateDesc.stencilBackFace != nextDepthStencilStateDesc.stencilBackFace)
			{
				glStencilOpSeparate(
					GL_BACK,
					Mapping::FromStencilOp(nextDepthStencilStateDesc.stencilBackFace.failOp),
					Mapping::FromStencilOp(nextDepthStencilStateDesc.stencilBackFace.depthFailOp),
					Mapping::FromStencilOp(nextDepthStencilStateDesc.stencilBackFace.passOp));
			}
		}
	}
	if (force || currentDepthStencilStateDesc.stencilWriteMask != nextDepthStencilStateDesc.stencilWriteMask)
	{
		ApplyStencilMask(nextDepthStencilStateDesc.stencilWriteMask);
	}
	if (force ||
		currentDepthStencilStateDesc.stencilReference != nextDepthStencilStateDesc.stencilReference ||
		currentDepthStencilStateDesc.stencilReadMask != nextDepthStencilStateDesc.stencilReadMask ||
		currentDepthStencilStateDesc.stencilFrontFace.compareOp != nextDepthStencilStateDesc.stencilFrontFace.compareOp ||
		currentDepthStencilStateDesc.stencilBackFace.compareOp != nextDepthStencilStateDesc.stencilBackFace.compareOp)
	{
		if (nextDepthStencilStateDesc.stencilFrontFace.compareOp == nextDepthStencilStateDesc.stencilBackFace.compareOp)
		{
			glStencilFunc(
				Mapping::FromCompareOp(nextDepthStencilStateDesc.stencilFrontFace.compareOp),
				nextDepthStencilStateDesc.stencilReference,
				nextDepthStencilStateDesc.stencilReadMask);
		}
		else
		{
			glStencilFuncSeparate(GL_FRONT,
				Mapping::FromCompareOp(nextDepthStencilStateDesc.stencilFrontFace.compareOp),
				nextDepthStencilStateDesc.stencilReference,
				nextDepthStencilStateDesc.stencilReadMask);
			glStencilFuncSeparate(GL_BACK,
				Mapping::FromCompareOp(nextDepthStencilStateDesc.stencilBackFace.compareOp),
				nextDepthStencilStateDesc.stencilReference,
				nextDepthStencilStateDesc.stencilReadMask);
		}
	}

	const BlendStateDesc& currentBlendStateDesc = m_GraphicsPipelineStateDesc.blendState;
	const BlendStateDesc& nextBlendStateDesc = pipelineStateDesc.blendState;
	if (force || currentBlendStateDesc.enabled != nextBlendStateDesc.enabled)
	{
		if (nextBlendStateDesc.enabled)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);
	}
	if (force ||
		currentBlendStateDesc.srcColorBlendFactor != nextBlendStateDesc.srcColorBlendFactor ||
		currentBlendStateDesc.srcAlphaBlendFactor != nextBlendStateDesc.srcAlphaBlendFactor ||
		currentBlendStateDesc.dstColorBlendFactor != nextBlendStateDesc.dstColorBlendFactor ||
		currentBlendStateDesc.dstAlphaBlendFactor != nextBlendStateDesc.dstAlphaBlendFactor)
	{
		if (nextBlendStateDesc.srcColorBlendFactor == nextBlendStateDesc.srcAlphaBlendFactor &&
			nextBlendStateDesc.dstColorBlendFactor == nextBlendStateDesc.dstAlphaBlendFactor)
		{
			glBlendFunc(
				Mapping::FromBlendFactor(nextBlendStateDesc.srcColorBlendFactor),
				Mapping::FromBlendFactor(nextBlendStateDesc.dstColorBlendFactor));
		}
		else
		{
			glBlendFuncSeparate(
				Mapping::FromBlendFactor(nextBlendStateDesc.srcColorBlendFactor),
				Mapping::FromBlendFactor(nextBlendStateDesc.dstColorBlendFactor),
				Mapping::FromBlendFactor(nextBlendStateDesc.srcAlphaBlendFactor),
				Mapping::FromBlendFactor(nextBlendStateDesc.dstAlphaBlendFactor));
		}
	}

	if (force ||
		currentBlendStateDesc.colorBlendOp != nextBlendStateDesc.colorBlendOp ||
		currentBlendStateDesc.alphaBlendOp != nextBlendStateDesc.alphaBlendOp)
	{
		if (nextBlendStateDesc.colorBlendOp == nextBlendStateDesc.alphaBlendOp)
		{
			glBlendEquation(Mapping::FromBlendOp(nextBlendStateDesc.colorBlendOp));
		}
		else
		{
			glBlendEquationSeparate(
				Mapping::FromBlendOp(nextBlendStateDesc.colorBlendOp),
				Mapping::FromBlendOp(nextBlendStateDesc.alphaBlendOp));
		}
	}

	if (force ||
		currentBlendStateDesc.constant != nextBlendStateDesc.constant)
	{
		glBlendColor(
			nextBlendStateDesc.constant.r,
			nextBlendStateDesc.constant.g,
			nextBlendStateDesc.constant.b,
			nextBlendStateDesc.constant.a);
	}

	if (force ||
		currentBlendStateDesc.colorWriteMask != nextBlendStateDesc.colorWriteMask)
	{
		ApplyColorMask(nextBlendStateDesc.colorWriteMask);
	}

	const RasterizationStateDesc& currentRasterizationStateDesc = m_GraphicsPipelineStateDesc.rasterizationState;
	const RasterizationStateDesc& nextRasterizationStateDesc = pipelineStateDesc.rasterizationState;
	if (force ||
		currentRasterizationStateDesc.cullMode != nextRasterizationStateDesc.cullMode)
	{
		if (nextRasterizationStateDesc.cullMode == CullMode::NONE)
		{
			glDisable(GL_CULL_FACE);
		}
		else
		{
			if (force || currentRasterizationStateDesc.cullMode == CullMode::NONE)
				glEnable(GL_CULL_FACE);
			glCullFace(nextRasterizationStateDesc.cullMode == CullMode::FRONT ? GL_FRONT : GL_BACK);
		}
	}

	if (force ||
		currentRasterizationStateDesc.frontFace != nextRasterizationStateDesc.frontFace)
	{
		if (nextRasterizationStateDesc.frontFace == FrontFace::CLOCKWISE)
			glFrontFace(GL_CW);
		else
			glFrontFace(GL_CCW);
	}

	m_GraphicsPipelineStateDesc = pipelineStateDesc;
}

void CDeviceCommandContext::BlitFramebuffer(
	CFramebuffer* destinationFramebuffer, CFramebuffer* sourceFramebuffer)
{
#if CONFIG2_GLES
	UNUSED2(destinationFramebuffer);
	UNUSED2(sourceFramebuffer);
	debug_warn("CDeviceCommandContext::BlitFramebuffer is not implemented for GLES");
#else
	// Source framebuffer should not be backbuffer.
	ENSURE( sourceFramebuffer->GetHandle() != 0);
	ENSURE( destinationFramebuffer != sourceFramebuffer );
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, sourceFramebuffer->GetHandle());
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, destinationFramebuffer->GetHandle());
	// TODO: add more check for internal formats. And currently we don't support
	// scaling inside blit.
	glBlitFramebufferEXT(
		0, 0, sourceFramebuffer->GetWidth(), sourceFramebuffer->GetHeight(),
		0, 0, sourceFramebuffer->GetWidth(), sourceFramebuffer->GetHeight(),
		(sourceFramebuffer->GetAttachmentMask() & destinationFramebuffer->GetAttachmentMask()),
		GL_NEAREST);
#endif
}

void CDeviceCommandContext::ClearFramebuffer()
{
	ClearFramebuffer(true, true, true);
}

void CDeviceCommandContext::ClearFramebuffer(const bool color, const bool depth, const bool stencil)
{
	const bool needsColor = color && (m_Framebuffer->GetAttachmentMask() & GL_COLOR_BUFFER_BIT) != 0;
	const bool needsDepth = depth && (m_Framebuffer->GetAttachmentMask() & GL_DEPTH_BUFFER_BIT) != 0;
	const bool needsStencil = stencil && (m_Framebuffer->GetAttachmentMask() & GL_STENCIL_BUFFER_BIT) != 0;
	GLbitfield mask = 0;
	if (needsColor)
	{
		ApplyColorMask(ColorWriteMask::RED | ColorWriteMask::GREEN | ColorWriteMask::BLUE | ColorWriteMask::ALPHA);
		glClearColor(
			m_Framebuffer->GetClearColor().r,
			m_Framebuffer->GetClearColor().g,
			m_Framebuffer->GetClearColor().b,
			m_Framebuffer->GetClearColor().a);
		mask |= GL_COLOR_BUFFER_BIT;
	}
	if (needsDepth)
	{
		ApplyDepthMask(true);
		mask |= GL_DEPTH_BUFFER_BIT;
	}
	if (needsStencil)
	{
		ApplyStencilMask(std::numeric_limits<uint32_t>::max());
		mask |= GL_STENCIL_BUFFER_BIT;
	}
	glClear(mask);
	if (needsColor)
		ApplyColorMask(m_GraphicsPipelineStateDesc.blendState.colorWriteMask);
	if (needsDepth)
		ApplyDepthMask(m_GraphicsPipelineStateDesc.depthStencilState.depthWriteEnabled);
	if (needsStencil)
		ApplyStencilMask(m_GraphicsPipelineStateDesc.depthStencilState.stencilWriteMask);
}

void CDeviceCommandContext::SetFramebuffer(CFramebuffer* framebuffer)
{
	ENSURE(framebuffer);
	ENSURE(framebuffer->GetHandle() == 0 || (framebuffer->GetWidth() > 0 && framebuffer->GetHeight() > 0));
	m_Framebuffer = framebuffer;
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer->GetHandle());
}

void CDeviceCommandContext::SetScissors(const uint32_t scissorCount, const ScissorRect* scissors)
{
	ENSURE(scissorCount <= 1);
	if (scissorCount == 0)
	{
		if (m_ScissorCount != scissorCount)
			glDisable(GL_SCISSOR_TEST);
	}
	else
	{
		if (m_ScissorCount != scissorCount)
			glEnable(GL_SCISSOR_TEST);
		ENSURE(scissors);
		if (m_ScissorCount != scissorCount || m_Scissors[0] != scissors[0])
		{
			m_Scissors[0] = scissors[0];
			glScissor(m_Scissors[0].x, m_Scissors[0].y, m_Scissors[0].width, m_Scissors[0].height);
		}
	}
	m_ScissorCount = scissorCount;
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
