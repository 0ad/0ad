/*
CGUIButtonBehavior
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx."
#include "GUI.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CGUIButtonBehavior::CGUIButtonBehavior() : m_Pressed(false)
{
}

CGUIButtonBehavior::~CGUIButtonBehavior()
{
}

//-------------------------------------------------------------------
//  Handles messages send from the CGUI
//  Input:
//    Message					Message ID, GUIM_*
//-------------------------------------------------------------------
void CGUIButtonBehavior::HandleMessage(const EGUIMessage &Message)
{
	switch (Message)
	{
	case GUIM_POSTPROCESS:
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

	case GUIM_MOUSE_PRESS_LEFT:
		m_Pressed = true;
		break;

	case GUIM_MOUSE_RELEASE_LEFT:
		if (m_Pressed)
		{
			m_Pressed = false;
			// BUTTON WAS CLICKED
			HandleMessage(GUIM_PRESSED);
		}
		break;

	default:
		break;
	}
}
