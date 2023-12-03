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

#include "Texture.h"

#include "lib/code_annotation.h"
#include "lib/config2.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/backend/gl/DeviceCommandContext.h"
#include "renderer/backend/gl/Mapping.h"

#include <algorithm>

namespace Renderer
{

namespace Backend
{

namespace GL
{

namespace
{

GLint CalculateMinFilter(const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount)
{
	if (MIPLevelCount == 1)
		return defaultSamplerDesc.minFilter == Sampler::Filter::LINEAR ? GL_LINEAR : GL_NEAREST;

	if (defaultSamplerDesc.minFilter == Sampler::Filter::LINEAR)
		return defaultSamplerDesc.mipFilter == Sampler::Filter::LINEAR ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST;

	return defaultSamplerDesc.mipFilter == Sampler::Filter::LINEAR ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
}

GLint AddressModeToGLEnum(Sampler::AddressMode addressMode)
{
	switch (addressMode)
	{
	case Sampler::AddressMode::REPEAT: return GL_REPEAT;
	case Sampler::AddressMode::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
	case Sampler::AddressMode::CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
	case Sampler::AddressMode::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
	}
	return GL_REPEAT;
}

GLenum TypeToGLEnum(CTexture::Type type)
{
	GLenum target = GL_TEXTURE_2D;
	switch (type)
	{
	case CTexture::Type::TEXTURE_2D:
		target = GL_TEXTURE_2D;
		break;
	case CTexture::Type::TEXTURE_2D_MULTISAMPLE:
#if CONFIG2_GLES
		ENSURE(false && "Multisample textures are unsupported on GLES");
#else
		target = GL_TEXTURE_2D_MULTISAMPLE;
#endif
		break;
	case CTexture::Type::TEXTURE_CUBE:
		target = GL_TEXTURE_CUBE_MAP;
		break;
	}
	return target;
}

} // anonymous namespace

// static
std::unique_ptr<CTexture> CTexture::Create(
	CDevice* device, const char* name, const Type type, const uint32_t usage,
	const Format format, const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount)
{
	std::unique_ptr<CTexture> texture(new CTexture());

	ENSURE(format != Format::UNDEFINED);
	ENSURE(width > 0 && height > 0 && MIPLevelCount > 0);
	ENSURE((type == Type::TEXTURE_2D_MULTISAMPLE && sampleCount > 1 && !(usage & ITexture::Usage::SAMPLED)) || sampleCount == 1);

	texture->m_Device = device;
	texture->m_Type = type;
	texture->m_Usage = usage;
	texture->m_Format = format;
	texture->m_Width = width;
	texture->m_Height = height;
	texture->m_MIPLevelCount = MIPLevelCount;

	glGenTextures(1, &texture->m_Handle);

	ogl_WarnIfError();

	const GLenum target = TypeToGLEnum(type);

	CDeviceCommandContext::ScopedBind scopedBind(
		texture->m_Device->GetActiveCommandContext(), target, texture->m_Handle);

	// It's forbidden to set sampler state for multisample textures.
	if (type != Type::TEXTURE_2D_MULTISAMPLE)
	{
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, CalculateMinFilter(defaultSamplerDesc, MIPLevelCount));
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, defaultSamplerDesc.magFilter == Sampler::Filter::LINEAR ? GL_LINEAR : GL_NEAREST);

		ogl_WarnIfError();

		glTexParameteri(target, GL_TEXTURE_WRAP_S, AddressModeToGLEnum(defaultSamplerDesc.addressModeU));
		glTexParameteri(target, GL_TEXTURE_WRAP_T, AddressModeToGLEnum(defaultSamplerDesc.addressModeV));
	}

#if !CONFIG2_GLES
	if (type == Type::TEXTURE_CUBE)
		glTexParameteri(target, GL_TEXTURE_WRAP_R, AddressModeToGLEnum(defaultSamplerDesc.addressModeW));
#endif

	ogl_WarnIfError();

#if !CONFIG2_GLES
	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, MIPLevelCount - 1);

	if (defaultSamplerDesc.mipLODBias != 0.0f)
		glTexParameteri(target, GL_TEXTURE_LOD_BIAS, defaultSamplerDesc.mipLODBias);
#endif // !CONFIG2_GLES

	if (type == Type::TEXTURE_2D && defaultSamplerDesc.anisotropyEnabled)
	{
		ENSURE(texture->m_Device->GetCapabilities().anisotropicFiltering);
		const float maxAnisotropy = std::min(
			defaultSamplerDesc.maxAnisotropy, texture->m_Device->GetCapabilities().maxAnisotropy);
		glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	}

