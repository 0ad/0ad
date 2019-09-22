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

#include "GUITooltip.h"

#include "gui/CGUI.h"
#include "gui/IGUIObject.h"
#include "lib/timer.h"
#include "ps/CLogger.h"

/*
	Tooltips:
	When holding the mouse stationary over an object for some amount of time,
	the tooltip is displayed. If the mouse moves off that object, the tooltip
	disappears. If the mouse re-enters an object within a short time, the new
	tooltip is displayed immediately. (This lets you run the mouse across a
	series of buttons, without waiting ages for the text to pop up every time.)

	See Visual Studio's toolbar buttons for an example.


	Implemented as a state machine:

	(where "*" lines are checked constantly, and "<" lines are handled
	on entry to that state)

	IN MOTION
	* If the mouse stops, check whether it should have a tooltip and move to
	  'STATIONARY, NO TOOLTIP' or 'STATIONARY, TOOLIP'
	* If the mouse enters an object with a tooltip delay of 0, switch to 'SHOWING'

	STATIONARY, NO TOOLTIP
	* If the mouse moves, switch to 'IN MOTION'

	STATIONARY, TOOLTIP
	< Set target time = now + tooltip time
	* If the mouse moves, switch to 'IN MOTION'
	* If now > target time, switch to 'SHOWING'

	SHOWING
	< Start displaying the tooltip
	* If the mouse leaves the object, check whether the new object has a tooltip
	  and switch to 'SHOWING' or 'COOLING'

	COOLING  (since I can't think of a better name)
	< Stop displaying the tooltip
	< Set target time = now + cooldown time
	* If the mouse has moved and is over a tooltipped object, switch to 'SHOWING'
	* If now > target time, switch to 'STATIONARY, NO TOOLTIP'
*/

enum
{
	ST_IN_MOTION,
	ST_STATIONARY_NO_TOOLTIP,
	ST_STATIONARY_TOOLTIP,
	ST_SHOWING,
	ST_COOLING
};

GUITooltip::GUITooltip()
: m_State(ST_IN_MOTION), m_PreviousObject(nullptr), m_PreviousTooltipName()
{
}

const double CooldownTime = 0.25; // TODO: Don't hard-code this value

bool GUITooltip::GetTooltip(IGUIObject* obj, CStr& style)
{
	if (obj && obj->SettingExists("_icon_tooltip_style") && obj->MouseOverIcon())
	{
		style = obj->GetSetting<CStr>("_icon_tooltip_style");
		if (!obj->GetSetting<CStrW>("_icon_tooltip").empty())
		{
			if (style.empty())
				style = "default";
			m_IsIconTooltip = true;
			return true;
		}
	}

	if (obj && obj->SettingExists("tooltip_style"))
	{
		style = obj->GetSetting<CStr>("tooltip_style");
		if (!obj->GetSetting<CStrW>("tooltip").empty())
		{
			if (style.empty())
				style = "default";
			m_IsIconTooltip = false;
			return true;
		}
	}

	return false;
}

void GUITooltip::ShowTooltip(IGUIObject* obj, const CPos& pos, const CStr& style, CGUI& pGUI)
{
	ENSURE(obj);

	if (style.empty())
		return;

	// Must be a CTooltip*, but we avoid dynamic_cast
	IGUIObject* tooltipobj = pGUI.FindObjectByName("__tooltip_" + style);
	if (!tooltipobj || !tooltipobj->SettingExists("use_object"))
	{
		LOGERROR("Cannot find tooltip named '%s'", style.c_str());
		return;
	}

	IGUIObject* usedobj; // object actually used to display the tooltip in

	const CStr& usedObjectName = tooltipobj->GetSetting<CStr>("use_object");
	if (usedObjectName.empty())
	{
		usedobj = tooltipobj;

		if (usedobj->SettingExists("_mousepos"))
		{
			usedobj->SetSetting<CPos>("_mousepos", pos, true);
		}
		else
		{
			LOGERROR("Object '%s' used by tooltip '%s' isn't a tooltip object!", usedObjectName.c_str(), style.c_str());
			return;
		}
	}
	else
	{
		usedobj = pGUI.FindObjectByName(usedObjectName);
		if (!usedobj)
		{
			LOGERROR("Cannot find object named '%s' used by tooltip '%s'", usedObjectName.c_str(), style.c_str());
			return;
		}
	}

	if (usedobj->SettingExists("caption"))
	{
		const CStrW& text = obj->GetSetting<CStrW>(m_IsIconTooltip ? "_icon_tooltip" : "tooltip");
		usedobj->SetSettingFromString("caption", text, true);
	}
	else
	{
		LOGERROR("Object '%s' used by tooltip '%s' must have a caption setting!", usedobj->GetPresentableName().c_str(), style.c_str());
		return;
	}

	// Every IGUIObject has a "hidden" setting
	usedobj->SetSetting<bool>("hidden", false, true);
}

