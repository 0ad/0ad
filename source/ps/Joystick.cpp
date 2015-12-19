/* Copyright (C) 2014 Wildfire Games.
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

#include "Joystick.h"

#include "lib/external_libraries/libsdl.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"

CJoystick g_Joystick;

CJoystick::CJoystick() :
	m_Joystick(NULL), m_Deadzone(0)
{
}

void CJoystick::Initialise()
{
	bool joystickEnable = false;
	CFG_GET_VAL("joystick.enable", joystickEnable);
	if (!joystickEnable)
		return;

	CFG_GET_VAL("joystick.deadzone", m_Deadzone);

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
	{
		LOGERROR("CJoystick::Initialise failed to initialise joysticks (\"%s\")", SDL_GetError());
		return;
	}

	int numJoysticks = SDL_NumJoysticks();

	LOGMESSAGE("Found %d joystick(s)", numJoysticks);

	for (int i = 0; i < numJoysticks; ++i)
	{
		SDL_Joystick* stick = SDL_JoystickOpen(i);
		if (!stick)
		{
			LOGERROR("CJoystick::Initialise failed to open joystick %d (\"%s\")", i, SDL_GetError());
			continue;
		}
		const char* name = SDL_JoystickName(stick);
		LOGMESSAGE("Joystick %d: %s", i, name);
		SDL_JoystickClose(stick);
	}

	if (numJoysticks)
	{
		SDL_JoystickEventState(SDL_ENABLE);

		// Always pick the first joystick, and assume that's the right one
		m_Joystick = SDL_JoystickOpen(0);
		if (!m_Joystick)
			LOGERROR("CJoystick::Initialise failed to open joystick (\"%s\")", SDL_GetError());
	}
}

bool CJoystick::IsEnabled()
{
	return (m_Joystick != NULL);
}

float CJoystick::GetAxisValue(int axis)
{
	if (!m_Joystick || axis < 0 || axis >= SDL_JoystickNumAxes(m_Joystick))
		return 0.f;

	int16_t val = SDL_JoystickGetAxis(m_Joystick, axis);

	// Normalize values into the range [-1, +1] excluding the deadzone around 0
	float norm;
	if (val > m_Deadzone)
		norm = (val - m_Deadzone) / (float)(32767 - m_Deadzone);
	else if (val < -m_Deadzone)
		norm = (val + m_Deadzone) / (float)(32767 - m_Deadzone);
	else
		norm = 0.f;

	return norm;
}
