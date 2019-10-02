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

#ifndef INCLUDED_GUICOLOR
#define INCLUDED_GUICOLOR

#include "graphics/Color.h"
#include "ps/CStr.h"

class CGUI;

/**
 * Same as the CColor class, but this one can also parse colors predefined in the GUI page (such as "yellow").
 */
struct CGUIColor : CColor
{
	// Take advantage of compiler warnings if unintentionally copying this
	NONCOPYABLE(CGUIColor);

	// Defines move semantics so that the structs using the class can use it.
	MOVABLE(CGUIColor);

	CGUIColor() : CColor() {}

	CGUIColor(float r, float g, float b, float a) : CColor(r, g, b, a) {}

	/**
	 * Returns this color if it has been set, otherwise the given fallback color.
	 */
	const CGUIColor& operator||(const CGUIColor& fallback) const
	{
		if (*this)
			return *this;
		return fallback;
	}

	/**
	 * Load color depending on current GUI page.
	 */
	bool ParseString(const CGUI& pGUI, const CStr& value, int defaultAlpha = 255);

	/**
	 * Ensure that all users check for predefined colors.
	 */
	bool ParseString(const CStr& value, int defaultAlpha = 255) = delete;
};
#endif // INCLUDED_GUICOLOR
