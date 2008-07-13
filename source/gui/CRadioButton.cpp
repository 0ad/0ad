/*
CCheckBox
*/

#include "precompiled.h"
#include "GUI.h"
#include "CRadioButton.h"


void CRadioButton::HandleMessage(const SGUIMessage &Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_PRESSED:
		{
			for (vector_pObjects::iterator it = GetParent()->ChildrenItBegin(); it != GetParent()->ChildrenItEnd(); ++it)
			{
				// Notice, if you use other objects within the parent object that has got
				//  the setting "checked", it too will change. Hence NO OTHER OBJECTS THAN
				//  RADIO BUTTONS SHOULD BE WITHIN IT!
				GUI<bool>::SetSetting((*it), "checked", false);
			}

			GUI<bool>::SetSetting(this, "checked", true);
			break;
		}

	default:
		break;
	}
}
