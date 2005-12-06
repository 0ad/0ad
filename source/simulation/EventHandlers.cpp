#include "precompiled.h"
#include "EventHandlers.h"
#include "Entity.h"

CEventAttack::CEventAttack( CEntity* target ) : CScriptEvent( L"attack", EVENT_ATTACK, true )
{
	m_target = target;
	AddLocalProperty( L"target", &m_target );
}
CEventHeal::CEventHeal( CEntity* target ) : CScriptEvent( L"heal", EVENT_HEAL, true)
{
	m_target = target;
	AddLocalProperty( L"target", &m_target );
}
CEventGather::CEventGather( CEntity* target ) : CScriptEvent( L"gather", EVENT_GATHER, true )
{
	m_target = target;
	AddLocalProperty( L"target", &m_target );
}

CEventDamage::CEventDamage( CEntity* inflictor, CDamageType* damage ) : CScriptEvent( L"takesDamage", EVENT_DAMAGE, true )
{
	m_inflictor = inflictor;
	m_damage = damage;
	AddLocalProperty( L"inflictor", &m_inflictor, true );
	AddLocalProperty( L"damage", &m_damage );
}

CEventTargetChanged::CEventTargetChanged( CEntity* target ) : CScriptEvent( L"targetChanged", EVENT_TARGET_CHANGED, false )
{
	m_target = target;
	m_defaultAction = -1;
	m_defaultCursor = L"arrow-default";
	AddLocalProperty( L"target", &m_target, true );
	AddLocalProperty( L"defaultAction", &m_defaultAction );
	AddLocalProperty( L"defaultCursor", &m_defaultCursor );
}

CEventPrepareOrder::CEventPrepareOrder( CEntity* target, int orderType ) : CScriptEvent( L"prepareOrder", EVENT_PREPARE_ORDER, true )
{
	m_target = target;
	m_orderType = orderType;
	AddLocalProperty( L"target", &m_target, true );
	AddLocalProperty( L"orderType", &m_orderType, true );
}

CEventOrderTransition::CEventOrderTransition( int orderPrevious, int orderCurrent, CEntity*& target, CVector3D& worldPosition ) : CScriptEvent( L"orderTransition", EVENT_ORDER_TRANSITION, true )
{
	m_orderPrevious = orderPrevious;
	m_orderCurrent = orderCurrent;
	m_target = &target;
	m_worldPosition = &worldPosition;
	AddLocalProperty( L"orderPrevious", &m_orderPrevious, true );
	AddLocalProperty( L"orderCurrent", &m_orderCurrent );
	AddLocalProperty( L"target", m_target );
	AddLocalProperty( L"position", m_worldPosition );
}
