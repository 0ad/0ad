/*
CCheckBox
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"
#include "CRadioButton.h"

using namespace std;

void CRadioButton::HandleMessage(const SGUIMessage &Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_PRESSED:
	{ // janwas added scoping to squelch ICC var decl warning
		for (vector_pObjects::iterator it = GetParent()->ChildrenItBegin(); it != GetParent()->ChildrenItEnd(); ++it)
		{
			// Notice, if you use other objects within the parent object that has got
			//  this the "checked", it too will change. Hence NO OTHER OBJECTS THAN
			//  RADIO BUTTONS SHOULD BE WITHIN IT!
			GUI<bool>::SetSetting((*it), "checked", false);
		}

		GUI<bool>::SetSetting(this, "checked", true);
		
		//GetGUI()->TEMPmessage = "Check box " + string((const TCHAR*)m_Name) + " was " + (m_Settings.m_Checked?"checked":"unchecked");
		break;
	}

	default:
		break;
	}
}
