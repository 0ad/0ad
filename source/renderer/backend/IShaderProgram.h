/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_RENDERER_BACKEND_ISHADERPROGRAM
#define INCLUDED_RENDERER_BACKEND_ISHADERPROGRAM

#include "lib/file/vfs/vfs_path.h"
#include "ps/CStrIntern.h"
#include "renderer/backend/Format.h"
#include "renderer/backend/IDeviceObject.h"

namespace Renderer
{

namespace Backend
{

enum class VertexAttributeStream : uint32_t
{
	POSITION,
	NORMAL,
	COLOR,
	UV0,
	UV1,
	UV2,
	UV3,
	UV4,
	UV5,
	UV6,
	UV7,
};

enum class VertexAttributeRate : uint32_t
{
	PER_VERTEX,
	PER_INSTANCE
};

struct SVertexAttributeFormat
{
	VertexAttributeStream stream;
	Format format;
	uint32_t offset;
	uint32_t stride;
	VertexAttributeRate rate;
	uint32_t bindingSlot;

	constexpr bool operator==(const SVertexAttributeFormat& other) const noexcept
	{
		return stream == other.stream && format == other.format &&
			offset == other.offset && stride == other.stride &&
			rate == other.rate && bindingSlot == other.bindingSlot;
	}
};

/**
 * IVertexInputLayout stores precompiled list of vertex attributes.
 */
class IVertexInputLayout : public IDeviceObject<IVertexInputLayout>
{
};

/**
 * IShaderProgram is a container for multiple shaders of different types.
 */
class IShaderProgram : public IDeviceObject<IShaderProgram>
{
public:
	virtual int32_t GetBindingSlot(const CStrIntern name) const = 0;

	virtual std::vector<VfsPath> GetFileDependencies() const = 0;
};

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_ISHADERPROGRAM
