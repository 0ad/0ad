// List of event handlers (for entities) the engine will call in to.
// Using integer tags should be ever-so-slightly faster than the hashmap lookup
// Also allows events to be renamed without affecting other code.

#ifndef EVENT_HANDLERS_INCLUDED
#define EVENT_HANDLERS_INCLUDED

#include "scripting/DOMEvent.h"
#include "Vector3D.h"

class CDamageType;

enum EEventType
{
	EVENT_INITIALIZE,
	EVENT_TICK,
	EVENT_ATTACK,
	EVENT_DAMAGE,
	EVENT_TARGET_CHANGED,
	EVENT_PREPARE_ORDER,
	EVENT_LAST,
};

static const wchar_t* EventNames[] =
{
	/* EVENT_INITIALIZE */ L"onInitialize",
	/* EVENT_TICK */ L"onTick",
	/* EVENT_ATTACK */ L"onAttack", /* This unit is the one doing the attacking... */
	/* EVENT_DAMAGE */ L"onTakesDamage",
	/* EVENT_TARGET_CHANGED */ L"onTargetChanged", /* If this unit is selected and the mouseover object changes */
	/* EVENT_PREPARE_ORDER */ L"onPrepareOrder" /* To check if a unit can execute a given order */
};

class CEventInitialize : public CScriptEvent
{
public:
	CEventInitialize() : CScriptEvent( L"initialize", false, EVENT_INITIALIZE ) {}
};

class CEventTick : public CScriptEvent
{
public:
	CEventTick() : CScriptEvent( L"tick", false, EVENT_TICK ) {}
};

class CEventAttack : public CScriptEvent
{
	CEntity* m_target;
public:
	CEventAttack( CEntity* target );
};

class CEventDamage : public CScriptEvent
{
	CEntity* m_inflictor;
	CDamageType* m_damage;
public:
	CEventDamage( CEntity* inflictor, CDamageType* damage );
};

class CEventTargetChanged : public CScriptEvent
{
	CEntity* m_target;
public:
	int m_defaultAction;
	CEventTargetChanged( CEntity* target );
};

class CEventPrepareOrder : public CScriptEvent
{
	CEntity* m_target;
	int m_orderType;
public:
	CEventPrepareOrder( CEntity* target, int orderType );
};

#endif
