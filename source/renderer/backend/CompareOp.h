/* Copyright (C) 2022 Wildfire Games.
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

#ifndef INCLUDED_RENDERER_BACKEND_COMPAREOP
#define INCLUDED_RENDERER_BACKEND_COMPAREOP

#include "graphics/Color.h"

class CStr;

namespace Renderer
{

namespace Backend
{

enum class CompareOp
{
	// Never passes the comparison.
	NEVER,
	// Passes if the source value is less than the destination value.
	LESS,
	// Passes if the source depth value is equal to the destination value.
	EQUAL,
	// Passes if the source depth value is less than or equal to the destination value.
	LESS_OR_EQUAL,
	// Passes if the source depth value is greater than the destination value.
	GREATER,
	// Passes if the source depth value is not equal to the destination value.
	NOT_EQUAL,
	// Passes if the source depth value is greater than or equal to the destination value.
	GREATER_OR_EQUAL,
	// Always passes the comparison.
	ALWAYS
};

CompareOp ParseCompareOp(const CStr& str);

} // namespace Backend

} // namespace Renderer

#endif // INCLUDED_RENDERER_BACKEND_COMPAREOP
