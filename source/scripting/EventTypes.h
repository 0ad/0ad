// EventTypes.h
// Script event declarations

enum EEventType
{
	// Entity events
	EVENT_INITIALIZE = 0,
	EVENT_TICK,
	EVENT_ATTACK,
	EVENT_GATHER,
	EVENT_DAMAGE,
	EVENT_TARGET_CHANGED,
	EVENT_PREPARE_ORDER,
	EVENT_ORDER_TRANSITION,
	EVENT_LAST,
	// General events
	EVENT_GAME_START = 0,
	EVENT_GAME_TICK,
	EVENT_SELECTION_CHANGED,
};

// Only used for entity events...
static const wchar_t* EventNames[] =
{
	/* EVENT_INITIALIZE */ L"onInitialize",
	/* EVENT_TICK */ L"onTick",
	/* EVENT_ATTACK */ L"onAttack", /* This unit is the one doing the attacking... */
	/* EVENT_GATHER */ L"onGather", /* This unit is the one doing the gathering... */
	/* EVENT_DAMAGE */ L"onTakesDamage",
	/* EVENT_TARGET_CHANGED */ L"onTargetChanged", /* If this unit is selected and the mouseover object changes */
	/* EVENT_PREPARE_ORDER */ L"onPrepareOrder", /* To check if a unit can execute a given order */
	/* EVENT_ORDER_TRANSITION */ L"onOrderTransition" /* When we change orders (sometimes...) */
};