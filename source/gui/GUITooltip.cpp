#include "precompiled.h"

#include "GUITooltip.h"
#include "lib/timer.h"
#include "IGUIObject.h"
#include "CGUI.h"

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
: m_State(ST_IN_MOTION), m_PreviousObject(NULL), m_PreviousTooltipName(NULL)
{
}

const double CooldownTime = 0.25; // TODO: Don't hard-code this value

static bool GetTooltip(IGUIObject* obj, CStr* &style)
{
	if (obj && obj->SettingExists("tooltip-style"))
	{
		// Use GetSettingPointer to avoid unnecessary string-copying.
		// (The tooltip code is only run once per frame, but efficiency
		// would be nice anyway.)
		if (GUI<CStr>::GetSettingPointer(obj, "tooltip-style", style) == PS_OK
			&& style->Length())

			return true;
	}
	return false;
}

// Urgh - this is only a method because it needs to access HandleMessage (which
// is 'protected'), so it needs to be friendable (and so not a static function)
void GUITooltip::ShowTooltip(IGUIObject* obj, CPos pos, CStr& style, CGUI* gui)
{
	IGUIObject* tooltipobj = gui->FindObjectByName(style);
	if (! tooltipobj)
	{
		LOG_ONCE(ERROR, "gui", "Cannot find tooltip object named '%s'", (const char*)style);
		return;
	}
	// Unhide the object
	GUI<bool>::SetSetting(tooltipobj, "hidden", false);

	assert(obj);

	// These shouldn't fail:

	// Retrieve object's 'tooltip' setting
	CStr text;
	if (GUI<CStr>::GetSetting(obj, "tooltip", text) != PS_OK)
		debug_warn("Failed to retrieve tooltip text");

	// Set tooltip's caption
	if (tooltipobj->SetSetting("caption", text) != PS_OK)
		debug_warn("Failed to set tooltip caption");

	// Store mouse position inside the tooltip
	if (GUI<CPos>::SetSetting(tooltipobj, "_mousepos", pos) != PS_OK)
		debug_warn("Failed to set tooltip mouse position");

	// Make the tooltip object regenerate its text
	tooltipobj->HandleMessage(SGUIMessage(GUIM_SETTINGS_UPDATED, "caption"));
}

static void HideTooltip(CStr& style, CGUI* gui)
{
	IGUIObject* tooltipobj = gui->FindObjectByName(style);
	if (! tooltipobj)
	{
		LOG_ONCE(ERROR, "gui", "Cannot find tooltip object named '%s'", (const char*)style);
		return;
	}
	GUI<bool>::SetSetting(tooltipobj, "hidden", true);
}

static int GetTooltipTime(CStr& style, CGUI* gui)
{
	int time = 500; // default value (in msec)

	IGUIObject* tooltipobj = gui->FindObjectByName(style);
	if (! tooltipobj)
	{
		LOG_ONCE(ERROR, "gui", "Cannot find tooltip object named '%s'", (const char*)style);
		return time;
	}
	GUI<int>::GetSetting(tooltipobj, "time", time);
	return time;
}

void GUITooltip::Update(IGUIObject* Nearest, CPos MousePos, CGUI* GUI)
{
	double now = get_time();

	CStr* style = NULL;

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
		if (Nearest != m_PreviousObject)
		{
			if (GetTooltip(Nearest, style))
				nextstate = ST_SHOWING;
			else
				nextstate = ST_COOLING;
		}
		break;

	case ST_COOLING:
		if (now >= m_Time)
			nextstate = ST_IN_MOTION;
		else if (Nearest != m_PreviousObject && GetTooltip(Nearest, style))
			nextstate = ST_SHOWING;
		break;
	}

	// Handle state-entry code

	if (nextstate != -1)
	{
		switch (nextstate)
		{
		case ST_STATIONARY_TOOLTIP:
			m_Time = now + (double)GetTooltipTime(*style, GUI) / 1000.;
			break;

		case ST_SHOWING:
			// show tooltip
			ShowTooltip(Nearest, MousePos, *style, GUI);
			m_PreviousTooltipName = *style;
			break;

		case ST_COOLING:
			// hide the tooltip
			HideTooltip(m_PreviousTooltipName, GUI);
			m_Time = now + CooldownTime;
			break;
		}

		m_State = nextstate;
	}


	m_PreviousMousePos = MousePos;
	m_PreviousObject = Nearest;

}