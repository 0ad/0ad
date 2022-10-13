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

#ifndef INCLUDED_RENDERER_BACKEND_GL_TEXTURE
#define INCLUDED_RENDERER_BACKEND_GL_TEXTURE

#include "lib/ogl.h"
#include "renderer/backend/ITexture.h"
#include "renderer/backend/Sampler.h"

#include <cstdint>
#include <memory>

namespace Renderer
{

namespace Backend
{

namespace GL
{

class CDevice;

/**
 * Represents a low-level GL texture, encapsulates all properties initialization.
 */
class CTexture final : public ITexture
{
public:
	~CTexture() override;

	IDevice* GetDevice() override;

	Type GetType() const override { return m_Type; }
	uint32_t GetUsage() const override { return m_Usage; }
	Format GetFormat() const override { return m_Format; }

	uint32_t GetWidth() const override { return m_Width; }
	uint32_t GetHeight() const override { return m_Height; }
	uint32_t GetMIPLevelCount() const override { return m_MIPLevelCount; }

	GLuint GetHandle() const { return m_Handle; }

private:
	friend class CDevice;

	CTexture();

	CDevice* m_Device = nullptr;

	// GL before 3.3 doesn't support sampler objects, so each texture should have
	// an own default sampler.
	static std::unique_ptr<CTexture> Create(
		CDevice* device, const char* name, const Type type, const uint32_t usage,
		const Format format, const uint32_t width, const uint32_t height,
		const Sampler::Desc& defaultSamplerDesc, const uint32_t MIPLevelCount, const uint32_t sampleCount);

	GLuint m_Handle = 0;

	Type m_Type = Type::TEXTURE_2D;
	uint32_t m_Usage = 0;
	Format m_Format = Format::UNDEFINED;
	uint32_t m_Width = 0;
	uint32_t m_Height = 0;
	uint32_t m_MIPLevelCount = 0;
};

} // namespace GL

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_GL_TEXTURE
