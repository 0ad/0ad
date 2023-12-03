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

#ifndef INCLUDED_RENDERER_BACKEND_DUMMY_BUFFER
#define INCLUDED_RENDERER_BACKEND_DUMMY_BUFFER

#include "renderer/backend/IBuffer.h"

#include <memory>

namespace Renderer
{

namespace Backend
{

namespace Dummy
{

class CDevice;

class CBuffer : public IBuffer
{
public:
	~CBuffer() override;

	IDevice* GetDevice() override;

	Type GetType() const override { return m_Type; }
	uint32_t GetSize() const override { return m_Size; }
	bool IsDynamic() const override { return m_Dynamic; }

private:
	friend class CDevice;

	static std::unique_ptr<IBuffer> Create(
		CDevice* device, const Type type, const uint32_t size, const bool dynamic);

	CBuffer();

	CDevice* m_Device = nullptr;

	Type m_Type = Type::VERTEX;
	uint32_t m_Size = 0;
	bool m_Dynamic = false;
};

} // namespace Dummy

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_DUMMY_BUFFER
