/*
IGUIButtonBehavior
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx.h"
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
	switch (Message.type)
	{
	case GUIM_PREPROCESS:
		m_Pressed = false;
		break;

/*	case GUIM_POSTPROCESS:
		// Check if button has been pressed
		if (m_Pressed)
		{
			// Now check if mouse is released, that means
			//  it's released outside, since GUIM_MOUSE_RELEASE_LEFT
			//  would've handled m_Pressed and reset it already
			
			// Get input structure
///			if (GetGUI()->GetInput()->mRelease(NEMM_BUTTON1))
			{
				// Reset
				m_Pressed = false;
			}
		}
		break;
*/
	case GUIM_MOUSE_PRESS_LEFT:
		if (!GetBaseSettings().m_Enabled)
			break;

		m_Pressed = true;
		break;

	case GUIM_MOUSE_RELEASE_LEFT:
		if (!GetBaseSettings().m_Enabled)
			break;

		if (m_Pressed)
		{
			m_Pressed = false;
			// BUTTON WAS CLICKED
			HandleMessage(GUIM_PRESSED);
		}
		break;

	case GUIM_SETTINGS_UPDATED:
		// If it's hidden, then it can't be pressed
		//if (GetBaseSettings().m_Hidden)
		//	m_Pressed = false;
		break;

	default:
		break;
	}
}
