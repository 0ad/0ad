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

#include "ps/CLogger.h"
#include "renderer/backend/gl/Buffer.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/backend/gl/Framebuffer.h"
#include "renderer/backend/gl/Mapping.h"
#include "renderer/backend/gl/Texture.h"

#include <algorithm>
#include <cstring>
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
	const CDeviceCommandContext::Rect& lhs,
	const CDeviceCommandContext::Rect& rhs)
{
	return
		lhs.x == rhs.x && lhs.y == rhs.y &&
		lhs.width == rhs.width && lhs.height == rhs.height;
}

bool operator!=(
	const CDeviceCommandContext::Rect& lhs,
	const CDeviceCommandContext::Rect& rhs)
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

GLenum BufferTypeToGLTarget(const CBuffer::Type type)
{
	GLenum target = GL_ARRAY_BUFFER;
	switch (type)
	{
	case CBuffer::Type::VERTEX:
		target = GL_ARRAY_BUFFER;
		break;
	case CBuffer::Type::INDEX:
		target = GL_ELEMENT_ARRAY_BUFFER;
		break;
	};
	return target;
}

void UploadBufferRegionImpl(
	const GLenum target, const uint32_t dataOffset, const uint32_t dataSize,
	const CDeviceCommandContext::UploadBufferFunction& uploadFunction)
{
	ENSURE(dataOffset < dataSize);
	while (true)
	{
		void* mappedData = glMapBufferARB(target, GL_WRITE_ONLY);
		if (mappedData == nullptr)
		{
			// This shouldn't happen unless we run out of virtual address space
			LOGERROR("glMapBuffer failed");
			break;
		}

		uploadFunction(static_cast<u8*>(mappedData) + dataOffset);

		if (glUnmapBufferARB(target) == GL_TRUE)
			break;

		// Unmap might fail on e.g. resolution switches, so just try again
		// and hope it will eventually succeed
		LOGMESSAGE("glUnmapBuffer failed, trying again...\n");
	}
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
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	for (std::pair<GLenum, GLuint>& unit : m_BoundTextures)
		unit.first = unit.second = 0;
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
		0, 0,
		std::max(1u, texture->GetWidth() >> level),
		std::max(1u, texture->GetHeight() >> level),
		level, layer);
}

void CDeviceCommandContext::UploadTextureRegion(
	CTexture* texture, const Format dataFormat,
	const void* data, const size_t dataSize,
	const uint32_t xOffset, const uint32_t yOffset,
	const uint32_t width, const uint32_t height,
	const uint32_t level, const uint32_t layer)
{
	ENSURE(texture);
	ENSURE(width > 0 && height > 0);
	if (texture->GetType() == CTexture::Type::TEXTURE_2D)
	{
		ENSURE(layer == 0);
		if (texture->GetFormat() == Format::R8G8B8A8_UNORM ||
			texture->GetFormat() == Format::R8G8B8_UNORM ||
			texture->GetFormat() == Format::A8_UNORM)
		{
			ENSURE(texture->GetFormat() == dataFormat);
			size_t bytesPerPixel = 4;
			GLenum pixelFormat = GL_RGBA;
			switch (dataFormat)
			{
			case Format::R8G8B8A8_UNORM:
				break;
			case Format::R8G8B8_UNORM:
				pixelFormat = GL_RGB;
				bytesPerPixel = 3;
				break;
			case Format::A8_UNORM:
				pixelFormat = GL_ALPHA;
				bytesPerPixel = 1;
				break;
			case Format::L8_UNORM:
				pixelFormat = GL_LUMINANCE;
				bytesPerPixel = 1;
				break;
			default:
				debug_warn("Unexpected format.");
				break;
			}
			ENSURE(dataSize == width * height * bytesPerPixel);

			ScopedBind scopedBind(this, GL_TEXTURE_2D, texture->GetHandle());
			glTexSubImage2D(GL_TEXTURE_2D, level,
				xOffset, yOffset, width, height,
				pixelFormat, GL_UNSIGNED_BYTE, data);
			ogl_WarnIfError();
		}
		else if (
			texture->GetFormat() == Format::BC1_RGB_UNORM ||
			texture->GetFormat() == Format::BC1_RGBA_UNORM ||
			texture->GetFormat() == Format::BC2_UNORM ||
			texture->GetFormat() == Format::BC3_UNORM)
		{
			ENSURE(xOffset == 0 && yOffset == 0);
			ENSURE(texture->GetFormat() == dataFormat);
			// TODO: add data size check.

			GLenum internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			switch (texture->GetFormat())
			{
			case Format::BC1_RGBA_UNORM:
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				break;
			case Format::BC2_UNORM:
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;
			case Format::BC3_UNORM:
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
			default:
				break;
			}

			ScopedBind scopedBind(this, GL_TEXTURE_2D, texture->GetHandle());
			glCompressedTexImage2DARB(GL_TEXTURE_2D, level, internalFormat, width, height, 0, dataSize, data);
			ogl_WarnIfError();
		}
		else
			debug_warn("Unsupported format");
	}
	else if (texture->GetType() == CTexture::Type::TEXTURE_CUBE)
	{
		if (texture->GetFormat() == Format::R8G8B8A8_UNORM)
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

			ScopedBind scopedBind(this, GL_TEXTURE_CUBE_MAP, texture->GetHandle());
			glTexImage2D(targets[layer], level, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			ogl_WarnIfError();
		}
		else
			debug_warn("Unsupported format");
	}
	else
		debug_warn("Unsupported type");
}

