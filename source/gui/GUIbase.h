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

#ifndef INCLUDED_GUIBASE
#define INCLUDED_GUIBASE

#include "ps/CStr.h"
#include "ps/Errors.h"
#include "ps/Shapes.h"
#include "scriptinterface/ScriptInterface.h"

/**
 * Client Area is a rectangle relative to a parent rectangle
 *
 * You can input the whole value of the Client Area by
 * string. Like used in the GUI.
 */
class CClientArea
{
public:
	// COPYABLE, since there are only primitives involved, making move and copy identical,
	// and since some temporaries cannot be avoided.
	CClientArea();
	CClientArea(const CStr& Value);
	CClientArea(const CRect& pixel, const CRect& percent);

	/// Pixel modifiers
	CRect pixel;

	/// Percent modifiers
	CRect percent;

	/**
	 * Get client area rectangle when the parent is given
	 */
	CRect GetClientArea(const CRect& parent) const;

	/**
	 * The ClientArea can be set from a string looking like:
	 *
	 * "0 0 100% 100%"
	 * "50%-10 50%-10 50%+10 50%+10"
	 *
	 * i.e. First percent modifier, then + or - and the pixel modifier.
	 * Although you can use just the percent or the pixel modifier. Notice
	 * though that the percent modifier must always be the first when
	 * both modifiers are inputted.
	 *
	 * @return true if success, false if failure. If false then the client area
	 *			will be unchanged.
	 */
	bool SetClientArea(const CStr& Value);

	bool operator==(const CClientArea& other) const
	{
		return pixel == other.pixel && percent == other.percent;
	}

	void ToJSVal(JSContext* cx, JS::MutableHandleValue ret) const;
	bool FromJSVal(JSContext* cx, JS::HandleValue v);
};


ERROR_GROUP(GUI);

ERROR_TYPE(GUI, InvalidSetting);
ERROR_TYPE(GUI, OperationNeedsGUIObject);
ERROR_TYPE(GUI, NameAmbiguity);
ERROR_TYPE(GUI, ObjectNeedsName);

#endif // INCLUDED_GUIBASE
