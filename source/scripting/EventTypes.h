/* Copyright (C) 2009 Wildfire Games.
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

// EventTypes.h
// Fairly game-specific event declarations for use with DOMEvent.
// Creates unique (for the current target) names for each event.
// DOMEvent currently uses a preallocated array of EVENT_LAST elements,
// so these must be consecutive integers starting with 0.

#ifndef INCLUDED_EVENTTYPES
#define INCLUDED_EVENTTYPES

enum EEventType
{
	// Entity events
	EVENT_INITIALIZE = 0,
	EVENT_DEATH,
	EVENT_TICK,
	EVENT_CONTACT_ACTION,
	EVENT_TARGET_EXHAUSTED,
	EVENT_START_CONSTRUCTION,
	EVENT_START_PRODUCTION,
	EVENT_CANCEL_PRODUCTION,
	EVENT_FINISH_PRODUCTION,
	EVENT_TARGET_CHANGED,
	EVENT_PREPARE_ORDER,
	EVENT_ORDER_TRANSITION,
	EVENT_NOTIFICATION,
	EVENT_FORMATION,
	EVENT_IDLE,
	EVENT_LAST,

	// Projectile events
	EVENT_IMPACT = 0,
	EVENT_MISS,

	// General events
	EVENT_GAME_START = 0,
	EVENT_GAME_TICK,
	EVENT_SELECTION_CHANGED,
	EVENT_WORLD_CLICK,
};

// Only used for entity events... (adds them as a property)
static const wchar_t* const EventNames[EVENT_LAST] =
{
	/* EVENT_INITIALIZE */ L"onInitialize",
	/* EVENT_DEATH */ L"onDeath",
	/* EVENT_TICK */ L"onTick",
	/* EVENT_CONTACT_ACTION */ L"onContactAction", /* For generic contact actions on a target unit, like attack or gather */
	/* EVENT_TARGET_EXHAUSTED*/ L"onTargetExhausted",	/* Called when the target of a generic action dies */
	/* EVENT_START_CONSTRUCTION */ L"onStartConstruction", /* We were selected when the user placed a building */
	/* EVENT_START_PRODUCTION */ L"onStartProduction", /* We're about to start training/researching something (deduct resources, etc) */
	/* EVENT_CANCEL_PRODUCTION */ L"onCancelProduction", /* Something in production has been cancelled */
	/* EVENT_FINISH_PRODUCTION */ L"onFinishProduction", /* We've finished something in production */
	/* EVENT_TARGET_CHANGED */ L"onTargetChanged", /* If this unit is selected and the mouseover object changes */
	/* EVENT_PREPARE_ORDER */ L"onPrepareOrder", /* To check if a unit can execute a given order */
	/* EVENT_ORDER_TRANSITION */ L"onOrderTransition", /* When we change orders (sometimes...) */
	/* EVENT_NOTIFICATION */ L"onNotification",  /*When we receive a notification */
	/* EVENT_FORMATION */ L"onFormation", /* When this unit does something with a formation */
	/* EVENT_IDLE */ L"onIdle", /* When this unit becomes idle, do something */
};

#endif	// #ifndef INCLUDED_EVENTTYPES


