/*
IGUIButtonBehavior
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
IGUIButtonBehavior::IGUIButtonBehavior() : m_Pressed(false)
{
}

IGUIButtonBehavior::~IGUIButtonBehavior()
{
}

void IGUIButtonBehavior::HandleMessage(const SGUIMessage &Message)
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
	}	break;

	case GUIM_MOUSE_RELEASE_LEFT:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
	
		if (!enabled)
			break;

		if (m_Pressed)
		{
			m_Pressed = false;
			// BUTTON WAS CLICKED
			HandleMessage(GUIM_PRESSED);
		}
	}	break;

	case GUIM_SETTINGS_UPDATED:
		// If it's hidden, then it can't be pressed
		//if (GetBaseSettings().m_Hidden)
		//	m_Pressed = false;
		break;

	default:
		break;
	}
}

CColor IGUIButtonBehavior::ChooseColor()
{
	CColor color, color_over, color_pressed, color_disabled;

	// Yes, the object must possess these settings. They are standard
	GUI<CColor>::GetSetting(this, "textcolor", color);
	GUI<CColor>::GetSetting(this, "textcolor-over", color_over);
	GUI<CColor>::GetSetting(this, "textcolor-pressed", color_pressed);
	GUI<CColor>::GetSetting(this, "textcolor-disabled", color_disabled);

	bool enabled;
	GUI<bool>::GetSetting(this, "enabled", enabled);

	if (!enabled)
	{
		return GUI<>::FallBackColor(color_disabled, color);
	}
	else
	if (m_MouseHovering)
	{
		if (m_Pressed)
			return GUI<>::FallBackColor(color_pressed, color);
		else
			return GUI<>::FallBackColor(color_over, color);
	}
	else return color;
}

void IGUIButtonBehavior::DrawButton(const CRect &rect,
									const float &z,
									const CStr &sprite,
									const CStr &sprite_over,
									const CStr &sprite_pressed,
									const CStr &sprite_disabled)
{
	if (GetGUI())
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);

		if (!enabled)
		{
			GetGUI()->DrawSprite(GUI<>::FallBackSprite(sprite_disabled, sprite), z, rect);
		}
		else
		if (m_MouseHovering)
		{
			if (m_Pressed)
				GetGUI()->DrawSprite(GUI<>::FallBackSprite(sprite_pressed, sprite), z, rect);
			else
				GetGUI()->DrawSprite(GUI<>::FallBackSprite(sprite_over, sprite), z, rect);
		}
		else GetGUI()->DrawSprite(sprite, z, rect);
	}
}
