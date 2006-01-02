// List of event handlers (for entities) the engine will call in to.
// Using integer tags should be ever-so-slightly faster than the hashmap lookup
// Also allows events to be renamed without affecting other code.

#ifndef EVENT_HANDLERS_INCLUDED
#define EVENT_HANDLERS_INCLUDED

#include "scripting/DOMEvent.h"
#include "Vector3D.h"

class CDamageType;

class CEventInitialize : public CScriptEvent
{
public:
	CEventInitialize() : CScriptEvent( L"initialize", EVENT_INITIALIZE, false ) {}
};

class CEventTick : public CScriptEvent
{
public:
	CEventTick() : CScriptEvent( L"tick", EVENT_TICK, false ) {}
};

class CEventAttack : public CScriptEvent
{
	CEntity* m_target;
public:
	CEventAttack( CEntity* target );
};

class CEventGather : public CScriptEvent
{
	CEntity* m_target;
public:
	CEventGather( CEntity* target );
};

/*class CEventDamage : public CScriptEvent
{
	CEntity* m_inflictor;
	CDamageType* m_damage;
public:
	CEventDamage( CEntity* inflictor, CDamageType* damage );
};*/

class CEventHeal : public CScriptEvent
{
	CEntity* m_target;
public:
	CEventHeal( CEntity* target );
};

class CEventGeneric : public CScriptEvent
{
	CEntity* m_target;
	int m_action;
public:
	CEventGeneric( CEntity* target, int m_action );
};

class CEventTargetChanged : public CScriptEvent
{
	CEntity* m_target;
public:
	int m_defaultOrder;
	int m_defaultAction;
	CStrW m_defaultCursor;
	CEventTargetChanged( CEntity* target );
};

class CEventPrepareOrder : public CScriptEvent
{
	CEntity* m_target;
	int m_orderType;
public:
	CEventPrepareOrder( CEntity* target, int orderType );
};

class CEventOrderTransition : public CScriptEvent
{
	int m_orderPrevious;
	int m_orderCurrent;
	CEntity** m_target;
	CVector3D* m_worldPosition;
public:
	CEventOrderTransition( int orderPrevious, int orderCurrent, CEntity*& target, CVector3D& worldPosition );
};

#endif
