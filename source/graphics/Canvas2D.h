/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_CANVAS2D
#define INCLUDED_CANVAS2D

#include "maths/Vector2D.h"

#include <vector>

class CRect;

struct CColor;

// Encapsulates 2D drawing functionality to hide and optimize
// low level API calls.
class CCanvas2D
{
public:
	void DrawLine(const std::vector<CVector2D>& points, const float width, const CColor& color);

	void DrawRect(const CRect& rect, const CColor& color);
};

#endif // INCLUDED_CANVAS2D
