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

#include "renderer/backend/dummy/Device.h"

namespace Renderer
{

namespace Backend
{

namespace Dummy
{

// static
std::unique_ptr<ITexture> CTexture::Create(
	CDevice* device, const Type type, const uint32_t usage, const Format format,
	const uint32_t width, const uint32_t height,
	const uint32_t MIPLevelCount)
{
	std::unique_ptr<CTexture> texture(new CTexture());

	texture->m_Device = device;
	texture->m_Type = type;
	texture->m_Usage = usage;
	texture->m_Format = format;
	texture->m_Width = width;
	texture->m_Height = height;
	texture->m_MIPLevelCount = MIPLevelCount;

	return texture;
}

CTexture::CTexture() = default;

CTexture::~CTexture() = default;

IDevice* CTexture::GetDevice()
{
	return m_Device;
}

} // namespace Dummy

} // namespace Backend

} // namespace Renderer