void CDeviceCommandContext::UploadBuffer(CBuffer* buffer, const void* data, const uint32_t dataSize)
{
	UploadBufferRegion(buffer, data, dataSize, 0);
}

void CDeviceCommandContext::UploadBuffer(
	CBuffer* buffer, const UploadBufferFunction& uploadFunction)
{
	UploadBufferRegion(buffer, 0, buffer->GetSize(), uploadFunction);
}

void CDeviceCommandContext::UploadBufferRegion(
	CBuffer* buffer, const void* data, const uint32_t dataOffset, const uint32_t dataSize)
{
	ENSURE(data);
	ENSURE(dataOffset + dataSize <= buffer->GetSize());
	const GLenum target = BufferTypeToGLTarget(buffer->GetType());
	glBindBufferARB(target, buffer->GetHandle());
	if (buffer->IsDynamic())
	{
		// Tell the driver that it can reallocate the whole VBO
		glBufferDataARB(target, buffer->GetSize(), nullptr, buffer->IsDynamic() ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

		// (In theory, glMapBufferRange with GL_MAP_INVALIDATE_BUFFER_BIT could be used
		// here instead of glBufferData(..., NULL, ...) plus glMapBuffer(), but with
		// current Intel Windows GPU drivers (as of 2015-01) it's much faster if you do
		// the explicit glBufferData.)

		UploadBufferRegion(buffer, dataOffset, dataSize, [data, dataOffset, dataSize](u8* mappedData)
		{
			std::memcpy(mappedData, data, dataSize);
		});
	}
	else
	{
		glBufferSubDataARB(target, dataOffset, dataSize, data);
	}
	glBindBufferARB(target, 0);
}

void CDeviceCommandContext::UploadBufferRegion(
	CBuffer* buffer, const uint32_t dataOffset, const uint32_t dataSize,
	const UploadBufferFunction& uploadFunction)
{
	ENSURE(dataOffset + dataSize <= buffer->GetSize());
	const GLenum target = BufferTypeToGLTarget(buffer->GetType());
	glBindBufferARB(target, buffer->GetHandle());
	ENSURE(buffer->IsDynamic());
	UploadBufferRegionImpl(target, dataOffset, dataSize, uploadFunction);
	glBindBufferARB(target, 0);
}

void CDeviceCommandContext::BeginScopedLabel(const char* name)
{
	if (!m_Device->GetCapabilities().debugScopedLabels)
		return;

	++m_ScopedLabelDepth;
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0x0AD, -1, name);
}

void CDeviceCommandContext::EndScopedLabel()
{
	if (!m_Device->GetCapabilities().debugScopedLabels)
		return;

	ENSURE(m_ScopedLabelDepth > 0);
	--m_ScopedLabelDepth;
	glPopDebugGroup();
}

void CDeviceCommandContext::BindTexture(const uint32_t unit, const GLenum target, const GLuint handle)
{
	ENSURE(unit < m_BoundTextures.size());
#if CONFIG2_GLES
	ENSURE(target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP);
#else
	ENSURE(target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP || target == GL_TEXTURE_2D_MULTISAMPLE);
#endif
	if (m_BoundTextures[unit].first == target && m_BoundTextures[unit].second == handle)
		return;
	if (m_ActiveTextureUnit != unit)
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		m_ActiveTextureUnit = unit;
	}
	if (m_BoundTextures[unit].first != target && m_BoundTextures[unit].first && m_BoundTextures[unit].second)
		glBindTexture(m_BoundTextures[unit].first, 0);
	if (m_BoundTextures[unit].second != handle)
		glBindTexture(target, handle);
	m_BoundTextures[unit] = {target, handle};
}

void CDeviceCommandContext::BindBuffer(const CBuffer::Type type, CBuffer* buffer)
{
	ENSURE(!buffer || buffer->GetType() == type);
	if (type == CBuffer::Type::INDEX)
	{
		if (!buffer)
			m_IndexBuffer = nullptr;
		m_IndexBufferData = nullptr;
	}
	glBindBufferARB(BufferTypeToGLTarget(type), buffer ? buffer->GetHandle() : 0);
}

