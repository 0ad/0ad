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

/*
IGUIButtonBehavior
*/

#include "precompiled.h"
#include "GUI.h"


//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
IGUIButtonBehavior::IGUIButtonBehavior() : m_Pressed(false)
{
}

IGUIButtonBehavior::~IGUIButtonBehavior()
{
}

void IGUIButtonBehavior::HandleMessage(SGUIMessage &Message)
{
	// TODO Gee: easier access functions
	switch (Message.type)
	{
	case GUIM_MOUSE_PRESS_LEFT:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
	
		if (!enabled)
			break;

		m_Pressed = true;
	}
	break;

	case GUIM_MOUSE_DBLCLICK_RIGHT:
	case GUIM_MOUSE_RELEASE_RIGHT:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
		
		if (!enabled)
			break;
		
		if (m_PressedRight)
		{
			m_PressedRight = false;
			if (Message.type == GUIM_MOUSE_RELEASE_RIGHT)
			{
				// Button was right-clicked
				SendEvent(GUIM_PRESSED_MOUSE_RIGHT, "pressright");
			}
			else
			{
				// Button was clicked a second time. We can't tell if the button
				// expects to receive doublepress events or just a second press
				// event, so send both of them (and assume the extra unwanted press
				// is harmless on buttons that expect doublepress)
				SendEvent(GUIM_PRESSED_MOUSE_RIGHT, "pressright");
				SendEvent(GUIM_DOUBLE_PRESSED_MOUSE_RIGHT, "doublepressright");
			}
		}
	}
	break;

	case GUIM_MOUSE_PRESS_RIGHT:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
		
		if (!enabled)
			break;
		
		m_PressedRight = true;
	}
	break;

	case GUIM_MOUSE_DBLCLICK_LEFT:
	case GUIM_MOUSE_RELEASE_LEFT:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
	
		if (!enabled)
			break;

		if (m_Pressed)
		{
			m_Pressed = false;
			if (Message.type == GUIM_MOUSE_RELEASE_LEFT)
			{
				// Button was clicked
				SendEvent(GUIM_PRESSED, "press");
			}
			else
			{
				// Button was clicked a second time. We can't tell if the button
				// expects to receive doublepress events or just a second press
				// event, so send both of them (and assume the extra unwanted press
				// is harmless on buttons that expect doublepress)
				SendEvent(GUIM_PRESSED, "press");
				SendEvent(GUIM_DOUBLE_PRESSED, "doublepress");
			}
		}
	}
	break;

	default:
		break;
	}
}

CColor IGUIButtonBehavior::ChooseColor()
{
	CColor color, color2;

	// Yes, the object must possess these settings. They are standard
	GUI<CColor>::GetSetting(this, "textcolor", color);

	bool enabled;
	GUI<bool>::GetSetting(this, "enabled", enabled);

	if (!enabled)
	{
		GUI<CColor>::GetSetting(this, "textcolor_disabled", color2);
		return GUI<>::FallBackColor(color2, color);
	}
	else
	if (m_MouseHovering)
	{
		if (m_Pressed)
		{
			GUI<CColor>::GetSetting(this, "textcolor_pressed", color2);
			return GUI<>::FallBackColor(color2, color);
		}
		else
		{
			GUI<CColor>::GetSetting(this, "textcolor_over", color2);
			return GUI<>::FallBackColor(color2, color);
		}
	}
	else return color;
}

void IGUIButtonBehavior::DrawButton(const CRect &rect,
									const float &z,
									CGUISpriteInstance& sprite,
									CGUISpriteInstance& sprite_over,
									CGUISpriteInstance& sprite_pressed,
									CGUISpriteInstance& sprite_disabled,
									int cell_id)
{
	if (GetGUI())
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);

		if (!enabled)
		{
			GetGUI()->DrawSprite(GUI<>::FallBackSprite(sprite_disabled, sprite), cell_id, z, rect);
		}
		else
		if (m_MouseHovering)
		{
			if (m_Pressed)
				GetGUI()->DrawSprite(GUI<>::FallBackSprite(sprite_pressed, sprite), cell_id, z, rect);
			else
				GetGUI()->DrawSprite(GUI<>::FallBackSprite(sprite_over, sprite), cell_id, z, rect);
		}
		else GetGUI()->DrawSprite(sprite, cell_id, z, rect);
	}
}
