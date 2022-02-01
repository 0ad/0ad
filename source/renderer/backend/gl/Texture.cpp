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
#include "lib/res/graphics/ogl_tex.h"
#include "renderer/backend/gl/Device.h"

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
std::unique_ptr<CTexture> CTexture::Create2D(const Format format,
	const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount)
{
	return Create(Type::TEXTURE_2D, format, width, height, defaultSamplerDesc, MIPLevelCount, sampleCount);
}

// static
std::unique_ptr<CTexture> CTexture::Create(const Type type, const Format format,
	const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount)
{
	std::unique_ptr<CTexture> texture(new CTexture());

	ENSURE(format != Format::UNDEFINED);
	ENSURE(width > 0 && height > 0 && MIPLevelCount > 0);
	ENSURE((type == Type::TEXTURE_2D_MULTISAMPLE && sampleCount > 1) || sampleCount == 1);

	texture->m_Format = format;
	texture->m_Type = type;
	texture->m_Width = width;
	texture->m_Height = height;
	texture->m_MIPLevelCount = MIPLevelCount;

	glGenTextures(1, &texture->m_Handle);

	ogl_WarnIfError();

	glActiveTextureARB(GL_TEXTURE0);

	const GLenum target = TypeToGLEnum(type);

	glBindTexture(target, texture->m_Handle);

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

	if (type == Type::TEXTURE_2D && defaultSamplerDesc.anisotropyEnabled && ogl_tex_has_anisotropy())
		glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, defaultSamplerDesc.maxAnisotropy);

	if (defaultSamplerDesc.addressModeU == Sampler::AddressMode::CLAMP_TO_BORDER ||
		defaultSamplerDesc.addressModeV == Sampler::AddressMode::CLAMP_TO_BORDER ||
		defaultSamplerDesc.addressModeW == Sampler::AddressMode::CLAMP_TO_BORDER)
	{
		glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, defaultSamplerDesc.borderColor.AsFloatArray());
	}

	ogl_WarnIfError();

	ENSURE(MIPLevelCount == 1);

	if (type == CTexture::Type::TEXTURE_2D)
	{
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
		case Format::R8G8B8A8:
			break;
		case Format::A8:
			internalFormat = GL_ALPHA;
			pixelFormat = GL_ALPHA;
			pixelType = GL_UNSIGNED_BYTE;
			break;
#if CONFIG2_GLES
		// GLES requires pixel type == UNSIGNED_SHORT or UNSIGNED_INT for depth.
		case Format::D16: FALLTHROUGH;
		case Format::D24: FALLTHROUGH;
		case Format::D32:
			internalFormat = GL_DEPTH_COMPONENT;
			pixelFormat = GL_DEPTH_COMPONENT;
			pixelType = GL_UNSIGNED_SHORT;
			break;
		case Format::D24_S8:
			debug_warn("Unsupported format");
			break;
#else
		case Format::D16:
			internalFormat = GL_DEPTH_COMPONENT16;
			pixelFormat = GL_DEPTH_COMPONENT;
			pixelType = GL_UNSIGNED_SHORT;
			break;
		case Format::D24:
			internalFormat = GL_DEPTH_COMPONENT24;
			pixelFormat = GL_DEPTH_COMPONENT;
			pixelType = GL_UNSIGNED_SHORT;
			break;
		case Format::D32:
			internalFormat = GL_DEPTH_COMPONENT32;
			pixelFormat = GL_DEPTH_COMPONENT;
			pixelType = GL_UNSIGNED_SHORT;
			break;
		case Format::D24_S8:
			internalFormat = GL_DEPTH24_STENCIL8_EXT;
			pixelFormat = GL_DEPTH_STENCIL_EXT;
			pixelType = GL_UNSIGNED_INT_24_8_EXT;
			break;
#endif
		}
		glTexImage2D(target, 0, internalFormat, width, height, 0, pixelFormat, pixelType, nullptr);
	}
	else if (type == CTexture::Type::TEXTURE_2D_MULTISAMPLE)
	{
#if !CONFIG2_GLES
		if (format == Format::R8G8B8A8)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_RGBA8, width, height, GL_TRUE);
		}
		else if (format == Format::D24_S8)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_DEPTH24_STENCIL8_EXT, width, height, GL_TRUE);
		}
		else
#endif // !CONFIG2_GLES
		{
			debug_warn("Unsupported format");
		}
	}

	ogl_WarnIfError();

	glBindTexture(target, 0);

	return texture;
}

CTexture::CTexture() = default;

CTexture::~CTexture()
{
	if (m_Handle)
		glDeleteTextures(1, &m_Handle);
}

} // namespace GL

} // namespace Backend

} // namespace Renderer
