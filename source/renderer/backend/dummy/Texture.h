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

#ifndef INCLUDED_RENDERER_BACKEND_DUMMY_TEXTURE
#define INCLUDED_RENDERER_BACKEND_DUMMY_TEXTURE

#include "renderer/backend/ITexture.h"

#include <memory>

namespace Renderer
{

namespace Backend
{

namespace Dummy
{

class CDevice;

class CTexture : public ITexture
{
public:
	~CTexture() override;

	IDevice* GetDevice() override;

	Type GetType() const override { return m_Type; }
	Format GetFormat() const override { return m_Format; }

	uint32_t GetWidth() const override { return m_Width; }
	uint32_t GetHeight() const override { return m_Height; }
	uint32_t GetMIPLevelCount() const override { return m_MIPLevelCount; }

private:
	friend class CDevice;

	CTexture();

	static std::unique_ptr<ITexture> Create(
		CDevice* device, const Type type, const Format format,
		const uint32_t width, const uint32_t height,
		const uint32_t MIPLevelCount);

	CDevice* m_Device = nullptr;
	Type m_Type = Type::TEXTURE_2D;
	Format m_Format = Format::UNDEFINED;
	uint32_t m_Width = 0;
	uint32_t m_Height = 0;
	uint32_t m_MIPLevelCount = 0;
};

} // namespace Dummy

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_DUMMY_TEXTURE
