/* Copyright (C) 2015 Wildfire Games.
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

#include "TouchInput.h"

#include <cinttypes>

#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "lib/timer.h"
#include "lib/external_libraries/libsdl.h"
#include "ps/Game.h"

// When emulation is enabled:
// Left-click to put finger 0 down.
// Then left-click-and-drag to move finger 0.
// Then left-click to put finger 0 up.
// Same with right-click for finger 1.
#define EMULATE_FINGERS_WITH_MOUSE 0

extern int g_xres, g_yres;

// NOTE: All this code is currently just a basic prototype for testing;
// it might need significant redesigning for proper usage.

CTouchInput::CTouchInput() :
	m_State(STATE_INACTIVE)
{
	for (size_t i = 0; i < MAX_FINGERS; ++i)
		m_Down[i] = false;

	for (size_t i = 0; i < MAX_MOUSE; ++i)
		m_MouseEmulateState[i] = MOUSE_INACTIVE;
}

CTouchInput::~CTouchInput()
{
}

bool CTouchInput::IsEnabled()
{
#if OS_ANDROID || EMULATE_FINGERS_WITH_MOUSE
	return true;
#else
	return false;
#endif
}

void CTouchInput::OnFingerDown(int id, int x, int y)
{
	debug_printf("finger down %d %d %d; state %d\n", id, x, y, m_State);
	m_Down[id] = true;
	m_Prev[id] = m_Pos[id] = CVector2D(x, y);

	if (m_State == STATE_INACTIVE && id == 0)
	{
		m_State = STATE_FIRST_TOUCH;
		m_FirstTouchTime = timer_Time();
		m_FirstTouchPos = CVector2D(x, y);
	}
	else if ((m_State == STATE_FIRST_TOUCH || m_State == STATE_PANNING) && id == 1)
	{
		m_State = STATE_ZOOMING;
	}
}

void CTouchInput::OnFingerUp(int id, int x, int y)
{
	debug_printf("finger up %d %d %d; state %d\n", id, x, y, m_State);
	m_Down[id] = false;
	m_Pos[id] = CVector2D(x, y);

	if (m_State == STATE_FIRST_TOUCH && id == 0 && timer_Time() < m_FirstTouchTime + 0.5)
	{
		m_State = STATE_INACTIVE;

		SDL_Event_ ev;
		ev.ev.button.button = SDL_BUTTON_LEFT;
		ev.ev.button.x = m_Pos[0].X;
		ev.ev.button.y = m_Pos[0].Y;

		ev.ev.type = SDL_MOUSEBUTTONDOWN;
		ev.ev.button.state = SDL_PRESSED;
		SDL_PushEvent(&ev.ev);

		ev.ev.type = SDL_MOUSEBUTTONUP;
		ev.ev.button.state = SDL_RELEASED;
		SDL_PushEvent(&ev.ev);
	}
	else if (m_State == STATE_ZOOMING && id == 1)
	{
 		m_State = STATE_PANNING;
	}
 	else
	{
		m_State = STATE_INACTIVE;
	}
}

void CTouchInput::OnFingerMotion(int id, int x, int y)
{
	debug_printf("finger motion %d %d %d; state %d\n", id, x, y, m_State);

	CVector2D pos(x, y);

	m_Prev[id] = m_Pos[id];
	m_Pos[id] = pos;

	if (m_State == STATE_FIRST_TOUCH && id == 0)
	{
		if ((pos - m_FirstTouchPos).Length() > 16)
		{
			m_State = STATE_PANNING;

			CCamera& camera = *(g_Game->GetView()->GetCamera());
			m_PanFocus = camera.GetWorldCoordinates(m_FirstTouchPos.X, m_FirstTouchPos.Y, true);
			m_PanDist = (m_PanFocus - camera.GetOrientation().GetTranslation()).Y;
		}
	}

	if (m_State == STATE_PANNING && id == 0)
	{
		CCamera& camera = *(g_Game->GetView()->GetCamera());
		CVector3D origin, dir;
		camera.BuildCameraRay(x, y, origin, dir);
		dir *= m_PanDist / dir.Y;
		camera.GetOrientation().Translate(m_PanFocus - dir - origin);
		camera.UpdateFrustum();
	}

	if (m_State == STATE_ZOOMING && id == 1)
	{
		float oldDist = (m_Prev[id] - m_Pos[1 - id]).Length();
		float newDist = (m_Pos[id] - m_Pos[1 - id]).Length();
		float zoomDist = (newDist - oldDist) * -0.005f * m_PanDist;

		CCamera& camera = *(g_Game->GetView()->GetCamera());
		CVector3D origin, dir;
		camera.BuildCameraRay(m_Pos[0].X, m_Pos[0].Y, origin, dir);
		dir *= zoomDist;
		camera.GetOrientation().Translate(dir);
		camera.UpdateFrustum();

		m_PanFocus = camera.GetWorldCoordinates(m_Pos[0].X, m_Pos[0].Y, true);
		m_PanDist = (m_PanFocus - camera.GetOrientation().GetTranslation()).Y;
	}
}

void CTouchInput::Frame()
{
	double t = timer_Time();
	if (m_State == STATE_FIRST_TOUCH && t > m_FirstTouchTime + 1.0)
	{
		m_State = STATE_INACTIVE;

		SDL_Event_ ev;
		ev.ev.button.button = SDL_BUTTON_RIGHT;
		ev.ev.button.x = m_Pos[0].X;
		ev.ev.button.y = m_Pos[0].Y;

		ev.ev.type = SDL_MOUSEBUTTONDOWN;
		ev.ev.button.state = SDL_PRESSED;
		SDL_PushEvent(&ev.ev);

		ev.ev.type = SDL_MOUSEBUTTONUP;
		ev.ev.button.state = SDL_RELEASED;
		SDL_PushEvent(&ev.ev);
	}
}

InReaction CTouchInput::HandleEvent(const SDL_Event_* ev)
{
	UNUSED2(ev); // may be unused depending on #ifs

	if (!IsEnabled())
		return IN_PASS;

#if EMULATE_FINGERS_WITH_MOUSE
	switch(ev->ev.type)
	{
	case SDL_MOUSEBUTTONDOWN:
	{
		int button;
		if (ev->ev.button.button == SDL_BUTTON_LEFT)
			button = 0;
		else if (ev->ev.button.button == SDL_BUTTON_RIGHT)
			button = 1;
		else
			return IN_PASS;

		m_MouseEmulateDownPos[button] = CVector2D(ev->ev.button.x, ev->ev.button.y);
		if (m_MouseEmulateState[button] == MOUSE_INACTIVE)
		{
			m_MouseEmulateState[button] = MOUSE_ACTIVATING;
			OnFingerDown(button, ev->ev.button.x, ev->ev.button.y);
		}
		else if (m_MouseEmulateState[button] == MOUSE_ACTIVE_UP)
		{
			m_MouseEmulateState[button] = MOUSE_ACTIVE_DOWN;
		}
		return IN_HANDLED;
	}

	case SDL_MOUSEBUTTONUP:
	{
		int button;
		if (ev->ev.button.button == SDL_BUTTON_LEFT)
			button = 0;
		else if (ev->ev.button.button == SDL_BUTTON_RIGHT)
			button = 1;
		else
			return IN_PASS;

		if (m_MouseEmulateState[button] == MOUSE_ACTIVATING)
		{
			m_MouseEmulateState[button] = MOUSE_ACTIVE_UP;
		}
		else if (m_MouseEmulateState[button] == MOUSE_ACTIVE_DOWN)
		{
			float dist = (m_MouseEmulateDownPos[button] - CVector2D(ev->ev.button.x, ev->ev.button.y)).Length();
			if (dist <= 2)
			{
				m_MouseEmulateState[button] = MOUSE_INACTIVE;
				OnFingerUp(button, ev->ev.button.x, ev->ev.button.y);
			}
			else
			{
				m_MouseEmulateState[button] = MOUSE_ACTIVE_UP;
			}
		}
		return IN_HANDLED;
	}

	case SDL_MOUSEMOTION:
	{
		for (size_t i = 0; i < MAX_MOUSE; ++i)
		{
			if (m_MouseEmulateState[i] == MOUSE_ACTIVE_DOWN)
			{
				OnFingerMotion(i, ev->ev.motion.x, ev->ev.motion.y);
			}
		}
		return IN_HANDLED;
	}
	}
#endif

	switch(ev->ev.type)
	{
	case SDL_FINGERDOWN:
	case SDL_FINGERUP:
	case SDL_FINGERMOTION:
	{
		// Map finger events onto the mouse, for basic testing
		debug_printf("finger %s tid=%" PRId64 " fid=%" PRId64 " x=%f y=%f dx=%f dy=%f p=%f\n",
			ev->ev.type == SDL_FINGERDOWN ? "down" :
			ev->ev.type == SDL_FINGERUP ? "up" :
			ev->ev.type == SDL_FINGERMOTION ? "motion" : "?",
			ev->ev.tfinger.touchId, ev->ev.tfinger.fingerId,
			ev->ev.tfinger.x, ev->ev.tfinger.y, ev->ev.tfinger.dx, ev->ev.tfinger.dy, ev->ev.tfinger.pressure);

		if (ev->ev.type == SDL_FINGERDOWN)
			OnFingerDown(ev->ev.tfinger.fingerId, g_xres * ev->ev.tfinger.x, g_yres * ev->ev.tfinger.y);
		else if (ev->ev.type == SDL_FINGERUP)
			OnFingerUp(ev->ev.tfinger.fingerId, g_xres * ev->ev.tfinger.x, g_yres * ev->ev.tfinger.y);
		else if (ev->ev.type == SDL_FINGERMOTION)
			OnFingerMotion(ev->ev.tfinger.fingerId, g_xres * ev->ev.tfinger.x, g_yres * ev->ev.tfinger.y);
		return IN_HANDLED;
	}
	}

	return IN_PASS;
}

CTouchInput g_TouchInput;

InReaction touch_input_handler(const SDL_Event_* ev)
{
	return g_TouchInput.HandleEvent(ev);
}
