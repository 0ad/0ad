/* Copyright (C) 2019 Wildfire Games.
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

#include "CGUIColor.h"

#include "gui/CGUI.h"
#include "ps/CStr.h"

bool CGUIColor::ParseString(const CGUI& pGUI, const CStr& value, int defaultAlpha)
{
	if (pGUI.HasPreDefinedColor(value))
	{
		const CGUIColor& color = pGUI.GetPreDefinedColor(value);

		// Explicit copy assignment
		r = color.r;
		g = color.g;
		b = color.b;
		a = color.a;
		return true;
	}

	return CColor::ParseString(value, defaultAlpha);
}
