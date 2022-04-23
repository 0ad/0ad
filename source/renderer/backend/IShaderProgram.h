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

#ifndef INCLUDED_RENDERER_BACKEND_ISHADERPROGRAM
#define INCLUDED_RENDERER_BACKEND_ISHADERPROGRAM

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

/**
 * IShaderProgram is a container for multiple shaders of different types.
 */
class IShaderProgram
{
public:
	virtual ~IShaderProgram() {}
};

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_ISHADERPROGRAM