void GUITooltip::HideTooltip(const CStr& style, CGUI& pGUI)
{
	if (style.empty())
		return;

	// Must be a CTooltip*, but we avoid dynamic_cast
	IGUIObject* tooltipobj = pGUI.FindObjectByName("__tooltip_" + style);
	if (!tooltipobj || !tooltipobj->SettingExists("use_object") || !tooltipobj->SettingExists("hide_object"))
	{
		LOGERROR("Cannot find tooltip named '%s' or it is not a tooltip", style.c_str());
		return;
	}

	const CStr& usedObjectName = tooltipobj->GetSetting<CStr>("use_object");
	if (!usedObjectName.empty())
	{
		IGUIObject* usedobj = pGUI.FindObjectByName(usedObjectName);
		if (usedobj && usedobj->SettingExists("caption"))
		{
			usedobj->SetSettingFromString("caption", L"", true);
		}
		else
		{
			LOGERROR("Object named '%s' used by tooltip '%s' does not exist or does not have a caption setting!", usedObjectName.c_str(), style.c_str());
			return;
		}

		if (tooltipobj->GetSetting<bool>("hide_object"))
			// Every IGUIObject has a "hidden" setting
			usedobj->SetSetting<bool>("hidden", true, true);
	}
	else
		tooltipobj->SetSetting<bool>("hidden", true, true);
}

static i32 GetTooltipDelay(const CStr& style, CGUI& pGUI)
{
	// Must be a CTooltip*, but we avoid dynamic_cast
	IGUIObject* tooltipobj = pGUI.FindObjectByName("__tooltip_" + style);

	if (!tooltipobj)
	{
		LOGERROR("Cannot find tooltip object named '%s'", style.c_str());
		return 500;
	}

	return tooltipobj->GetSetting<i32>("delay");
}

void GUITooltip::Update(IGUIObject* Nearest, const CPos& MousePos, CGUI& GUI)
{
	// Called once per frame, so efficiency isn't vital
	double now = timer_Time();

	CStr style;
	int nextstate = -1;

	switch (m_State)
	{
	case ST_IN_MOTION:
		if (MousePos == m_PreviousMousePos)
		{
			if (GetTooltip(Nearest, style))
				nextstate = ST_STATIONARY_TOOLTIP;
			else
				nextstate = ST_STATIONARY_NO_TOOLTIP;
		}
		else
		{
			// Check for movement onto a zero-delayed tooltip
			if (GetTooltip(Nearest, style) && GetTooltipDelay(style, GUI)==0)
			{
				// Reset any previous tooltips completely
				//m_Time = now + (double)GetTooltipDelay(style, GUI) / 1000.;
				HideTooltip(m_PreviousTooltipName, GUI);

				nextstate = ST_SHOWING;
			}
		}
		break;

	case ST_STATIONARY_NO_TOOLTIP:
		if (MousePos != m_PreviousMousePos)
			nextstate = ST_IN_MOTION;
		break;

	case ST_STATIONARY_TOOLTIP:
		if (MousePos != m_PreviousMousePos)
			nextstate = ST_IN_MOTION;
		else if (now >= m_Time)
		{
			// Make sure the tooltip still exists
			if (GetTooltip(Nearest, style))
				nextstate = ST_SHOWING;
			else
			{
				// Failed to retrieve style - the object has probably been
				// altered, so just restart the process
				nextstate = ST_IN_MOTION;
			}
		}
		break;

	case ST_SHOWING:
		// Handle special case of icon tooltips
		if (Nearest == m_PreviousObject && (!m_IsIconTooltip || Nearest->MouseOverIcon()))
		{
			// Still showing the same object's tooltip, but the text might have changed
			if (GetTooltip(Nearest, style))
				ShowTooltip(Nearest, MousePos, style, GUI);
		}
		else
		{
			// Mouse moved onto a new object
			if (GetTooltip(Nearest, style))
			{
				CStr style_old;

				// If we're displaying a tooltip with no delay, then we want to
				//  reset so that other object that should have delay can't
				//  "ride this tail", it have to wait.
				// Notice that this doesn't apply to when you go from one delay=0
				//  to another delay=0
				if (GetTooltip(m_PreviousObject, style_old) && GetTooltipDelay(style_old, GUI) == 0 &&
					GetTooltipDelay(style, GUI) != 0)
				{
					HideTooltip(m_PreviousTooltipName, GUI);
					nextstate = ST_IN_MOTION;
				}
				else
				{
					// Hide old scrollbar
					HideTooltip(m_PreviousTooltipName, GUI);
					nextstate = ST_SHOWING;
				}
			}
			else
				nextstate = ST_COOLING;
		}
		break;

	case ST_COOLING:
		if (GetTooltip(Nearest, style))
			nextstate = ST_SHOWING;
		else if (now >= m_Time)
			nextstate = ST_IN_MOTION;
		break;
	}

	// Handle state-entry code:
	if (nextstate != -1)
	{
		switch (nextstate)
		{
		case ST_STATIONARY_TOOLTIP:
			m_Time = now + (double)GetTooltipDelay(style, GUI) / 1000.;
			break;

		case ST_SHOWING:
			ShowTooltip(Nearest, MousePos, style, GUI);
			m_PreviousTooltipName = style;
			break;

		case ST_COOLING:
			HideTooltip(m_PreviousTooltipName, GUI);
			m_Time = now + CooldownTime;
			break;
		}

		m_State = nextstate;
	}

	m_PreviousMousePos = MousePos;
	m_PreviousObject = Nearest;
}
