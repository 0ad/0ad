// List of event handlers (for entities) the engine will call in to.
// Using integer tags should be ever-so-slightly faster than the hashmap lookup
// Also allows events to be renamed without affecting other code.

#ifndef EVENT_HANDLERS_INCLUDED
#define EVENT_HANDLERS_INCLUDED

#include "scripting/DOMEvent.h"

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

class CEventInitialize : public CScriptEvent
{
public: CEventInitialize() : CScriptEvent( L"initialize", false, EVENT_INITIALIZE ) {}
};

class CEventTick : public CScriptEvent
{
public: CEventTick() : CScriptEvent( L"tick", false, EVENT_TICK ) {}
};

#endif