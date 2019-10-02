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

#include "CRadioButton.h"

CRadioButton::CRadioButton(CGUI& pGUI)
	: CCheckBox(pGUI)
{
}

void CRadioButton::HandleMessage(SGUIMessage& Message)
{
	IGUIButtonBehavior::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_PRESSED:
		for (IGUIObject* const& obj : GetParent()->GetChildren())
		{
			// Notice, if you use other objects within the parent object that has got
			//  the setting "checked", it too will change. Hence NO OTHER OBJECTS THAN
			//  RADIO BUTTONS SHOULD BE WITHIN IT!
			obj->SetSetting<bool>("checked", false, true);
		}

		SetSetting<bool>("checked", true, true);
		break;

	default:
		break;
	}
}
