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

#ifndef INCLUDED_CPROGRESSBAR
#define INCLUDED_CPROGRESSBAR

#include "GUI.h"

/**
 * Object used to draw a value (e.g. progress) from 0 to 100 visually.
 *
 * @see IGUIObject
 */
class CProgressBar : public IGUIObject
{
	GUI_OBJECT(CProgressBar)

public:
	CProgressBar(CGUI* pGUI);
	virtual ~CProgressBar();

protected:
	/**
	 * Draws the progress bar
	 */
	virtual void Draw();

	// If caption is set, make sure it's within the interval 0-100
	/**
	 * @see IGUIObject#HandleMessage()
	 */
	void HandleMessage(SGUIMessage& Message);
};

#endif // INCLUDED_CPROGRESSBAR
