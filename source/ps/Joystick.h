/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_JOYSTICK
#define INCLUDED_JOYSTICK

#include "lib/external_libraries/sdl.h"

class CJoystick
{
public:
	CJoystick();

	/**
	 * Initialises joystick support.
	 */
	void Initialise();

	/**
	 * Returns true if initialised and the joystick is present and enabled by configuration.
	 */
	bool IsEnabled();

	/**
	 * Returns current value of the given joystick axis, in the range [-1, +1].
	 */
	float GetAxisValue(int axis);

private:
	SDL_Joystick* m_Joystick;
	int m_Deadzone;
};

extern CJoystick g_Joystick;

#endif // INCLUDED_JOYSTICK
