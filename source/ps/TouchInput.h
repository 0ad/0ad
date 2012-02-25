/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_TOUCHINPUT
#define INCLUDED_TOUCHINPUT

#include "lib/input.h"
#include "maths/Vector2D.h"
#include "maths/Vector3D.h"

/**
 * Maps touch events (e.g. on Android touchscreen devices) onto mouse events
 * and camera movement.
 */
class CTouchInput
{
public:
	CTouchInput();
	~CTouchInput();

	/**
	 * Returns whether the touch input mode is enabled for this device.
	 */
	bool IsEnabled();

	InReaction HandleEvent(const SDL_Event_* ev);

	/**
	 * Should be called once per frame to perform updates.
	 */
	void Frame();

private:
	void OnFingerDown(int id, int x, int y);
	void OnFingerUp(int id, int x, int y);
	void OnFingerMotion(int id, int x, int y);

	// Mouse emulation state:
	enum
	{
		MOUSE_INACTIVE,
		MOUSE_ACTIVATING,
		MOUSE_ACTIVE_UP,
		MOUSE_ACTIVE_DOWN
	};
	static const size_t MAX_MOUSE = 2;
	int m_MouseEmulateState[MAX_MOUSE];
	CVector2D m_MouseEmulateDownPos[MAX_MOUSE];

	// Current finger state:
	static const size_t MAX_FINGERS = 2;
	bool m_Down[MAX_FINGERS];
	CVector2D m_Pos[MAX_FINGERS];
	CVector2D m_Prev[MAX_FINGERS];


	enum
	{
		STATE_INACTIVE,
		STATE_FIRST_TOUCH,
		STATE_PANNING,
		STATE_ZOOMING
	};
	int m_State;

	double m_FirstTouchTime;
	CVector2D m_FirstTouchPos;

	CVector3D m_PanFocus;
	float m_PanDist;
};

extern CTouchInput g_TouchInput;

extern InReaction touch_input_handler(const SDL_Event_* ev);

#endif // INCLUDED_TOUCHINPUT
