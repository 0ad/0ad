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

#ifndef INCLUDED_CGUISIZE
#define INCLUDED_CGUISIZE

#include "ps/CStr.h"
#include "ps/Shapes.h"
#include "scriptinterface/ScriptInterface.h"

/**
 * This class represents a rectangle relative to a parent rectangle
 * The value can be initialized from a string or JS object.
 */
class CGUISize
{
public:
	// COPYABLE, since there are only primitives involved, making move and copy identical,
	// and since some temporaries cannot be avoided.
	CGUISize();
	CGUISize(const CRect& pixel, const CRect& percent);

	static CGUISize Full();

	/// Pixel modifiers
	CRect pixel;

	/// Percent modifiers
	CRect percent;

	/**
	 * Get client area rectangle when the parent is given
	 */
	CRect GetSize(const CRect& parent) const;

	/**
	 * The value can be set from a string looking like:
	 *
	 * "0 0 100% 100%"
	 * "50%-10 50%-10 50%+10 50%+10"
	 *
	 * i.e. First percent modifier, then + or - and the pixel modifier.
	 * Although you can use just the percent or the pixel modifier. Notice
	 * though that the percent modifier must always be the first when
	 * both modifiers are inputted.
	 *
	 * @return true if success, otherwise size will remain unchanged.
	 */
	bool FromString(const CStr& Value);

	bool operator==(const CGUISize& other) const
	{
		return pixel == other.pixel && percent == other.percent;
	}

	void ToJSVal(JSContext* cx, JS::MutableHandleValue ret) const;
	bool FromJSVal(JSContext* cx, JS::HandleValue v);
};

#endif // INCLUDED_CGUISIZE
