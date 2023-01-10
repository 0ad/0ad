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

#ifndef INCLUDED_RENDERER_BACKEND_FORMAT
#define INCLUDED_RENDERER_BACKEND_FORMAT

namespace Renderer
{

namespace Backend
{

enum class Format
{
	UNDEFINED,
	R8_UNORM,
	R8G8_UNORM,
	R8G8_UINT,
	R8G8B8_UNORM,
	R8G8B8A8_UNORM,
	R8G8B8A8_UINT,

	// TODO: we need to drop legacy A8 and L8 formats as soon as we have proper
	// channel swizzling.
	A8_UNORM,
	L8_UNORM,

	R16_UNORM,
	R16_UINT,
	R16_SINT,
	R16G16_UNORM,
	R16G16_UINT,
	R16G16_SINT,

	R32_SFLOAT,
	R32G32_SFLOAT,
	R32G32B32_SFLOAT,
	R32G32B32A32_SFLOAT,

	D16,
	D24,
	D24_S8,
	D32,

	BC1_RGB_UNORM,
	BC1_RGBA_UNORM,
	BC2_UNORM,
	BC3_UNORM
};

inline bool IsDepthFormat(const Format format)
{
	return
		format == Format::D16 ||
		format == Format::D24 ||
		format == Format::D24_S8 ||
		format == Format::D32;
}

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_FORMAT
