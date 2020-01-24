/* Copyright (C) 2020 Wildfire Games.
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

#include "gui/CGUISprite.h"

const CStr IGUIButtonBehavior::EventNamePress = "Press";
const CStr IGUIButtonBehavior::EventNamePressRight = "PressRight";
const CStr IGUIButtonBehavior::EventNameDoublePress = "DoublePress";
const CStr IGUIButtonBehavior::EventNameDoublePressRight = "DoublePressRight";
const CStr IGUIButtonBehavior::EventNameRelease = "Release";
const CStr IGUIButtonBehavior::EventNameReleaseRight = "ReleaseRight";

IGUIButtonBehavior::IGUIButtonBehavior(IGUIObject& pObject)
	: m_pObject(pObject),
	  m_Pressed(),
	  m_PressedRight(),
	  m_SoundDisabled(),
	  m_SoundEnter(),
	  m_SoundLeave(),
	  m_SoundPressed(),
	  m_SoundReleased()
{
	m_pObject.RegisterSetting("sound_disabled", m_SoundDisabled);
	m_pObject.RegisterSetting("sound_enter", m_SoundEnter);
	m_pObject.RegisterSetting("sound_leave", m_SoundLeave);
	m_pObject.RegisterSetting("sound_pressed", m_SoundPressed);
	m_pObject.RegisterSetting("sound_released", m_SoundReleased);
}

IGUIButtonBehavior::~IGUIButtonBehavior()
{
}

void IGUIButtonBehavior::ResetStates()
{
	if (m_Pressed)
	{
		m_Pressed = false;
		m_pObject.PlaySound(m_SoundReleased);
		m_pObject.SendEvent(GUIM_PRESSED_MOUSE_RELEASE, EventNameRelease);
	}

	if (m_PressedRight)
	{
		m_PressedRight = false;
		m_pObject.PlaySound(m_SoundReleased);
		m_pObject.SendEvent(GUIM_PRESSED_MOUSE_RELEASE_RIGHT, EventNameReleaseRight);
	}
}

void IGUIButtonBehavior::HandleMessage(SGUIMessage& Message)
{
	// TODO Gee: easier access functions
	switch (Message.type)
	{
	case GUIM_MOUSE_ENTER:
		if (m_pObject.IsEnabled())
			m_pObject.PlaySound(m_SoundEnter);
		break;

	case GUIM_MOUSE_LEAVE:
		if (m_pObject.IsEnabled())
			m_pObject.PlaySound(m_SoundLeave);
		break;

	case GUIM_MOUSE_DBLCLICK_LEFT:
		if (!m_pObject.IsEnabled())
			break;

		// Since GUIM_MOUSE_PRESS_LEFT also gets called twice in a
		// doubleclick event, we let it handle playing sounds.
		m_pObject.SendEvent(GUIM_DOUBLE_PRESSED, EventNameDoublePress);
		break;

	case GUIM_MOUSE_PRESS_LEFT:
		if (!m_pObject.IsEnabled())
		{
			m_pObject.PlaySound(m_SoundDisabled);
			break;
		}

		m_pObject.PlaySound(m_SoundPressed);
		m_pObject.SendEvent(GUIM_PRESSED, EventNamePress);
		m_Pressed = true;
		break;

	case GUIM_MOUSE_DBLCLICK_RIGHT:
		if (!m_pObject.IsEnabled())
			break;

		// Since GUIM_MOUSE_PRESS_RIGHT also gets called twice in a
		// doubleclick event, we let it handle playing sounds.
		m_pObject.SendEvent(GUIM_DOUBLE_PRESSED_MOUSE_RIGHT, EventNameDoublePressRight);
		break;

	case GUIM_MOUSE_PRESS_RIGHT:
		if (!m_pObject.IsEnabled())
		{
			m_pObject.PlaySound(m_SoundDisabled);
			break;
		}

		// Button was right-clicked
		m_pObject.PlaySound(m_SoundPressed);
		m_pObject.SendEvent(GUIM_PRESSED_MOUSE_RIGHT, EventNamePressRight);
		m_PressedRight = true;
		break;

	case GUIM_MOUSE_RELEASE_RIGHT:
		if (m_pObject.IsEnabled())
			ResetStates();
		break;

	case GUIM_MOUSE_RELEASE_LEFT:
		if (m_pObject.IsEnabled())
			ResetStates();
		break;

	default:
		break;
	}
}

const CGUISpriteInstance& IGUIButtonBehavior::GetButtonSprite(const CGUISpriteInstance& sprite, const CGUISpriteInstance& sprite_over, const CGUISpriteInstance& sprite_pressed, const CGUISpriteInstance& sprite_disabled) const
{
	if (!m_pObject.IsEnabled())
		return sprite_disabled || sprite;

	if (!m_pObject.IsMouseHovering())
		return sprite;

	if (m_Pressed)
		return sprite_pressed || sprite;

	return sprite_over || sprite;
}
