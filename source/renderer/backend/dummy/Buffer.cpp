/* Copyright (C) 2024 Wildfire Games.
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

#include "Buffer.h"

#include "renderer/backend/dummy/Device.h"

namespace Renderer
{

namespace Backend
{

namespace Dummy
{

// static
std::unique_ptr<IBuffer> CBuffer::Create(
	CDevice* device, const Type type, const uint32_t size, const uint32_t usage)
{
	std::unique_ptr<CBuffer> buffer(new CBuffer());
	buffer->m_Device = device;
	buffer->m_Type = type;
	buffer->m_Size = size;
	buffer->m_Usage = usage;
	return buffer;
}

CBuffer::CBuffer() = default;

CBuffer::~CBuffer() = default;

IDevice* CBuffer::GetDevice()
{
	return m_Device;
}

} // namespace Dummy

} // namespace Backend

} // namespace Renderer
