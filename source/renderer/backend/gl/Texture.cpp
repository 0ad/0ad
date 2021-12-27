/* Copyright (C) 2021 Wildfire Games.
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

GLint CalculateMinFilter(const Sampler::Desc& defaultSamplerDesc, const uint32_t mipCount)
{
	if (mipCount == 1)
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

} // anonymous namespace

// static
std::unique_ptr<CTexture> CTexture::Create2D(const Format format,
	const uint32_t width, const uint32_t height,
	const Sampler::Desc& defaultSamplerDesc, const uint32_t mipCount)
{
	std::unique_ptr<CTexture> texture(new CTexture());

	ENSURE(format != Format::UNDEFINED);
	ENSURE(width > 0 && height > 0 && mipCount > 0);

	texture->m_Format = format;
	texture->m_Width = width;
	texture->m_Height = height;
	texture->m_MipCount = mipCount;

	glGenTextures(1, &texture->m_Handle);

	glActiveTextureARB(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->m_Handle);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, CalculateMinFilter(defaultSamplerDesc, mipCount));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, defaultSamplerDesc.magFilter == Sampler::Filter::LINEAR ? GL_LINEAR : GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, AddressModeToGLEnum(defaultSamplerDesc.addressModeU));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, AddressModeToGLEnum(defaultSamplerDesc.addressModeV));

#if !CONFIG2_GLES
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipCount - 1);

	if (defaultSamplerDesc.mipLODBias != 0.0f)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, defaultSamplerDesc.mipLODBias);
#endif // !CONFIG2_GLES

	if (defaultSamplerDesc.anisotropyEnabled && ogl_tex_has_anisotropy())
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, defaultSamplerDesc.maxAnisotropy);

	if (defaultSamplerDesc.addressModeU == Sampler::AddressMode::CLAMP_TO_BORDER ||
		defaultSamplerDesc.addressModeV == Sampler::AddressMode::CLAMP_TO_BORDER ||
		defaultSamplerDesc.addressModeW == Sampler::AddressMode::CLAMP_TO_BORDER)
	{
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, defaultSamplerDesc.borderColor.AsFloatArray());
	}

	glBindTexture(GL_TEXTURE_2D, 0);

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
