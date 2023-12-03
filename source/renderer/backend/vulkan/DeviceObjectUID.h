/* Copyright (C) 2023 Wildfire Games.
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

#ifndef INCLUDED_RENDERER_BACKEND_VULKAN_UID
#define INCLUDED_RENDERER_BACKEND_VULKAN_UID

#include <cstdint>

namespace Renderer
{

namespace Backend
{

namespace Vulkan
{

/**
 * Unique identifier for a device object. It must be unique along all objects
 * during a whole application run. We assume that 32bits should be enough, else
 * we'd have a too big object flow.
 * TODO: maybe it makes sense to add it for all backends. Also it might make
 * sense to add categories/types. Several high bits might be for describing an
 * object type, low bits for indexing.
 */
using DeviceObjectUID = uint32_t;
static constexpr DeviceObjectUID INVALID_DEVICE_OBJECT_UID = 0;

} // namespace Vulkan

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_VULKAN_UID
