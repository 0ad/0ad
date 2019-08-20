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

#include "GUI.h"

#include "ps/CLogger.h"
#include "soundmanager/ISoundManager.h"

IGUIButtonBehavior::IGUIButtonBehavior(CGUI* pGUI)
	: IGUIObject(pGUI), m_Pressed(false)
{
}

IGUIButtonBehavior::~IGUIButtonBehavior()
{
}

void IGUIButtonBehavior::HandleMessage(SGUIMessage& Message)
{
	bool enabled;
	GUI<bool>::GetSetting(this, "enabled", enabled);
	CStrW soundPath;
	// TODO Gee: easier access functions
	switch (Message.type)
	{
	case GUIM_MOUSE_ENTER:
		if (!enabled)
			break;

		if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_enter", soundPath) == PSRETURN_OK && !soundPath.empty())
			g_SoundManager->PlayAsUI(soundPath.c_str(), false);
		break;

	case GUIM_MOUSE_LEAVE:
		if (!enabled)
			break;

		if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_leave", soundPath) == PSRETURN_OK && !soundPath.empty())
			g_SoundManager->PlayAsUI(soundPath.c_str(), false);
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
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_disabled", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);
			break;
		}

		// Button was clicked
		if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_pressed", soundPath) == PSRETURN_OK && !soundPath.empty())
			g_SoundManager->PlayAsUI(soundPath.c_str(), false);
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
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_disabled", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);
			break;
		}

		// Button was right-clicked
		if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_pressed", soundPath) == PSRETURN_OK && !soundPath.empty())
			g_SoundManager->PlayAsUI(soundPath.c_str(), false);
		SendEvent(GUIM_PRESSED_MOUSE_RIGHT, "pressright");
		m_PressedRight = true;
		break;

	case GUIM_MOUSE_RELEASE_RIGHT:
		if (!enabled)
			break;

		if (m_PressedRight)
		{
			m_PressedRight = false;
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_released", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);
		}
		break;

	case GUIM_MOUSE_RELEASE_LEFT:
		if (!enabled)
			break;

		if (m_Pressed)
		{
			m_Pressed = false;
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_released", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);
		}
		break;

	default:
		break;
	}
}

const CGUIColor& IGUIButtonBehavior::ChooseColor()
{
	// Yes, the object must possess these settings. They are standard
	const CGUIColor& color = GUI<CGUIColor>::GetSetting(this, "textcolor");

	bool enabled;
	GUI<bool>::GetSetting(this, "enabled", enabled);

	if (!enabled)
		return GUI<CGUIColor>::GetSetting(this, "textcolor_disabled") || color;

	if (m_MouseHovering)
	{
		if (m_Pressed)
			return GUI<CGUIColor>::GetSetting(this, "textcolor_pressed") || color;
		else
			return GUI<CGUIColor>::GetSetting(this, "textcolor_over") || color;
	}

	return color;
}

void IGUIButtonBehavior::DrawButton(const CRect& rect, const float& z, CGUISpriteInstance& sprite, CGUISpriteInstance& sprite_over, CGUISpriteInstance& sprite_pressed, CGUISpriteInstance& sprite_disabled, int cell_id)
{
	bool enabled;
	GUI<bool>::GetSetting(this, "enabled", enabled);

	if (!enabled)
		m_pGUI->DrawSprite(sprite_disabled || sprite, cell_id, z, rect);
	else if (m_MouseHovering)
	{
		if (m_Pressed)
			m_pGUI->DrawSprite(sprite_pressed || sprite, cell_id, z, rect);
		else
			m_pGUI->DrawSprite(sprite_over || sprite, cell_id, z, rect);
	}
	else
		m_pGUI->DrawSprite(sprite, cell_id, z, rect);
}
