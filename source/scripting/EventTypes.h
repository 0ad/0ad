// EventTypes.h
// Fairly game-specific event declarations for use with DOMEvent.
// Creates unique (for the current target) names for each event.
// DOMEvent currently uses a preallocated array of EVENT_LAST elements,
// so these must be consecutive integers starting with 0.

#ifndef EVENTTYPES_H__
#define EVENTTYPES_H__

enum EEventType
{
	// Entity events
	EVENT_INITIALIZE = 0,
	EVENT_TICK,
	EVENT_ATTACK,
	EVENT_GATHER,
	EVENT_DAMAGE,
	EVENT_HEAL,
	EVENT_TARGET_CHANGED,
	EVENT_PREPARE_ORDER,
	EVENT_ORDER_TRANSITION,
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
	/* EVENT_TICK */ L"onTick",
	/* EVENT_ATTACK */ L"onAttack", /* This unit is the one doing the attacking... */
	/* EVENT_GATHER */ L"onGather", /* This unit is the one doing the gathering... */
	/* EVENT_DAMAGE */ L"onTakesDamage",
	/* EVENT_HEAL */ L"onHeal",
	/* EVENT_TARGET_CHANGED */ L"onTargetChanged", /* If this unit is selected and the mouseover object changes */
	/* EVENT_PREPARE_ORDER */ L"onPrepareOrder", /* To check if a unit can execute a given order */
	/* EVENT_ORDER_TRANSITION */ L"onOrderTransition" /* When we change orders (sometimes...) */
};

#endif	// #ifndef EVENTTYPES_H__