void CDeviceCommandContext::Flush()
{
	ResetStates();

	m_IndexBuffer = nullptr;
	m_IndexBufferData = nullptr;

	BindTexture(0, GL_TEXTURE_2D, 0);
	BindBuffer(CBuffer::Type::INDEX, nullptr);
	BindBuffer(CBuffer::Type::VERTEX, nullptr);

	ENSURE(m_ScopedLabelDepth == 0);
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

	const RasterizationStateDesc& currentRasterizationStateDesc =
		m_GraphicsPipelineStateDesc.rasterizationState;
	const RasterizationStateDesc& nextRasterizationStateDesc =
		pipelineStateDesc.rasterizationState;
	if (force ||
		currentRasterizationStateDesc.polygonMode != nextRasterizationStateDesc.polygonMode)
	{
#if !CONFIG2_GLES
		glPolygonMode(
			GL_FRONT_AND_BACK,
			nextRasterizationStateDesc.polygonMode == PolygonMode::LINE ? GL_LINE : GL_FILL);
#endif
	}

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

#if !CONFIG2_GLES
	if (force ||
		currentRasterizationStateDesc.depthBiasEnabled != nextRasterizationStateDesc.depthBiasEnabled)
	{
		if (nextRasterizationStateDesc.depthBiasEnabled)
			glEnable(GL_POLYGON_OFFSET_FILL);
		else
			glDisable(GL_POLYGON_OFFSET_FILL);
	}
	if (force ||
		currentRasterizationStateDesc.depthBiasConstantFactor != nextRasterizationStateDesc.depthBiasConstantFactor ||
		currentRasterizationStateDesc.depthBiasSlopeFactor != nextRasterizationStateDesc.depthBiasSlopeFactor)
	{
		glPolygonOffset(
			nextRasterizationStateDesc.depthBiasSlopeFactor,
			nextRasterizationStateDesc.depthBiasConstantFactor);
	}
#endif

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

void CDeviceCommandContext::SetScissors(const uint32_t scissorCount, const Rect* scissors)
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

void CDeviceCommandContext::SetViewports(const uint32_t viewportCount, const Rect* viewports)
{
	ENSURE(viewportCount == 1);
	glViewport(viewports[0].x, viewports[0].y, viewports[0].width, viewports[0].height);
}

void CDeviceCommandContext::SetIndexBuffer(CBuffer* buffer)
{
	ENSURE(buffer->GetType() == CBuffer::Type::INDEX);
	m_IndexBuffer = buffer;
	m_IndexBufferData = nullptr;
	BindBuffer(CBuffer::Type::INDEX, m_IndexBuffer);
}

void CDeviceCommandContext::SetIndexBufferData(const void* data)
{
	if (m_IndexBuffer)
	{
		BindBuffer(CBuffer::Type::INDEX, nullptr);
		m_IndexBuffer = nullptr;
	}
	m_IndexBufferData = data;
}

void CDeviceCommandContext::Draw(
	const uint32_t firstVertex, const uint32_t vertexCount)
{
	// Some drivers apparently don't like count = 0 in glDrawArrays here, so skip
	// all drawing in that case.
	if (vertexCount == 0)
		return;
	glDrawArrays(GL_TRIANGLES, firstVertex, vertexCount);
}

void CDeviceCommandContext::DrawIndexed(
	const uint32_t firstIndex, const uint32_t indexCount, const int32_t vertexOffset)
{
	if (indexCount == 0)
		return;
	ENSURE(m_IndexBuffer || m_IndexBufferData);
	ENSURE(vertexOffset == 0);
	if (m_IndexBuffer)
	{
		ENSURE(sizeof(uint16_t) * (firstIndex + indexCount) <= m_IndexBuffer->GetSize());
	}
	// Don't use glMultiDrawElements here since it doesn't have a significant
	// performance impact and it suffers from various driver bugs (e.g. it breaks
	// in Mesa 7.10 swrast with index VBOs).
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT,
		static_cast<const void*>((static_cast<const uint8_t*>(m_IndexBufferData) + sizeof(uint16_t) * firstIndex)));
}

void CDeviceCommandContext::DrawIndexedInRange(
	const uint32_t firstIndex, const uint32_t indexCount,
	const uint32_t start, const uint32_t end)
{
	if (indexCount == 0)
		return;
	ENSURE(m_IndexBuffer || m_IndexBufferData);
	const void* indices =
		static_cast<const void*>((static_cast<const uint8_t*>(m_IndexBufferData) + sizeof(uint16_t) * firstIndex));
	// Draw with DrawRangeElements where available, since it might be more
	// efficient for slow hardware.
#if CONFIG2_GLES
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, indices);
#else
	glDrawRangeElementsEXT(GL_TRIANGLES, start, end, indexCount, GL_UNSIGNED_SHORT, indices);
#endif
}

CDeviceCommandContext::ScopedBind::ScopedBind(
	CDeviceCommandContext* deviceCommandContext,
	const GLenum target, const GLuint handle)
	: m_DeviceCommandContext(deviceCommandContext),
	m_OldBindUnit(deviceCommandContext->m_BoundTextures[deviceCommandContext->m_ActiveTextureUnit])
{
	m_DeviceCommandContext->BindTexture(
		m_DeviceCommandContext->m_ActiveTextureUnit, target, handle);
}

CDeviceCommandContext::ScopedBind::~ScopedBind()
{
	m_DeviceCommandContext->BindTexture(
		m_DeviceCommandContext->m_ActiveTextureUnit, m_OldBindUnit.first, m_OldBindUnit.second);
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