	if (defaultSamplerDesc.addressModeU == Sampler::AddressMode::CLAMP_TO_BORDER ||
		defaultSamplerDesc.addressModeV == Sampler::AddressMode::CLAMP_TO_BORDER ||
		defaultSamplerDesc.addressModeW == Sampler::AddressMode::CLAMP_TO_BORDER)
	{
		CColor borderColor(0.0f, 0.0f, 0.0f, 0.0f);
		switch (defaultSamplerDesc.borderColor)
		{
		case Sampler::BorderColor::TRANSPARENT_BLACK:
			break;
		case Sampler::BorderColor::OPAQUE_BLACK:
			borderColor = CColor(0.0f, 0.0f, 0.0f, 1.0f);
			break;
		case Sampler::BorderColor::OPAQUE_WHITE:
			borderColor = CColor(1.0f, 1.0f, 1.0f, 1.0f);
			break;
		}
		glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, borderColor.AsFloatArray().data());
	}

	ogl_WarnIfError();

	if (type == CTexture::Type::TEXTURE_2D)
	{
		bool compressedFormat = false;
		GLint internalFormat = GL_RGBA;
		// Actually pixel data is nullptr so it doesn't make sense to account
		// it, but in theory some buggy drivers might complain about invalid
		// pixel format.
		GLenum pixelFormat = GL_RGBA;
		GLenum pixelType = GL_UNSIGNED_BYTE;
		switch (format)
		{
		case Format::UNDEFINED:
			debug_warn("Texture should defined format");
			break;
		case Format::R8G8B8A8_UNORM:
			break;
		case Format::R8G8B8_UNORM:
			internalFormat = GL_RGB;
			pixelFormat = GL_RGB;
			pixelType = GL_UNSIGNED_BYTE;
			break;
#if !CONFIG2_GLES
		case Format::R8_UNORM:
			internalFormat = GL_RED;
			pixelFormat = GL_RED;
			pixelType = GL_UNSIGNED_BYTE;
			break;
#endif
		case Format::A8_UNORM:
			internalFormat = GL_ALPHA;
			pixelFormat = GL_ALPHA;
			pixelType = GL_UNSIGNED_BYTE;
			break;
		case Format::L8_UNORM:
			internalFormat = GL_LUMINANCE;
			pixelFormat = GL_LUMINANCE;
			pixelType = GL_UNSIGNED_BYTE;
			break;
#if CONFIG2_GLES
		// GLES requires pixel type == UNSIGNED_SHORT or UNSIGNED_INT for depth.
		case Format::D16_UNORM: FALLTHROUGH;
		case Format::D24_UNORM: FALLTHROUGH;
		case Format::D32_SFLOAT:
			internalFormat = GL_DEPTH_COMPONENT;
			pixelFormat = GL_DEPTH_COMPONENT;
			pixelType = GL_UNSIGNED_SHORT;
			break;
		case Format::D24_UNORM_S8_UINT:
			debug_warn("Unsupported format");
			break;
#else
		case Format::D16_UNORM:
			internalFormat = GL_DEPTH_COMPONENT16;
			pixelFormat = GL_DEPTH_COMPONENT;
			pixelType = GL_UNSIGNED_SHORT;
			break;
		case Format::D24_UNORM:
			internalFormat = GL_DEPTH_COMPONENT24;
			pixelFormat = GL_DEPTH_COMPONENT;
			pixelType = GL_UNSIGNED_SHORT;
			break;
		case Format::D32_SFLOAT:
			internalFormat = GL_DEPTH_COMPONENT32;
			pixelFormat = GL_DEPTH_COMPONENT;
			pixelType = GL_UNSIGNED_SHORT;
			break;
		case Format::D24_UNORM_S8_UINT:
			internalFormat = GL_DEPTH24_STENCIL8_EXT;
			pixelFormat = GL_DEPTH_STENCIL_EXT;
			pixelType = GL_UNSIGNED_INT_24_8_EXT;
			break;
#endif
		case Format::BC1_RGB_UNORM: FALLTHROUGH;
		case Format::BC1_RGBA_UNORM: FALLTHROUGH;
		case Format::BC2_UNORM: FALLTHROUGH;
		case Format::BC3_UNORM:
			compressedFormat = true;
			break;
		default:
			debug_warn("Unsupported format.");
		}
		// glCompressedTexImage2D can't accept a null data, so we will initialize it during uploading.
		if (!compressedFormat)
		{
			for (uint32_t level = 0; level < MIPLevelCount; ++level)
			{
				glTexImage2D(target, level, internalFormat,
					std::max(1u, width >> level), std::max(1u, height >> level),
					0, pixelFormat, pixelType, nullptr);
			}
		}
	}
	else if (type == CTexture::Type::TEXTURE_2D_MULTISAMPLE)
	{
		ENSURE(MIPLevelCount == 1);
#if !CONFIG2_GLES
		if (format == Format::R8G8B8A8_UNORM)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_RGBA8, width, height, GL_TRUE);
		}
		else if (format == Format::D24_UNORM_S8_UINT)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_DEPTH24_STENCIL8_EXT, width, height, GL_TRUE);
		}
		else
#endif // !CONFIG2_GLES
		{
			debug_warn("Unsupported format");
		}
	}


#if !CONFIG2_GLES
	if (IsDepthFormat(format))
	{
		if (defaultSamplerDesc.compareEnabled)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(
				GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,
				Mapping::FromCompareOp(defaultSamplerDesc.compareOp));
		}
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}
#endif

	ogl_WarnIfError();

	if (texture->m_Device->GetCapabilities().debugLabels)
	{
		glObjectLabel(GL_TEXTURE, texture->m_Handle, -1, name);
	}

	return texture;
}

CTexture::CTexture() = default;

CTexture::~CTexture()
{
	m_Device->GetActiveCommandContext()->OnTextureDestroy(this);
	if (m_Handle)
		glDeleteTextures(1, &m_Handle);
}

IDevice* CTexture::GetDevice()
{
	return m_Device;
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
