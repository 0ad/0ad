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

#ifndef INCLUDED_RENDERER_BACKEND_IBUFFER
#define INCLUDED_RENDERER_BACKEND_IBUFFER

#include "renderer/backend/IDeviceObject.h"

#include <cstdint>

namespace Renderer
{

namespace Backend
{

class IBuffer : public IDeviceObject<IBuffer>
{
public:
	enum class Type
	{
		VERTEX,
		INDEX,
		UPLOAD,
		UNIFORM,
	};

	// Using a struct instead of a enum allows using the same syntax while
	// avoiding adding operator overrides and additional checks on casts.
	struct Usage
	{
		static constexpr uint32_t DYNAMIC = 1u << 0u;
		static constexpr uint32_t TRANSFER_SRC = 1u << 1u;
		static constexpr uint32_t TRANSFER_DST = 1u << 2u;
	};

	virtual Type GetType() const = 0;
	virtual uint32_t GetSize() const = 0;
	virtual uint32_t GetUsage() const = 0;

	bool IsDynamic() const { return GetUsage() & IBuffer::Usage::DYNAMIC; }
};

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_IBUFFER
