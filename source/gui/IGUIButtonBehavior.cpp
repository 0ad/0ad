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
			ScriptEvent("press");
		}
	}	break;

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
