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

#include "precompiled.h"

#include "IGUIButtonBehavior.h"

#include "gui/CGUI.h"
#include "gui/CGUISprite.h"

IGUIButtonBehavior::IGUIButtonBehavior(CGUI& pGUI)
	: IGUIObject(pGUI),
	  m_Pressed(false),
	  m_PressedRight(false)
{
	AddSetting<CStrW>("sound_disabled");
	AddSetting<CStrW>("sound_enter");
	AddSetting<CStrW>("sound_leave");
	AddSetting<CStrW>("sound_pressed");
	AddSetting<CStrW>("sound_released");
}

IGUIButtonBehavior::~IGUIButtonBehavior()
{
}

void IGUIButtonBehavior::HandleMessage(SGUIMessage& Message)
{
	const bool enabled = GetSetting<bool>("enabled");

	// TODO Gee: easier access functions
	switch (Message.type)
	{
	case GUIM_MOUSE_ENTER:
		if (enabled)
			PlaySound("sound_enter");
		break;

	case GUIM_MOUSE_LEAVE:
		if (enabled)
			PlaySound("sound_leave");
		break;

	case GUIM_MOUSE_DBLCLICK_LEFT:
		if (!enabled)
			break;

		// Since GUIM_MOUSE_PRESS_LEFT also gets called twice in a
		// doubleclick event, we let it handle playing sounds.
		SendEvent(GUIM_DOUBLE_PRESSED, "doublepress");
		break;

	case GUIM_MOUSE_PRESS_LEFT:
		if (!enabled)
		{
			PlaySound("sound_disabled");
			break;
		}

		PlaySound("sound_pressed");
		SendEvent(GUIM_PRESSED, "press");
		m_Pressed = true;
		break;

	case GUIM_MOUSE_DBLCLICK_RIGHT:
		if (!enabled)
			break;

		// Since GUIM_MOUSE_PRESS_RIGHT also gets called twice in a
		// doubleclick event, we let it handle playing sounds.
		SendEvent(GUIM_DOUBLE_PRESSED_MOUSE_RIGHT, "doublepressright");
		break;

	case GUIM_MOUSE_PRESS_RIGHT:
		if (!enabled)
		{
			PlaySound("sound_disabled");
			break;
		}

		// Button was right-clicked
		PlaySound("sound_pressed");
		SendEvent(GUIM_PRESSED_MOUSE_RIGHT, "pressright");
		m_PressedRight = true;
		break;

	case GUIM_MOUSE_RELEASE_RIGHT:
		if (!enabled)
			break;

		if (m_PressedRight)
		{
			m_PressedRight = false;
			PlaySound("sound_released");
		}
		break;

	case GUIM_MOUSE_RELEASE_LEFT:
		if (!enabled)
			break;

		if (m_Pressed)
		{
			m_Pressed = false;
			PlaySound("sound_released");
		}
		break;

	default:
		break;
	}
}

void IGUIButtonBehavior::DrawButton(const CRect& rect, const float& z, CGUISpriteInstance& sprite, CGUISpriteInstance& sprite_over, CGUISpriteInstance& sprite_pressed, CGUISpriteInstance& sprite_disabled, int cell_id)
{
	if (!GetSetting<bool>("enabled"))
		m_pGUI.DrawSprite(sprite_disabled || sprite, cell_id, z, rect);
	else if (m_MouseHovering)
	{
		if (m_Pressed)
			m_pGUI.DrawSprite(sprite_pressed || sprite, cell_id, z, rect);
		else
			m_pGUI.DrawSprite(sprite_over || sprite, cell_id, z, rect);
	}
	else
		m_pGUI.DrawSprite(sprite, cell_id, z, rect);
}
