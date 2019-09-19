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

/*
 * This is the top class of the whole GUI, all objects
 * and settings are stored within this class.
 */

#ifndef INCLUDED_CGUIDUMMYOBJECT
#define INCLUDED_CGUIDUMMYOBJECT

#include "gui/IGUIObject.h"

/**
 * Dummy object are used for the base object and objects of type "empty".
 */
class CGUIDummyObject : public IGUIObject
{
	GUI_OBJECT(CGUIDummyObject)

public:
	CGUIDummyObject(CGUI& pGUI) : IGUIObject(pGUI) {}

	virtual void Draw() {}

	/**
	 * Empty can never be hovered. It is only a category.
	 */
	virtual bool IsMouseOver() const
	{
		return false;
	}
};

#endif // INCLUDED_CGUIDUMMYOBJECT
