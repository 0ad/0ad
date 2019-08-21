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

#ifndef INCLUDED_CIMAGE
#define INCLUDED_CIMAGE

#include "GUI.h"

/**
 * Object just for drawing a sprite. Like CText, without the
 * possibility to draw text.
 *
 * Created, because I've seen the user being indecisive about
 * what control to use in these situations. I've seen button
 * without functionality used, and that is a lot of unnecessary
 * overhead. That's why I thought I'd go with an intuitive
 * control.
 *
 * @see IGUIObject
 */
class CImage : public IGUIObject
{
	GUI_OBJECT(CImage)

public:
	CImage(CGUI& pGUI);
	virtual ~CImage();

protected:
	/**
	 * Draws the Image
	 */
	virtual void Draw();
};

#endif // INCLUDED_CIMAGE
