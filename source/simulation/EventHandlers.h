// List of event handlers (for entities) the engine will call in to.
// Using integer tags should be ever-so-slightly faster than the hashmap lookup
// Also allows events to be renamed without affecting other code.

#ifndef EVENT_HANDLERS_INCLUDED
#define EVENT_HANDLERS_INCLUDED

enum EEventType
{
	EVENT_INITIALIZE,
	EVENT_TICK,
	EVENT_LAST,
};

static const wchar_t* EventNames[] =
{
	/* EVENT_INITIALIZE */ L"onInitialize",
	/* EVENT_TICK */ L"onTick"
};

#endif