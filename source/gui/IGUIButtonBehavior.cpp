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
	  m_Pressed(),
	  m_PressedRight(),
	  m_SoundDisabled(),
	  m_SoundEnter(),
	  m_SoundLeave(),
	  m_SoundPressed(),
	  m_SoundReleased()
{
	RegisterSetting("sound_disabled", m_SoundDisabled);
	RegisterSetting("sound_enter", m_SoundEnter);
	RegisterSetting("sound_leave", m_SoundLeave);
	RegisterSetting("sound_pressed", m_SoundPressed);
	RegisterSetting("sound_released", m_SoundReleased);
}

IGUIButtonBehavior::~IGUIButtonBehavior()
{
}

void IGUIButtonBehavior::HandleMessage(SGUIMessage& Message)
{
	// TODO Gee: easier access functions
	switch (Message.type)
	{
	case GUIM_MOUSE_ENTER:
		if (m_Enabled)
			PlaySound(m_SoundEnter);
		break;

	case GUIM_MOUSE_LEAVE:
		if (m_Enabled)
			PlaySound(m_SoundLeave);
		break;

	case GUIM_MOUSE_DBLCLICK_LEFT:
		if (!m_Enabled)
			break;

		// Since GUIM_MOUSE_PRESS_LEFT also gets called twice in a
		// doubleclick event, we let it handle playing sounds.
		SendEvent(GUIM_DOUBLE_PRESSED, "doublepress");
		break;

	case GUIM_MOUSE_PRESS_LEFT:
		if (!m_Enabled)
		{
			PlaySound(m_SoundDisabled);
			break;
		}

		PlaySound(m_SoundPressed);
		SendEvent(GUIM_PRESSED, "press");
		m_Pressed = true;
		break;

	case GUIM_MOUSE_DBLCLICK_RIGHT:
		if (!m_Enabled)
			break;

		// Since GUIM_MOUSE_PRESS_RIGHT also gets called twice in a
		// doubleclick event, we let it handle playing sounds.
		SendEvent(GUIM_DOUBLE_PRESSED_MOUSE_RIGHT, "doublepressright");
		break;

	case GUIM_MOUSE_PRESS_RIGHT:
		if (!m_Enabled)
		{
			PlaySound(m_SoundDisabled);
			break;
		}

		// Button was right-clicked
		PlaySound(m_SoundPressed);
		SendEvent(GUIM_PRESSED_MOUSE_RIGHT, "pressright");
		m_PressedRight = true;
		break;

	case GUIM_MOUSE_RELEASE_RIGHT:
		if (!m_Enabled)
			break;

		if (m_PressedRight)
		{
			m_PressedRight = false;
			PlaySound(m_SoundReleased);
		}
		break;

	case GUIM_MOUSE_RELEASE_LEFT:
		if (!m_Enabled)
			break;

		if (m_Pressed)
		{
			m_Pressed = false;
			PlaySound(m_SoundReleased);
		}
		break;

	default:
		break;
	}
}

void IGUIButtonBehavior::DrawButton(const CRect& rect, const float& z, const CGUISpriteInstance& sprite, const CGUISpriteInstance& sprite_over, const CGUISpriteInstance& sprite_pressed, const CGUISpriteInstance& sprite_disabled, int cell_id)
{
	if (!m_Enabled)
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
